# Simple_HTTP_Parser
A simple HTTP/1.1 library used for parsing requests over TCP Sockets. 
See tests/ for examples of how to use Simple_HTTP, along with error handling. 

## Installation
1. git clone https://github.com/seamoose44499958/Simple_HTTP_Parser.git
2. Add add_subdirectory(DIRECTORY_FOR_CLONED_PROJECT) inside CMakeLists.txt
3. Add target_link_libraries(YOUR_EXECUTABLE SIMPLE_HTTP) to link library
4. Add #include "simple_http.h" in your .h or .c files

## Max Size Macros
Simple_HTTP contains macros that contain limits to how long a field(of ascii characters) can be.
* **HTTP_MAX_BODY_SIZE**(Default: 2048): The max body size that can be stored.
* **HTTP_MAX_HEADER_KEY_SIZE**(Default: 255): The max size a header key can be.
* **HTTP_MAX_HEADER_VAL_SIZE**(Default: 512): The max size a header value can be(anything after the ':').
* **HTTP_MAX_PATH_SIZE**(Default: 30): The max size the path field can be.
* **HTTP_MAX_HEADERS**(Default: 30): The maximum amount of headers the request can contain(note that headers with repeating keys are counted torwards the total).

These macros can be reconfigured through CMAKE(OPTION command) or by specifying the macro before the include. 
For both the http method and version the max size is 8, these can not be configured.

## How It Works
Behind the scenes, http_request_t is a giant state machine which switches states based on what has already been parsed. This allows for the method to 
parse the request in chunks as it comes over the TCP socket. Eventually, the parser will either finish in an error state or the FINISHED state once the whole
message has been parsed(assuming it was fully transfered).

## HTTP Parse States 
Found in http_request -> state
* **HTTP_ERROR**: If something went wrong the string will not be parsed any further
* **HTTP_METHOD_START**: Allocation of memory for the method C string
* **HTTP_METHOD**: Where the method is copied to the method C string
* **HTTP_PATH_START**: Allocation of memory for the path C string
* **HTTP_PATH**: Where the path is copied to the path C string
* **HTTP_VERSION_START**: Allocation of memory for the version C string
* **HTTP_VERSION**: Where the version is copied to the version C string 
* **HTTP_HEADER_START**: Allocation for where to store the key and value of a header
* **HTTP_HEADER_FIND_AND_PARSE**, Parses header and stores it in the headers data structure. State will move back to HTTP_HEADER_START if there are more headers.
* **HTTP_BODY_START**: Allocation for the body C string
* **HTTP_BODY**: Where the body is copied to the body C string
* **HTTP_FINISHED**: The HTTP Request has been parsed succesfully

## HTTP Parse Error States
Found in http_request -> error
* **HTTP_OK**(Default state): No errors have occured
* **HTTP_OUT_OF_MEM**: HTTP Parse could not allocate memory with malloc or calloc
* **HTTP_OUT_OF_BOUNDS**: Http field exceeded the max character count listed by the Max Size Macros
* **HTTP_INVALID_HEADER**: Occurs when the header was not formatted correctly(see tests/ for examples)

## HTTP Parse Type(http_request_t)
* **method**: Stores method C string
* **path**: Stores path C string
* **version**: Stores version C string
* **headers**: Where headers are stored(more details in HTTP Parse Headers)
* **body**: Stores body C uint8_t array(of size Content-Length + 1(stores a 0))
* **state**: Current parse state of the request
* **error**: Current error state
* **_internal**: Responsible for storing state of the field being copied and parsed between request chunks

## HTTP Parse Headers(headers_t) Functions:
Headers are stored in the headers_t data structure which behind the scenes is a hashmap of a C string and a dynamically resizable array storing a C string.
The key of the hashmap stores the key of the header, the value of the hashmap stores an array which stores header values with the same header key.
Example of key with multiple values:
*Set-Cookie: cookie1\r\n*
*Set-Cookie: cookie2\r\n*

**get_last_header(headers_t *headers, char *key)**: Gets the last added value with the header key 'key'. Returns the value, or 0 if not found <br>
**get_header(headers_t *headers, char *key, uint64_t val_index)**: Gets the 'val_index' added value with the header key 'key'. Returns value ,or 0 if not found

## HTTP Parse Type(http_request_t) Functions:
**http_request_init()**: Allocates memory for http_request_t <br>
**http_request_free(http_request_t\* req)**: Deallocates memory for http_request_t <br>
**parse_http_request(http_request_t *req, const char *buf, uint64__t buf_len)**: Parses 'buf'(ascii) of length 'buf_len' and stores parsed data in http_request_t 

## Considerations and Non Compliance with HTTP/1.1 standard:
Currently the http parser only supports parsing a body that is specified by a Content-Length(Chuncked Transfer Encoding is currently not supported and will be not parsed). 
HTTP Parser will not error if the body sent through the connection is larger than the Content-Length specified(unless the content length is greater than HTTP_MAX_BODY_SIZE).
None of the stored data is validated(for example the method can have GTE instead of GET). "Content-Length" must be sent by the client with both the C and L being capital for the 
body to be parsed, else it will not be parsed. Header values are not parsed any further than just copying the string. Body will be copied as long as there is a Content-Length header
with a value greater than zero, even if the method is GET. 

## Bug Report
Code has been tested with the following tests and valgrind to check for memory leaks. If leaks or bugs are found please share.

## Example 
```C
  http_request_t *req = http_request_init();

   // The whole HTTP Request
   char *req_str = "POST /test_path/1 HTTP/1.1\r\nAccept: text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, ;q=0.8\r\nCookie: PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1\r\nContent-Length: 4\r\n\r\ntest--";

   //Notice request can be parsed in chunks as provided by packets from a TCP socket
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

   if(req -> state == FINISHED) {
      //The request is fully parsed(The result of running this code)
   }
   if(req -> state == ERROR) {
      req -> error; //Gives an error state if an error occured such as out of memory
   }
   // C string of http fields is stored in req -> method, req -> path, req -> version, req -> body, make sure to check for null
   char *path = req -> path;

   //Get header using either get_last_header() or get_header()
   char *cookie = get_last_header(req -> headers, "Cookie");
   //Cookie will store "PHPSESSID=298zf09hf012fh2; csrftoken=u32t4o3tb3gg43; _gat=1"
   http_request_free(req);
```

