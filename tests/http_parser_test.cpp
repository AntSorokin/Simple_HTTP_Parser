#include <catch2/catch_test_macros.hpp>

extern "C" {
    #include <string.h>
    #include "simple_http.h"
}

TEST_CASE("MINIMAL REQUEST") {
   http_request_t *req = http_request_init();
   
   char *req_str = "GET /test_path/1 HTTP/1.1\r\n\r\n";
   parse_http_request(req, req_str, strlen(req_str));

   REQUIRE(req -> state == HTTP_FINISHED);
   REQUIRE(strcmp(req -> method, "GET") == 0);
   REQUIRE(strcmp(req -> path, "/test_path/1") == 0);
   REQUIRE(strcmp(req -> version, "HTTP/1.1") == 0);

   http_request_free(req);
}

// Example of how headers are parsed
TEST_CASE("HEADERS AND BODY") {
   http_request_t *req = http_request_init();
   
   char *req_str = "POST /test_path/1 HTTP/1.1\r\nAccept: text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, ;q=0.8\r\nCookie: PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1\r\nContent-Length: 4\r\n\r\ntest";
   parse_http_request(req, req_str, strlen(req_str));

   REQUIRE(req -> state == HTTP_FINISHED);
   REQUIRE(strcmp(req -> method, "POST") == 0);
   REQUIRE(strcmp(req -> path, "/test_path/1") == 0);
   REQUIRE(strcmp(req -> version, "HTTP/1.1") == 0);

   char *accept = get_last_header(req -> headers, "Accept");
   REQUIRE(strcmp(accept, "text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, ;q=0.8") == 0);

   char *cookie = get_last_header(req -> headers, "Cookie");
   REQUIRE(strcmp(cookie, "PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1") == 0);

   char *content_len = get_last_header(req -> headers, "Content-Length");
   REQUIRE(strcmp(content_len, "4") == 0);

   REQUIRE(strcmp((char*)req -> body, "test") == 0);

   http_request_free(req);
}

//Shows how a broken up http message can be parsed
TEST_CASE("PARSING CHUNCKS") {
    http_request_t *req = http_request_init();
   
   char *req_str = "POST /test_path/1 HTTP/1.1\r\nAccept: text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, ;q=0.8\r\nCookie: PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1\r\nContent-Length: 4\r\n\r\ntest";
   parse_http_request(req, "PO", 2);
   parse_http_request(req, "ST /test_path/1 H", 17);
   parse_http_request(req, "TTP/1.1\r", 8);
   parse_http_request(req, "\nAccept: text/html, application/xht", 35);
   parse_http_request(req, "ml+xml, application/xml;q=0.9, image/webp, ;q=0.8", 49);
   parse_http_request(req, "\r\n", 2);
   parse_http_request(req, "Cookie: PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1\r\nContent-Length: 4\r\n\r", 89);
   parse_http_request(req, "\nte", 3);
   //Notice anything after content length is cut off
   parse_http_request(req, "st--", 4);

   REQUIRE(req -> state == HTTP_FINISHED);
   REQUIRE(strcmp(req -> method, "POST") == 0);
   REQUIRE(strcmp(req -> path, "/test_path/1") == 0);
   REQUIRE(strcmp(req -> version, "HTTP/1.1") == 0);

   char *accept = get_last_header(req -> headers, "Accept");
   REQUIRE(strcmp(accept, "text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, ;q=0.8") == 0);

   char *cookie = get_last_header(req -> headers, "Cookie");
   REQUIRE(strcmp(cookie, "PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1") == 0);

   char *content_len = get_last_header(req -> headers, "Content-Length");
   REQUIRE(strcmp(content_len, "4") == 0);

   REQUIRE(strcmp((char*)req -> body, "test") == 0);

   http_request_free(req);
}

//The method can't be longer than 8 characters, 
TEST_CASE("OUT OF BOUNDS -> METHOD") {
    http_request_t *req = http_request_init();

    char *req_str = "GETTTTTTT /test HTTP/1.1\r\n\r\n";
    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}

//The method can't be longer than HTTP_MAX_PATH_SIZE characters which by default is set to 30, 
TEST_CASE("OUT OF BOUNDS -> PATH") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /testfkldslkfn123sklfnsfkn13sn3 HTTP/1.1\r\n\r\n";
    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}

//The amount of headers can't be greater than HTTP_MAX_HEADERS which by default is set to 30
TEST_CASE("OUT OF BOUNDS -> HEADER COUNT") {
    http_request_t *req = http_request_init();

    char *req_str = "GET /test HTTP/1.1\r\n0: 0\r\n1: 1\r\n2: 2\r\n3: 3\r\n4: 4\r\n5: 5\r\n6: 6\r\n7: 7\r\n8: 8\r\n9: 9\r\n10: 10\r\n11: 11\r\n12: 12\r\n13: 13\r\n14: 14\r\n15: 15\r\n16: 16\r\n17: 17\r\n18: 18\r\n19: 19\r\n20: 20\r\n21: 21\r\n22: 22\r\n23: 23\r\n24: 24\r\n25: 25\r\n26: 26\r\n27: 27\r\n28: 28\r\n29: 29\r\n30: 30\r\n\r\n\r\n";
    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}

//For headers the max size is HTTP_MAX_HEADER_KEY_SIZE + 1 + HTTP_MAX_HEADER_VAL_SIZE, 
//The default for HTTP_MAX_HEADER_KEY_SIZE is 255 and HTTP_MAX_HEADER_VAL_SIZE is 512(+ 1 is for the colon)  
//Any spaces or \t before the value are counted as part of the header size
TEST_CASE("OUT OF BOUNDS -> KEY/VAL") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\nBREAK-MAX: 123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234\r\n\r\n";
    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}

TEST_CASE("OUT OF BOUNDS -> KEY") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\nBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAX: 1234\r\n\r\n";
    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}

TEST_CASE("OUT OF BOUNDS -> VAL") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\nBreak: -MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAX1234-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAXBREAK-MAX1234\r\n\r\n";
    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}

TEST_CASE("INVALID HEADER -> MISSING :") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\nTEST=VAL\r\n\r\n";

    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_INVALID_HEADER);

    http_request_free(req);
}

TEST_CASE("INVALID HEADER -> ONLY :") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\n: \r\n\r\n";

    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_INVALID_HEADER);

    http_request_free(req);
}


TEST_CASE("INVALID HEADER -> MISSING KEY") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\n:VAL\r\n\r\n";

    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_INVALID_HEADER);

    http_request_free(req);
}

TEST_CASE("INVALID HEADER -> MISSING VAL") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\nKEY:\r\n\r\n";

    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_INVALID_HEADER);

    http_request_free(req);
}

//Max body size is set by HTTP_MAX_BODY_SIZE and is 2048
TEST_CASE("OUT OF BOUNDS -> BODY") {
    http_request_t *req = http_request_init();

    char *req_str = "PUT /test1 HTTP/1.1\r\nKEY:VAL\r\nContent-Length: 2050\r\n\r\ntestbody";

    parse_http_request(req, req_str, strlen(req_str));

    REQUIRE(req -> state == HTTP_ERROR);
    REQUIRE(req -> error == HTTP_OUT_OF_BOUNDS);

    http_request_free(req);
}