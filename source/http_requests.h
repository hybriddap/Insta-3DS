// http_requests.h
#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

//includes
#include <3ds.h>

//variables


//methods
Result http_post(const char* url, const char* data);
Result http_download(const char *url);

#endif