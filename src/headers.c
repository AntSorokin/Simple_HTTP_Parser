#include <string.h>
#include "headers.h"

//djb2 hash function for strings
static uint64_t string_hash(void *key) {
    unsigned char *str = (unsigned char*) key;
    unsigned long hash = 5381;
    int c;

    while((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}


static bool string_equal(void *a, void *b) {
    return strcmp((char*)a, (char*)b) == 0;
}

/**
 * Hashmap stores key and value in a pair.
 * When deallocating memory, each pair stored in the hashmap is iterated by this function
 */
static void _headers_free(hashmap_pair_t *pair, void *state) {
    free(pair -> key);
    array_struct(char*) *array = pair -> val;

    //deallocates each value in array
    for(uint64_t i = 0;i < array -> size; ++i) {
        free(array -> buf[i]);
    }

    array_free((*array));
    free(array);
    free(pair);
}

headers_t* headers_init(uint64_t max_headers) {
    headers_t *temp = malloc(sizeof(headers_t));

    if(!temp) {
        return 0;
    }

    temp -> max_headers = max_headers;
    //hashmap takes in hash function and equality function
    temp -> headers = hashmap_init(max_headers * 2, string_hash, string_equal);
    temp -> header_count = 0;
    
    if(!temp -> headers) {
        free(temp);
        return 0;
    }
    
    return temp;
}

/**
 * Deallocates all memory in hashmap
 */
void headers_free(headers_t *headers) {
    if(headers) {
        if(headers -> headers) {
            hashmap_free(headers -> headers, _headers_free, 0);
        }
        free(headers);
    } 
}

headers_state add_header(headers_t *headers, char *key, char *val) {
    hashmap_pair_t *pair = hashmap_get(headers -> headers, key);

    // Will not add header if max_headers is reached
    if(headers -> header_count == headers -> max_headers) {
        return HEADERS_OUT_OF_BOUNDS;
    }

    // If key doesnt exist
    if(!pair) {
        pair = malloc(sizeof(hashmap_pair_t));
        if(!pair) {
            return HEADERS_OUT_OF_MEM;
        }
        pair -> key = key;

        array_struct(char*) *array = malloc(sizeof(array_struct(char*)));

        if(array == 0) {
            free(pair);
            return HEADERS_OUT_OF_MEM;
        }

        array_init(char*, (*array), 1);
        //Adds value to array
        array_add(char*, (*array), val);

        if(array -> error) {
            free(pair);
            array_free((*array));
            free(array);
            return HEADERS_OUT_OF_MEM;
        }

        pair -> val = array;

        //Adds key and array with value to hashmap
        if(!hashmap_add(headers -> headers, pair)) {
            free(pair);
            array_free((*array));
            free(array);
            return HEADERS_OUT_OF_MEM;
        }
    }
    else {
        //If key is found, adds value to already existing array
        array_struct(char*) *array = pair -> val;
        array_add(char*, (*array), val);
        if(array -> error) {
            return HEADERS_OUT_OF_MEM;
        }
    }

    headers -> header_count++;
    return HEADERS_OK_ERROR;
}

char* get_last_header(headers_t *headers, char *key) {
    hashmap_pair_t *pair = hashmap_get(headers -> headers, key);
    if(!pair) {
        return 0;
    }

    array_struct(char*) *array = pair -> val;
    if(array -> size == 0) {
        return 0;
    }

    char *ret = 0;
    array_get((*array), array -> size - 1, ret);
    return ret;
}

char* get_header(headers_t *headers, char *key, uint64_t val_index) {
    hashmap_pair_t *pair = hashmap_get(headers -> headers, key);

    if(!pair) {
        return 0;
    }

    array_struct(char*) *array = pair -> val;

    char *ret = 0;
    array_get((*array), val_index, ret);

    //Needs to reset state of array if out of bounds so array can be used later
    if(array -> error == ARRAY_OUT_OF_BOUNDS) {
        array -> error = ARRAY_OK_ERROR;
        return 0;
    }

    return ret;
}

uint64_t num_header_vals(headers_t *headers, char *key) {
    hashmap_pair_t *pair = hashmap_get(headers -> headers, key);
    if(!pair) {
        return 0;
    }

    array_struct(char*) *array = pair -> val;
    return array -> size;
}

