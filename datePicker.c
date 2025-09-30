#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <curl/curl.h>

typedef enum {
	HEMISPHERE_NORTH = 0,
	HEMISPHERE_SOUTH = 1
} Hemisphere;

typedef enum {
	WEATHER_SUNNY = 0,
	WEATHER_RAINY,
	WEATHER_SNOWY,
	WEATHER_WINDY,
	WEATHER_CLOUDY,
	WEATHER_STORMY,
	WEATHER_ANY
} WeatherType;

static void trim_newline(char *s) {
	if (!s) return;
	size_t n = strlen(s);
	if (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[n - 1] = '\0';
	/* Handle Windows CRLF if present */
	n = strlen(s);
	if (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[n - 1] = '\0';
}

static void to_lower_str(char *s) {
	if (!s) return;
	for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

static Hemisphere parse_hemisphere(const char *s) {
	if (!s) return HEMISPHERE_NORTH;
	if (strstr(s, "south")) return HEMISPHERE_SOUTH;
	return HEMISPHERE_NORTH;
}

static WeatherType parse_weather(const char *s) {
	if (!s) return WEATHER_ANY;
	if (strstr(s, "sun")) return WEATHER_SUNNY;
	if (strstr(s, "rain")) return WEATHER_RAINY;
	if (strstr(s, "snow")) return WEATHER_SNOWY;
	if (strstr(s, "wind")) return WEATHER_WINDY;
	if (strstr(s, "cloud")) return WEATHER_CLOUDY;
	if (strstr(s, "storm")) return WEATHER_STORMY;
	if (strstr(s, "any") || strstr(s, "random")) return WEATHER_ANY;
	return WEATHER_ANY;
}

static int is_leap_year(int year) {
	if (year % 400 == 0) return 1;
	if (year % 100 == 0) return 0;
	return year % 4 == 0;
}

static int days_in_month(int year, int month_1_to_12) {
	static const int days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	if (month_1_to_12 == 2) return days[1] + (is_leap_year(year) ? 1 : 0);
	return days[month_1_to_12 - 1];
}

typedef enum {
	SEASON_WINTER = 0,
	SEASON_SPRING = 1,
	SEASON_SUMMER = 2,
	SEASON_FALL = 3
} Season;

static Season month_to_season(int month_1_to_12, Hemisphere hemi) {
	/* Meteorological seasons: DJF, MAM, JJA, SON */
	int m = month_1_to_12;
	Season season;
	if (m == 12 || m == 1 || m == 2) season = SEASON_WINTER;
	else if (m >= 3 && m <= 5) season = SEASON_SPRING;
	else if (m >= 6 && m <= 8) season = SEASON_SUMMER;
	else season = SEASON_FALL;
	if (hemi == HEMISPHERE_SOUTH) {
		/* Invert seasons for southern hemisphere */
		if (season == SEASON_WINTER) return SEASON_SUMMER;
		if (season == SEASON_SPRING) return SEASON_FALL;
		if (season == SEASON_SUMMER) return SEASON_WINTER;
		return SEASON_SPRING;
	}
	return season;
}

static double season_affinity(WeatherType w, Season s) {
	/* Heuristic weights: higher means more likely */
	switch (w) {
		case WEATHER_SUNNY:
			if (s == SEASON_SUMMER) return 1.0;
			if (s == SEASON_SPRING) return 0.7;
			if (s == SEASON_FALL) return 0.5;
			return 0.2; /* winter */
		case WEATHER_RAINY:
			if (s == SEASON_SPRING) return 1.0;
			if (s == SEASON_FALL) return 0.8;
			if (s == SEASON_SUMMER) return 0.5;
			return 0.4; /* winter */
		case WEATHER_SNOWY:
			if (s == SEASON_WINTER) return 1.0;
			return 0.05; /* rare otherwise */
		case WEATHER_WINDY:
			if (s == SEASON_FALL) return 1.0;
			if (s == SEASON_SPRING) return 0.8;
			return 0.4; /* summer/winter */
		case WEATHER_CLOUDY:
			if (s == SEASON_WINTER) return 1.0;
			if (s == SEASON_FALL) return 0.8;
			if (s == SEASON_SPRING) return 0.6;
			return 0.4; /* summer */
		case WEATHER_STORMY:
			if (s == SEASON_SUMMER) return 1.0; /* convective */
			if (s == SEASON_SPRING) return 0.8;
			if (s == SEASON_FALL) return 0.6;
			return 0.3; /* winter */
		case WEATHER_ANY:
		default:
			return 1.0;
	}
}

typedef struct {
	int year;
	int month; /* 1-12 */
	double weight;
} MonthCandidate;

static int pick_weighted_index(const double *weights, int n) {
	double total = 0.0;
	for (int i = 0; i < n; ++i) total += (weights[i] > 0.0 ? weights[i] : 0.0);
	if (total <= 0.0) return rand() % n;
	double r = ((double)rand() / (double)RAND_MAX) * total;
	double acc = 0.0;
	for (int i = 0; i < n; ++i) {
		acc += (weights[i] > 0.0 ? weights[i] : 0.0);
		if (r <= acc) return i;
	}
	return n - 1;
}

static void pick_date(Hemisphere hemi, WeatherType weather, int *out_year, int *out_month, int *out_day) {
	/* Build the next 12 months window starting this month */
	time_t now = time(NULL);
	struct tm *lt = localtime(&now);
	int cur_year = lt->tm_year + 1900;
	int cur_month = lt->tm_mon + 1; /* 1-12 */

	MonthCandidate candidates[12];
	double weights[12];
	for (int i = 0; i < 12; ++i) {
		int m = ((cur_month - 1 + i) % 12) + 1;
		int y = cur_year + ((cur_month - 1 + i) / 12);
		Season season = month_to_season(m, hemi);
		double w = season_affinity(weather, season);
		/* Slightly boost nearer months so we respect "time of year it is" */
		double recency = 1.0 - (i * 0.03); /* decays across the year */
		if (recency < 0.7) recency = 0.7;
		candidates[i].year = y;
		candidates[i].month = m;
		candidates[i].weight = w * recency;
		weights[i] = candidates[i].weight;
	}

	int idx = pick_weighted_index(weights, 12);
	int year = candidates[idx].year;
	int month = candidates[idx].month;
	int dim = days_in_month(year, month);
	int day = (rand() % dim) + 1;

	*out_year = year;
	*out_month = month;
	*out_day = day;
}

typedef struct {
	char *data;
	size_t size;
} MemoryBuffer;

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	MemoryBuffer *mem = (MemoryBuffer *)userp;
	char *ptr = (char*)realloc(mem->data, mem->size + realsize + 1);
	if (!ptr) return 0;
	mem->data = ptr;
	memcpy(&(mem->data[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->data[mem->size] = '\0';
	return realsize;
}

static int fetch_openweather_json(const char *city, const char *api_key, char **out_json) {
	if (!city || !api_key || !out_json) return 0;
	CURL *curl = curl_easy_init();
	if (!curl) return 0;
	char *city_enc = curl_easy_escape(curl, city, 0);
	if (!city_enc) { curl_easy_cleanup(curl); return 0; }
	char url[512];
	snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", city_enc, api_key);
	curl_free(city_enc);

	int attempts = 0;
	while (attempts < 3) {
		attempts++;
		MemoryBuffer chunk = {0};
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		CURLcode res = curl_easy_perform(curl);
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		if (res == CURLE_OK && http_code >= 200 && http_code < 300 && chunk.data && chunk.size > 0) {
			*out_json = chunk.data;
			curl_easy_cleanup(curl);
			return 1;
		}
		free(chunk.data);
	}
	curl_easy_cleanup(curl);
	return 0;
}

static int extract_json_string_field(const char *json, const char *key, char *out, size_t outsz) {
	/* naive extractor for patterns like \"main\":\"Clear\" within objects */
	if (!json || !key || !out || outsz == 0) return 0;
	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	const char *p = strstr(json, pattern);
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	p++;
	while (*p && (*p == ' ')) p++;
	if (*p == '"') {
		p++;
		size_t i = 0;
		while (*p && *p != '"' && i + 1 < outsz) {
			out[i++] = *p++;
		}
		out[i] = '\0';
		return 1;
	}
	return 0;
}

static int extract_json_double_field(const char *json, const char *key, double *out_value) {
	if (!json || !key || !out_value) return 0;
	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\"", key);
	const char *p = strstr(json, pattern);
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	p++;
	while (*p && (*p == ' ')) p++;
	char *endptr = NULL;
	double v = strtod(p, &endptr);
	if (endptr == p) return 0;
	*out_value = v;
	return 1;
}

static int parse_openweather_payload(const char *json, char *weather_main_out, size_t weather_out_sz, double *lat_out) {
	if (!json) return 0;
	weather_main_out[0] = '\0';
	if (lat_out) *lat_out = 0.0;

	/* Scope into coord {} for lat */
	const char *p = strstr(json, "\"coord\"");
	if (p) {
		int depth = 0;
		const char *brace = strchr(p, '{');
		if (brace) {
			p = brace + 1; depth = 1;
			while (*p && depth > 0) {
				if (*p == '{') depth++;
				else if (*p == '}') depth--;
				else if (depth == 1 && lat_out) {
					const char *latk = strstr(p, "\"lat\"");
					if (latk && latk < strchr(p, '}')) {
						double v = 0.0; if (extract_json_double_field(latk, "lat", &v)) *lat_out = v;
						break;
					}
				}
				p++;
			}
		}
	}

	/* weather: [ { main: "..." } ] */
	const char *w = strstr(json, "\"weather\"");
	if (w) {
		const char *arr = strchr(w, '[');
		if (arr) {
			/* find first object's main within matching brackets */
			int depth = 0; const char *q = arr; int found = 0;
			while (*q && !found) {
				if (*q == '[') depth++;
				else if (*q == ']') depth--;
				else if (*q == '"' && depth > 0) {
					/* try parse at q-1 window */
					if (extract_json_string_field(q, "main", weather_main_out, weather_out_sz)) { found = 1; break; }
				}
				q++;
			}
		}
	}
	return weather_main_out[0] != '\0';
}

static Hemisphere hemisphere_from_lat(double lat) {
	return (lat < 0.0) ? HEMISPHERE_SOUTH : HEMISPHERE_NORTH;
}

static WeatherType map_openweather_main_to_type(const char *main_str) {
	if (!main_str) return WEATHER_ANY;
	char tmp[64];
	strncpy(tmp, main_str, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = '\0';
	to_lower_str(tmp);
	if (strstr(tmp, "clear")) return WEATHER_SUNNY;
	if (strstr(tmp, "cloud")) return WEATHER_CLOUDY;
	if (strstr(tmp, "rain") || strstr(tmp, "drizzle")) return WEATHER_RAINY;
	if (strstr(tmp, "snow")) return WEATHER_SNOWY;
	if (strstr(tmp, "thunder")) return WEATHER_STORMY;
	if (strstr(tmp, "squall") || strstr(tmp, "tornado")) return WEATHER_STORMY;
	if (strstr(tmp, "wind")) return WEATHER_WINDY;
	return WEATHER_ANY;
}

static int list_activity_ideas(WeatherType w, Season s, char ideas[][96], int maxIdeas) {
	/* Populate up to maxIdeas short activity strings */
	int count = 0;
	#define ADD_IDEA(str) do { if (count < maxIdeas) { strncpy(ideas[count], (str), 95); ideas[count][95] = '\0'; count++; } } while (0)
	if (w == WEATHER_SUNNY) {
		if (s == SEASON_SUMMER) {
			ADD_IDEA("Beach day and picnic");
			ADD_IDEA("Sunset hike");
			ADD_IDEA("Outdoor concert");
			ADD_IDEA("Kayaking on a lake");
		} else if (s == SEASON_SPRING) {
			ADD_IDEA("Botanical garden visit");
			ADD_IDEA("City bike tour");
			ADD_IDEA("Farmer's market stroll");
		} else if (s == SEASON_FALL) {
			ADD_IDEA("Scenic foliage drive");
			ADD_IDEA("Pumpkin patch + cider");
			ADD_IDEA("Harvest festival");
		} else {
			ADD_IDEA("Sunny winter walk");
			ADD_IDEA("Outdoor photography");
		}
	} else if (w == WEATHER_RAINY) {
		ADD_IDEA("Museum afternoon");
		ADD_IDEA("Cozy cafe and book");
		ADD_IDEA("Aquarium visit");
		if (s == SEASON_SPRING || s == SEASON_FALL) ADD_IDEA("Rainy day ramen crawl");
	} else if (w == WEATHER_SNOWY) {
		ADD_IDEA("Sledding at a local hill");
		ADD_IDEA("Ice skating rink");
		ADD_IDEA("Snowshoe trail");
		ADD_IDEA("Hot chocolate and movie night");
	} else if (w == WEATHER_WINDY) {
		ADD_IDEA("Kite flying");
		ADD_IDEA("Coastal walk");
		ADD_IDEA("Art gallery visit");
	} else if (w == WEATHER_CLOUDY) {
		ADD_IDEA("Matinee at the theater");
		ADD_IDEA("Board game cafe");
		ADD_IDEA("Local brewery tour");
	} else if (w == WEATHER_STORMY) {
		ADD_IDEA("Home cooking class night");
		ADD_IDEA("Planetarium or science center");
		ADD_IDEA("Spa day");
	} else {
		ADD_IDEA("Surprise local event");
		ADD_IDEA("New restaurant tryout");
	}
	#undef ADD_IDEA
	return count;
}

typedef struct {
	int year;
	int month;
	int day;
	char activity[96];
} ActivityOption;

static int generate_activity_options(Hemisphere hemi, WeatherType weather, ActivityOption *options, int maxOptions) {
	if (!options || maxOptions <= 0) return 0;
	/* Create activities list based on the season of the picked month for each option */
	int produced = 0;
	/* attempt to produce up to maxOptions unique-looking pairs */
	for (int i = 0; i < maxOptions * 2 && produced < maxOptions; ++i) {
		int y, m, d;
		pick_date(hemi, weather, &y, &m, &d);
		Season s = month_to_season(m, hemi);
		char ideas[8][96];
		int n = list_activity_ideas(weather, s, ideas, 8);
		if (n <= 0) continue;
		const char *idea = ideas[rand() % n];
		/* ensure not a duplicate of immediate previous */
		int dup = 0;
		for (int k = 0; k < produced; ++k) {
			if (options[k].year == y && options[k].month == m && options[k].day == d && strcmp(options[k].activity, idea) == 0) {
				dup = 1; break;
			}
		}
		if (dup) continue;
		options[produced].year = y;
		options[produced].month = m;
		options[produced].day = d;
		strncpy(options[produced].activity, idea, sizeof(options[produced].activity) - 1);
		options[produced].activity[sizeof(options[produced].activity) - 1] = '\0';
		produced++;
	}
	return produced;
}

/* ---------------- Ticketmaster Discovery API Integration ---------------- */

static char *url_encode_component(CURL *curl, const char *s) {
	if (!s) return NULL;
	char *escaped = curl_easy_escape(curl, s, 0);
	if (!escaped) return NULL;
	/* Caller must free via curl_free */
	return escaped;
}

static void format_date_range_utc(int year, int month, int day, char *start_iso, size_t start_sz, char *end_iso, size_t end_sz) {
	/* Ticketmaster expects ISO8601 with Z. We'll use whole day UTC */
	snprintf(start_iso, start_sz, "%04d-%02d-%02dT00:00:00Z", year, month, day);
	snprintf(end_iso, end_sz, "%04d-%02d-%02dT23:59:59Z", year, month, day);
}

static int fetch_ticketmaster_json(const char *city, const char *api_key, int year, int month, int day, char **out_json) {
	if (!city || !api_key || !out_json) return 0;
	CURL *curl = curl_easy_init();
	if (!curl) return 0;
	char start_iso[32];
	char end_iso[32];
	format_date_range_utc(year, month, day, start_iso, sizeof(start_iso), end_iso, sizeof(end_iso));
	char *city_enc = url_encode_component(curl, city);
	char *start_enc = url_encode_component(curl, start_iso);
	char *end_enc = url_encode_component(curl, end_iso);
	if (!city_enc || !start_enc || !end_enc) {
		if (city_enc) curl_free(city_enc);
		if (start_enc) curl_free(start_enc);
		if (end_enc) curl_free(end_enc);
		curl_easy_cleanup(curl);
		return 0;
	}
	char url[1024];
	snprintf(url, sizeof(url),
		"https://app.ticketmaster.com/discovery/v2/events.json?apikey=%s&city=%s&startDateTime=%s&endDateTime=%s&size=10",
		api_key, city_enc, start_enc, end_enc);
	curl_free(city_enc);
	curl_free(start_enc);
	curl_free(end_enc);

	int attempts = 0;
	while (attempts < 3) {
		attempts++;
		MemoryBuffer chunk = {0};
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		CURLcode res = curl_easy_perform(curl);
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		if (res == CURLE_OK && http_code >= 200 && http_code < 300 && chunk.data && chunk.size > 0) {
			*out_json = chunk.data;
			curl_easy_cleanup(curl);
			return 1;
		}
		free(chunk.data);
	}
	curl_easy_cleanup(curl);
	return 0;
}

static int parse_ticketmaster_event_names(const char *json, char names[][128], int max_names) {
	if (!json || !names || max_names <= 0) return 0;
	/* Scope into _embedded { events: [ { name: "..." } ] } */
	const char *emb = strstr(json, "\"_embedded\"");
	if (!emb) return 0;
	const char *brace = strchr(emb, '{');
	if (!brace) return 0;
	int depth = 1; const char *p = brace + 1;
	const char *events = NULL;
	while (*p && depth > 0) {
		if (*p == '{') depth++;
		else if (*p == '}') depth--;
		else if (!events) {
			const char *ev = strstr(p, "\"events\"");
			if (ev && ev < strchr(p, '}')) { events = ev; break; }
		}
		p++;
	}
	if (!events) return 0;
	const char *arr = strchr(events, '[');
	if (!arr) return 0;
	/* Iterate names inside events array */
	depth = 0; p = arr;
	int count = 0;
	while (*p && count < max_names) {
		if (*p == '[') depth++;
		else if (*p == ']') { depth--; if (depth <= 0) break; }
		else if (*p == '"' && depth > 0) {
			if (extract_json_string_field(p, "name", names[count], 128)) {
				/* Filter out generic container names if any */
				if (strlen(names[count]) > 0) count++;
			}
		}
		p++;
	}
	return count;
}

int main(void) {
	/* Seed once */
	srand((unsigned int)time(NULL));

	char city[128];
	char api_key_input[128];
	printf("Enter city (e.g., London or Austin,US): ");
	fflush(stdout);
	if (!fgets(city, sizeof(city), stdin)) return 1;
	trim_newline(city);

	const char *api_key_env = getenv("OPENWEATHER_API_KEY");
	if (api_key_env && *api_key_env) {
		strncpy(api_key_input, api_key_env, sizeof(api_key_input) - 1);
		api_key_input[sizeof(api_key_input) - 1] = '\0';
	} else {
		printf("Enter OpenWeather API key: ");
		fflush(stdout);
		if (!fgets(api_key_input, sizeof(api_key_input), stdin)) return 1;
		trim_newline(api_key_input);
	}

	char *json = NULL;
	if (!fetch_openweather_json(city, api_key_input, &json)) {
		fprintf(stderr, "Failed to fetch weather for %s. Falling back to manual input.\n", city);
		char hemi_input[64];
		char weather_input[64];
		printf("Enter hemisphere (north/south): ");
		fflush(stdout);
		if (!fgets(hemi_input, sizeof(hemi_input), stdin)) return 1;
		trim_newline(hemi_input);
		to_lower_str(hemi_input);
		printf("Enter weather type (sunny/rainy/snowy/windy/cloudy/stormy/any): ");
		fflush(stdout);
		if (!fgets(weather_input, sizeof(weather_input), stdin)) return 1;
		trim_newline(weather_input);
		to_lower_str(weather_input);
		Hemisphere hemi_fallback = parse_hemisphere(hemi_input);
		WeatherType weather_fallback = parse_weather(weather_input);
		ActivityOption opts[5];
		int n = generate_activity_options(hemi_fallback, weather_fallback, opts, 5);
		printf("\nActivity date options (fallback):\n");
		for (int i = 0; i < n; ++i) {
			printf("- %04d-%02d-%02d: %s\n", opts[i].year, opts[i].month, opts[i].day, opts[i].activity);
		}
		return 0;
	}

	char weather_main[64] = {0};
	double lat = 0.0;
	parse_openweather_payload(json, weather_main, sizeof(weather_main), &lat);
	free(json);
	Hemisphere hemi = hemisphere_from_lat(lat);
	WeatherType weather = map_openweather_main_to_type(weather_main);

	/* Ask for Ticketmaster API Key */
	char tm_api_key[128];
	const char *tm_env = getenv("TICKETMASTER_API_KEY");
	if (tm_env && *tm_env) {
		strncpy(tm_api_key, tm_env, sizeof(tm_api_key) - 1);
		tm_api_key[sizeof(tm_api_key) - 1] = '\0';
	} else {
		printf("Enter Ticketmaster API key: ");
		fflush(stdout);
		if (!fgets(tm_api_key, sizeof(tm_api_key), stdin)) return 1;
		trim_newline(tm_api_key);
	}

	ActivityOption options[6];
	int count = generate_activity_options(hemi, weather, options, 6);
	printf("\nCurrent weather: %s | Hemisphere: %s\n", weather_main[0] ? weather_main : "unknown", hemi == HEMISPHERE_SOUTH ? "south" : "north");
	printf("Activity date options for %s (with events):\n", city);
	for (int i = 0; i < count; ++i) {
		printf("- %04d-%02d-%02d: %s\n", options[i].year, options[i].month, options[i].day, options[i].activity);
		char *events_json = NULL;
		if (fetch_ticketmaster_json(city, tm_api_key, options[i].year, options[i].month, options[i].day, &events_json)) {
			char names[10][128];
			int n = parse_ticketmaster_event_names(events_json, names, 5);
			free(events_json);
			if (n > 0) {
				for (int k = 0; k < n; ++k) {
					printf("    • %s\n", names[k]);
				}
			} else {
				printf("    (no events found)\n");
			}
		} else {
			printf("    (failed to fetch events)\n");
		}
	}
	return 0;
}