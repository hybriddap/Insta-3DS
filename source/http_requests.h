// http_requests.h
#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

//includes
#include <3ds.h>

//variables


//methods
Result upload_ppm_file(const char *url, const char *filename,char** responseBuf);
Result upload_post_data(const char* url,const char* token, const char* caption, const char* image_url);
Result http_post(const char* url, const char* data);
Result http_download(const char *url);

#endif