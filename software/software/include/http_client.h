#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#pragma once

#include <stddef.h>

typedef struct {
    int sockfd;
    char* host;
    int port;
    char recv_buf[65536];
    int timeout_ms;
} HTTPClient;

#ifdef __cplusplus
extern "C" {
#endif

HTTPClient* http_client_create(const char* host, int port);
void http_client_destroy(HTTPClient* client);
int http_client_post(HTTPClient* client, const char* path, const char* body, char* response, size_t response_size);
int http_client_get(HTTPClient* client, const char* path, char* response, size_t response_size);
void http_client_set_timeout(HTTPClient* client, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_CLIENT_H */
