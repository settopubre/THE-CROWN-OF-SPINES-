#define _POSIX_C_SOURCE 200809L
#include "http_client.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define DEFAULT_TIMEOUT_MS 3000

static int set_socket_timeout(int sockfd, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) return -1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) return -1;
    return 0;
}

HTTPClient* http_client_create(const char* host, int port) {
    if (!host || port <= 0) return NULL;
    
    HTTPClient* client = (HTTPClient*)calloc(1, sizeof(HTTPClient));
    if (!client) return NULL;
    
    client->host = strdup(host);
    client->port = port;
    client->timeout_ms = DEFAULT_TIMEOUT_MS;
    
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        free(client->host);
        free(client);
        return NULL;
    }
    
    client->sockfd = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (client->sockfd < 0) {
        freeaddrinfo(res);
        free(client->host);
        free(client);
        return NULL;
    }
    
    int flag = 1;
    setsockopt(client->sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    /*
     * P0 CRITICAL FIX: IPv4/IPv6 dual-stack safe port assignment.
     *
     * WAS: struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
     *      addr->sin_port = htons(port);
     *
     * BUG: On dual-stack systems, getaddrinfo may return AF_INET6 addresses.
     *      Blindly casting to sockaddr_in* corrupts the IPv6 sin6_addr and
     *      sin6_scope_id fields, causing connect() failures or connecting
     *      to wrong addresses.
     *
     * FIX: Use sockaddr_storage as an intermediate generic container, then
     *      branch on ai_family to set the correct port field for each protocol.
     *      This is the POSIX-recommended pattern for address-family-agnostic
     *      socket programming (RFC 2553 section 3.2).
     */
    struct sockaddr_storage* addr = (struct sockaddr_storage*)res->ai_addr;
    if (addr->ss_family == AF_INET) {
        ((struct sockaddr_in*)addr)->sin_port = htons(port);
    } else if (addr->ss_family == AF_INET6) {
        ((struct sockaddr_in6*)addr)->sin6_port = htons(port);
    }
    /* If AF_UNSPEC returned another family, port assignment is N/A */
    
    if (connect(client->sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        close(client->sockfd);
        freeaddrinfo(res);
        free(client->host);
        free(client);
        return NULL;
    }
    
    freeaddrinfo(res);
    set_socket_timeout(client->sockfd, client->timeout_ms);
    return client;
}

void http_client_destroy(HTTPClient* client) {
    if (!client) return;
    if (client->sockfd >= 0) close(client->sockfd);
    free(client->host);
    free(client);
}

void http_client_set_timeout(HTTPClient* client, int timeout_ms) {
    if (client) {
        client->timeout_ms = timeout_ms;
        set_socket_timeout(client->sockfd, timeout_ms);
    }
}

static int http_request(HTTPClient* client, const char* method, const char* path, 
                        const char* body, char* response, size_t response_size) {
    if (!client || !method || !path) return -1;
    
    char req[8192];
    int len = snprintf(req, sizeof(req), 
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n",
        method, path, client->host);
    
    if (body && strlen(body) > 0) {
        len += snprintf(req + len, sizeof(req) - len, "Content-Length: %zu\r\n\r\n%s", strlen(body), body);
    } else {
        len += snprintf(req + len, sizeof(req) - len, "\r\n");
    }
    
    if (send(client->sockfd, req, len, 0) < 0) return -1;
    
    size_t total = 0;
    ssize_t n;
    while ((n = recv(client->sockfd, client->recv_buf + total, sizeof(client->recv_buf) - total - 1, 0)) > 0) {
        total += n;
        if (total >= sizeof(client->recv_buf) - 1) break;
    }
    client->recv_buf[total] = '\0';
    
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) return -1;
    
    /* Find body after \r\n\r\n */
    char* body_start = strstr(client->recv_buf, "\r\n\r\n");
    if (!body_start) return -1;
    body_start += 4;
    
    if (response && response_size > 0) {
        size_t body_len = strlen(body_start);
        if (body_len >= response_size) body_len = response_size - 1;
        memcpy(response, body_start, body_len);
        response[body_len] = '\0';
    }
    return 0;
}

int http_client_post(HTTPClient* client, const char* path, const char* body, char* response, size_t response_size) {
    return http_request(client, "POST", path, body, response, response_size);
}

int http_client_get(HTTPClient* client, const char* path, char* response, size_t response_size) {
    return http_request(client, "GET", path, NULL, response, response_size);
}
