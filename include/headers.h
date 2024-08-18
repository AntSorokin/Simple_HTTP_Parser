#ifndef HEADERS_H
#define HEADERS_H

#include <stdbool.h>
#include "hashmap.h"
#include "array.h"

/**
 * headers is a hashmap of dynamically resizable arrays
 * If multiple headers with the same key are found then 
 * the value will be appended to the end of the array.
 */

/**
 * HEADERS_OK_ERROR: everything is normal
 * HEADERS_OUT_OF_MEM: malloc or calloc failed
 * HEADERS_OUT_OF_BOUNDS: the number of headers has reached max_headers
 */
typedef enum {
    HEADERS_OK_ERROR,
    HEADERS_OUT_OF_MEM,
    HEADERS_OUT_OF_BOUNDS,
} headers_state;

typedef struct _headers {
    hashmap_t *headers;
    uint64_t header_count;
    uint64_t max_headers;
} headers_t;

/**
 * Allocates memory for hashmap(http_request_init handles this)
 * @param max_headers Max headers that can be stored
 * @returns null if failed to allocate memory
 */
headers_t* headers_init(uint64_t max_headers);
/**
 * Free's header(http_request_free handles this)
 */
void headers_free(headers_t *headers);

/**
 * Can add header by supplying a key and val
 * @returns OUT_OF_BOUNDS if number of vals reached max headers or OUT_OF_MEM if failed malloc
 */
headers_state add_header(headers_t *headers, char *key, char *val);
/**
 * Gets the last value added to a specific key
 * @returns value or null if not found
 */
char* get_last_header(headers_t *headers, char *key);

/**
 * Can get the val_index value for the given key
 * @returns value or null if not found
 */
char* get_header(headers_t *headers, char *key, uint64_t val_index);

/**
 * @returns number of values for a given key
 */
uint64_t num_header_vals(headers_t *headers, char *key);
#endif