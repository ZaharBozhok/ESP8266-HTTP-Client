#define ENDL "\r\n"
#define GET "GET "
#define POST "POST "
#define HTTP_HOST " HTTP/1.0" ENDL "Host: "
#define CONTENT ENDL "Content-Type: application/x-www-form-urlencoded" ENDL "Content-Length: "
#define STR_SIZE(X) (sizeof(X) - 1 )

void free_request(char **request);
char * init_request(char **request,char *uri,const char *params);
