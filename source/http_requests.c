#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <3ds.h>

Result upload_ppm_file(const char *url, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open file: %s\n", filename);
        return -1;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read file into buffer
    unsigned char *filedata = malloc(filesize);
    if (!filedata) {
        fclose(fp);
        printf("Memory alloc failed\n");
        return -1;
    }

    fread(filedata, 1, filesize, fp);
    fclose(fp);

    // === HTTP upload ===
    Result ret=0;
    httpcContext context;
    u32 statuscode=0;
	u8 *response = NULL;
	u32 readsize = 0, size = 0;

    printf("Uploading %s (%ld bytes)\n", filename, filesize);

    ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);
    if (ret) return ret;

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    httpcAddRequestHeaderField(&context, "User-Agent", "Insta-3DS/1.0");
    httpcAddRequestHeaderField(&context, "Content-Type", "application/octet-stream");

    // Send raw binary data
    ret = httpcAddPostDataRaw(&context, (u32*)filedata, (u32)filesize);
    if (ret) {
        printf("httpcAddPostDataRaw failed: %lx\n", ret);
        free(filedata);
        return ret;
    }

    ret = httpcBeginRequest(&context);
    if (ret) {
        printf("Begin request failed\n");
        httpcCloseContext(&context);
        free(filedata);
        return ret;
    }

    ret = httpcGetResponseStatusCode(&context, &statuscode);
    //printf("Server returned %lu\n", statuscode);


	// --- Read the response body ---
    response = malloc(0x1000);
    if (!response) {
        httpcCloseContext(&context);
        return -1;
    }

    do {
        ret = httpcDownloadData(&context, response + size, 0x1000, &readsize);
        size += readsize;
        if (ret == HTTPC_RESULTCODE_DOWNLOADPENDING) {
            response = realloc(response, size + 0x1000);
        }
    } while (ret == HTTPC_RESULTCODE_DOWNLOADPENDING);

    response[size] = '\0'; // null terminate (safe if text)
    printf("%s\n", response);

    httpcCloseContext(&context);
    free(filedata);
    return ret;
}

Result upload_post_data(const char* url)
{
    Result ret;
    httpcContext context;
    u32 statuscode;

    const char* token = "DapsToken";
    const char* caption = "Test from my #3ds";
    const char* image_url = "url";

    char json_body[512];
    snprintf(json_body, sizeof(json_body),
             "{\"token\": \"%s\", \"caption\": \"%s\", \"image_url\": \"%s\"}",
             token, caption, image_url);

    printf("Uploading to meta...\n");

    // --- HTTP setup ---
    ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);
    if (ret) return ret;

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);

    // Headers
    httpcAddRequestHeaderField(&context, "User-Agent", "Insta-3DS/1.0");
    httpcAddRequestHeaderField(&context, "Content-Type", "application/json");

    // Body (send JSON string)
    ret = httpcAddPostDataRaw(&context, (u32*)json_body, strlen(json_body));
    if (ret) return ret;

    // Send request
    ret = httpcBeginRequest(&context);
    if (ret) {
        httpcCloseContext(&context);
        return ret;
    }

    // Check status code
    httpcGetResponseStatusCode(&context, &statuscode);
    printf("Response status: %lu\n", statuscode);

    // --- Read response ---
    u8* response =(u8*)malloc(0x1000);
    u32 readsize = 0, size = 0;

    do {
        ret = httpcDownloadData(&context, response + size, 0x1000, &readsize);
        size += readsize;
        if (ret == HTTPC_RESULTCODE_DOWNLOADPENDING)
            response = realloc(response, size + 0x1000);
    } while (ret == HTTPC_RESULTCODE_DOWNLOADPENDING);

    response[size] = '\0';
    printf("%s\n", response);

    free(response);
    httpcCloseContext(&context);
    return 0;
}