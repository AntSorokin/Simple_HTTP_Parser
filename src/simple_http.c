#include <string.h>
#include "simple_http.h"

/**
 * Copies buf into c -> store_buf until 'search' is found, 'buf' ends, or 'store_buf' is reached.
 * @param buf buffer to search
 * @param buf_len buffer length
 * @param search search string to find
 * @param search_len length of search string
 * @param c internal state for parsing the buffer for 'search'
 * @param it current index of 'buf'
 * @returns 0 if search string was not found, -1 if store_buf was filled, 1 if search string was found
 */
static int copy_to(const char *buf, uint64_t buf_len, char *search, uint64_t search_len, _copy_state *c, uint64_t *it) {
    //Iterates through buf
    while(*it < buf_len) {
        //checks if character is not matched
        if(buf[*it] != search[c -> search_index]) {
            //Checks for out of bounds
            if(c -> store_index + c -> search_index >= c -> store_buf_len) {
                return -1;
            }
            
            //Copies anything that was partially matched the search string, but not fully matched
            for(uint64_t i = 0; i < c -> search_index; i++) {
                c -> store_buf[c -> store_index++] = search[i];
            }

            c -> search_index = 0;
            //stores the buf
            c -> store_buf[c -> store_index++] = buf[*it];
        }
        else {
            //wont store characters inside store_buf in case store_buf fully matches
            c -> search_index++;
            //if the whole search string is found
            if(c -> search_index == search_len) {
                c -> store_buf_len = c -> store_index + 1;
                return 1;
            }
        }

        (*it)++;
    }

    return 0;
}

/**
 * A copy_to wrapper with error handling
 * @param req
 * @param buf buffer to parse
 * @param buf_len length of 'buf'
 * @param delim delimeter to search for
 * @param delim_len delimeter string length
 * @param dest buffer to copy to everything before delim
 * @param it iterator for buf
 * @param next_state state to transfer to if succesful and delim was found
 * @note can change state to HTTP_ERROR/HTTP_OUT_OF_BOUNDS   
 */
static void copy_to_delim(http_request_t *req, const char *buf, uint64_t buf_len, char *delim, uint64_t delim_len, char **dest, uint64_t *it,  http_response_state next_state) {
    int status = copy_to(buf, buf_len, delim, delim_len, req -> _internal, it);
    
    if(status == 1) {
        *dest = req -> _internal -> store_buf;
        req -> state = next_state;
        (*it)++;
    }
    else if(status == -1) {
        req -> state = HTTP_ERROR;
        req -> error = HTTP_OUT_OF_BOUNDS;
    }
}

/**
 * Resets the _copy_state, allocates new memory to parse next field, and transfers to next state
 * @param req 
 * @param max_store_len max length for store_buf
 * @param next_state state to transfer to 
 * @note can change state to HTTP_ERROR/HTTP_OUT_OF_MEM
 */
static void reset(http_request_t *req, uint64_t max_store_len, http_response_state next_state) {
    req -> _internal -> store_index = 0;
    req -> _internal -> search_index = 0;
    req -> _internal -> store_buf_len = max_store_len;

    req -> _internal -> store_buf = calloc(max_store_len + 1, sizeof(char));

    if(req -> _internal -> store_buf) {
        req -> state = next_state;
    }
    else {
        req -> state = HTTP_ERROR;
        req -> error = HTTP_OUT_OF_MEM;
    }
}

/**
 * Attempts to find a single header and parse it. Stores the header in req -> headers
 * @param req http_request_t
 * @param buf buffer to parse
 * @param buf_len length of buf
 * @param it iterator for buf
 */
static void find_and_parse_header(http_request_t *req, const char *buf, uint64_t buf_len, uint64_t *it) {
    char *unparsed_header;
    //Copies up to \r\n
    copy_to_delim(req, buf, buf_len, "\r\n", 2, &unparsed_header, it, HTTP_HEADER_START);
    //Doesn't start parsing the string until an error or \r\n is found in the buf
    if(req -> state == HTTP_ERROR || req -> state == HTTP_HEADER_FIND_AND_PARSE) {
        return;
    }
    
    //Checks if the end of headers has not been reached meaning buf found \r\n\r\n
    if(req -> _internal -> store_index != 0) {
        char *it_char;
        //Finds everything up to the colon
        char *temp = strtok_r(unparsed_header, ":", &it_char);

        if(!temp) {
            req -> state = HTTP_ERROR;
            req -> error = HTTP_INVALID_HEADER;
            return;
        }
        //no key?
        if(*it_char == '\0') {
            req -> state = HTTP_ERROR;
            req -> error = HTTP_INVALID_HEADER;
            return;
        }
        
        long key_len = strlen(temp);
        if(key_len > HTTP_MAX_HEADER_KEY_SIZE) {
            req -> state = HTTP_ERROR;
            req -> error = HTTP_OUT_OF_BOUNDS;
            return;
        }

        char *key = malloc(key_len + 1);
        if(!key) {
            req -> state = HTTP_ERROR;
            req -> error = HTTP_OUT_OF_MEM;
            return;
        }

        key = strcpy(key, temp);
        //Does not copy all the spaces and tabs before the value
        while((*it_char == ' ' || *it_char == '\t') && *it_char != '\0') {
            it_char++;
        }

        //no value?
        if(*it_char == '\0') {
            free(key);
            req -> state = HTTP_ERROR;
            req -> error = HTTP_INVALID_HEADER;
            return;
        }
        
        long val_len = strlen(it_char); 

        if(val_len > HTTP_MAX_HEADER_VAL_SIZE) {
            free(key);
            req -> state = HTTP_ERROR;
            req -> error = HTTP_OUT_OF_BOUNDS;
            return;
        }
        char *val = malloc(val_len + 1);

        if(!val) {
            req -> state = HTTP_ERROR;
            req -> error = HTTP_OUT_OF_MEM;
            return;
        }

        strcpy(val, it_char);
        headers_state ht = add_header(req -> headers, key, val);
        if(ht == HEADERS_OUT_OF_MEM) {
            free(key);
            free(val);
            req -> state = HTTP_ERROR;
            req -> error = HTTP_OUT_OF_MEM;
            return;
        }
        //If max header count was reached
        else if(ht == HEADERS_OUT_OF_BOUNDS) {
            free(key);
            free(val);
            req -> state = HTTP_ERROR;
            req -> error = HTTP_OUT_OF_BOUNDS;
            return;
        }
    }
    else {
        //TODO FUTURE: Add Chunked transfer
        char *content_len_str = get_header(req -> headers, "Content-Length", 0);
        //Will not attempt to parse body unless Content-Length is found with a non zero value
        if(content_len_str == 0 || strcmp(content_len_str, "0") == 0) {
            req -> state = HTTP_FINISHED;
            return;
        }

        req -> state = HTTP_BODY_START;
    }
    //the default next state is HTTP_HEADER_START as set by copy_delim
    free(unparsed_header);
    req -> _internal-> store_buf = 0;
}

