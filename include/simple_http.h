#ifndef SIMPLE_HTTP_H
#define SIMPLE_HTTP_H

#include <stdint.h>
#include "headers.h"

#ifndef HTTP_MAX_BODY_SIZE
    #define HTTP_MAX_BODY_SIZE 2048
#endif

#ifndef HTTP_MAX_HEADER_KEY_SIZE
    #define HTTP_MAX_HEADER_KEY_SIZE 255
#endif

#ifndef HTTP_MAX_HEADER_VAL_SIZE
    #define HTTP_MAX_HEADER_VAL_SIZE 512
#endif

#ifndef HTTP_MAX_PATH_SIZE
    #define HTTP_MAX_PATH_SIZE 30
#endif

#ifndef HTTP_MAX_HEADERS
    #define HTTP_MAX_HEADERS 30
#endif

/**
 * HTTP_OK: Default value, everything is ok
 * HTTP_OUT_OF_MEM: A malloc or calloc failed
 * HTTP_OUT_OF_BOUNDS: A values length surpassed a MAX macro
 * HTTP_INVALID_HEADER: Some part of the header is invalid(missing colon, no key, etc)
 */
typedef enum {
    HTTP_OK,
    HTTP_OUT_OF_MEM,
    HTTP_OUT_OF_BOUNDS,
    HTTP_INVALID_HEADER,
} http_response_error;

/**
 * The current state of the state machine when parsing an http request
 * STATE_START prefix is usually used for allocating memory on heap
 * STATE Where the processing of state occurs
 */
typedef enum {
    HTTP_ERROR,
    HTTP_METHOD_START,
    HTTP_METHOD,
    HTTP_PATH_START,
    HTTP_PATH,
    HTTP_VERSION_START,
    HTTP_VERSION,
    HTTP_HEADER_START,
    HTTP_HEADER_FIND_AND_PARSE,
    HTTP_BODY_START, 
    HTTP_BODY,
    HTTP_FINISHED
} http_response_state;

/**
 * Used for storing state of how much of the method, body, version, etc .. was parsed for multiple parse_http_request calls 
 * @see copy_to in simple_http.c 
 */
typedef struct {
    //Where currently parsed data is being stored
    char* store_buf;
    //Max length of store_buf
    uint64_t store_buf_len;
    //Current index to write to
    uint64_t store_index;
    //How much of the delimeter has been found in buf
    uint64_t search_index;
} _copy_state;

/**
 * Where all the parsed http request data is stored
 */
typedef struct {
    char* method;
    char* path;
    char* version;
    headers_t *headers;
    uint8_t *body;
    http_response_state state;
    http_response_error error;
    _copy_state *_internal;
} http_request_t;

/**
 * Allocates memory and configures state for http_request_t
 * @return http_request_t or null if malloc failed
 */
http_request_t* http_request_init();

/**
 * Free's http_request_t even in an error state or an unallocated state
 * @param req http_request_t allocated by http_request_init
 */
void http_request_free(http_request_t* req);

/**
 * Parses buf and stores the parsed data in req
 * @param req http_request_t allocated by http_request_init
 * @param buf A chunk of a ascii buffer to be parsed
 * @param buf_len Length of buf(not including \0)
 */
void parse_http_request(http_request_t *req, const char* buf, uint64_t buf_len);

#endif 
