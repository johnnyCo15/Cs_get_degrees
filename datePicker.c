#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define HASH_SIZE 10

// define a struct for key-value pair
typedef struct KeyValuePair {
    char* key;
    int value;
    struct KeyValuePair* next; 
} KeyValuePair;

// define for hashmap 
typedef struct hashmap {
    KeyValuePair* data[HASH_SIZE]
}hashmap;

// hashing function 
int hash(char* key) {
    int hash = 0;
    while (*key) {
        hash = (hash + *key) % HASH_SIZE;
        key++;
    }
    return hash;
}

// function to create new hashmap 
hashmap* createHashMap() {
    hashmap* map = (hashmap*)malloc(sizeof(hashmap));
    for (int i = 0; i < HASH_SIZE; i++) {
        map->data[i] = NULL;
    }
    return map;
}

// function to insert key value pair to hashmap 
void insert(hashmap* map, char* key, int value) {
    int index = hash(key);
    KeyValuePair* kvp = (KeyValuePair*)malloc(sizeof(KeyValuePair));
    kvp->key = strdup(key);
    kvp->value = value;
    kvp->next = map->data[index]; // Chaining
    map->data[index] = kvp;
}

// function to randomly pick 
KeyValuePair* getRandomEntity(hashmap* map) {
    // Initialize random seed
    srand(time(NULL));

    // Randomly select an index
    int index = rand() % HASH_SIZE;
    
    // Traverse the chain at the selected index
    KeyValuePair* current = map->data[index];
    while (current != NULL) {
        current = current->next;
    }

    return current; // Return the randomly selected entity
}



void main (){
    hashmap* map = createHashMap();
    
    exit(0);

}