/**
 * Allocates memory for body based on the Content-Length header
 * @param req http_request_t to store body
 */
static void allocate_body(http_request_t *req) {
    //Content-Length exists as checked in find_and_parse_header
    char *content_len_str = get_header(req -> headers, "Content-Length", 0);

    uint64_t content_len = 0;

    // converting string to number
    for (uint64_t i = 0; content_len_str[i] != '\0'; i++) {
        if(content_len_str[i] >= 48 && content_len_str[i] <= 57) {
            content_len = content_len * 10 + (content_len_str[i] - 48);
        }
        else {
            req -> state = HTTP_ERROR;
            req -> error = HTTP_INVALID_HEADER;
            return;
        }  
    }

    if(content_len > HTTP_MAX_BODY_SIZE) {
        req -> state = HTTP_ERROR;
        req -> error = HTTP_OUT_OF_BOUNDS;
        return;
    }

    if(content_len <= 0) {
        req -> state = HTTP_FINISHED;
        return;
    }

    reset(req, content_len, HTTP_BODY);
}

/**
 * Copies body from buf based on how much memory was allocated by allocate_body
 */
static void copy_body(http_request_t *req, const char *buf, uint64_t buf_len, uint64_t *it) {
    while(*it < buf_len) {
        req -> _internal -> store_buf[req -> _internal -> store_index++] = buf[*it];
        //If body was fully copied
        if(req -> _internal -> store_index == req -> _internal -> store_buf_len) {

            req -> body = req -> _internal -> store_buf;
            req -> state = HTTP_FINISHED;
            return;
        }

        (*it)++;
    }
}

http_request_t* http_request_init() {
    http_request_t *temp = calloc(1, sizeof(http_request_t));

    if(!temp) {
        return NULL;
    }
    temp -> headers = headers_init(HTTP_MAX_HEADERS);

    if(!temp -> headers) {
        free(temp);
        return NULL;
    }

    temp -> _internal = calloc(1, sizeof(_copy_state));

    if(!temp -> _internal) {
        free(temp -> headers);
        free(temp);
        return NULL;
    }

    temp -> state = HTTP_METHOD_START;

    return temp;
}

void parse_http_request(http_request_t *req, const char* buf, uint64_t buf_len) {
    uint64_t i = 0;
    while(i < buf_len) {
        switch(req -> state) {
            case HTTP_METHOD_START:
                reset(req, 8, HTTP_METHOD);
                break;
            case HTTP_METHOD:
                copy_to_delim(req, buf, buf_len, " ", 1, &(req -> method), &i, HTTP_PATH_START);
                break;
            case HTTP_PATH_START:
                reset(req, HTTP_MAX_PATH_SIZE, HTTP_PATH);
                break;
            case HTTP_PATH:
                copy_to_delim(req, buf, buf_len, " ", 1, &(req -> path), &i, HTTP_VERSION_START);
                break;
            case HTTP_VERSION_START:
                reset(req, 8, HTTP_VERSION);
                break;
            case HTTP_VERSION:
                copy_to_delim(req, buf, buf_len, "\r\n", 2, &(req -> version), &i, HTTP_HEADER_START);
                break;
            case HTTP_HEADER_START:
                reset(req, HTTP_MAX_HEADER_KEY_SIZE + 1 + HTTP_MAX_HEADER_VAL_SIZE, HTTP_HEADER_FIND_AND_PARSE);
                break;
            case HTTP_HEADER_FIND_AND_PARSE:
                find_and_parse_header(req, buf, buf_len, &i);
                break;
            case HTTP_BODY_START:
                allocate_body(req);
                break;
            case HTTP_BODY:
                copy_body(req, buf, buf_len, &i);
                break;
            default:
                //in case of HTTP_ERROR or HTTP_FINISHED
                return;
        }
    }
}

void http_request_free(http_request_t* req) {
    if(req) {
        if(req -> method) {
            free(req -> method);
        }

        if(req -> path) {
            free(req -> path);
        }

        if(req -> version) {
            free(req -> version);
        }

        if(req -> headers) {
            headers_free(req -> headers);
        }

        if(req -> _internal -> store_buf) {
            free(req -> _internal -> store_buf);
        }

        if(req -> _internal) {
            free(req -> _internal);
        }

        free(req);
    }
}