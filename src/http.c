#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "http.h"

#define BUF_SIZE 2048


/*
 * create_buffer:
 *
 * Allocates memory for and returns a buffer to be used to hold the downloaded
 * HTTP response
 */
Buffer *create_buffer() {
    Buffer* b = (Buffer*) malloc(sizeof(Buffer));
    b->data = (char*) malloc(BUF_SIZE);
    b->length = 0;
    return b;
}

/*
 * build_request:
 *
 * Forms and returns the actual HTTP request string for a GET request from the
 * specified host and page.
 *
 */
char *build_request(char *host, char *page) {
    char *req_format = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: getter\r\n\r\n";
    int req_len = strlen(req_format) + strlen(page) + strlen(host);
    char *request = (char*) malloc(req_len);

    snprintf(request, req_len, req_format, page, host);
    return request;
}

/*
 * addrinfo_init:
 *
 * Essentially performs all of the boilerplate (specifies connection protocol etc...)
 * Returns a structure containing required information about the host.
 */
struct addrinfo *addrinfo_init(char *host, char *page, int port) {
    int status;
    char port_string[4];
    sprintf(port_string, "%d", port); //convert port number to string
    struct addrinfo info, *their_info;

    memset(&info, 0, sizeof(info));
	info.ai_family = AF_INET; //Use IPv4
	info.ai_socktype = SOCK_STREAM; //Select protocol as TCP

    if ((status = getaddrinfo(host, port_string, &info, &their_info)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);

    } return their_info;
}

/*
 * get_response:
 *
 * Receives data through the socket and into the provided buffer, one chunk at a time.
 * The buffer is dynamically reallocated to facilitate downloads of an arbitrary
 * size.
 */
void get_response(Buffer* buf, int sock) {
    int n_bytes_received;

    while ((n_bytes_received = recv(sock, buf->data + buf->length, BUF_SIZE, 0)) > 0) {
        buf->length += n_bytes_received;
        buf->data = realloc(buf->data, buf->length + BUF_SIZE);

    } buf->data[buf->length] = '\0';
}

/*
 * http_query:
 *
 * Initializes the socket and connection, sends the required HTTP request and
 * returns a buffer holding the downloaded response.
 */
Buffer *http_query(char *host, char *page, int port) {
    int sock;
    char *request = build_request(host, page);
    struct addrinfo *their_info = addrinfo_init(host, page, port);
    Buffer* buf = create_buffer(); //allocate memory to hold the response

    sock = socket(their_info->ai_family, their_info->ai_socktype, their_info->ai_protocol);

    connect(sock, their_info->ai_addr, their_info->ai_addrlen);
    send(sock, request, strlen(request), 0); //perform the HTTP request
    get_response(buf, sock); //download the response into the buffer

    close(sock);
    free(request);
	freeaddrinfo(their_info);
    return buf;
}

// split http content from the response string
char *http_get_content(Buffer *response) {
    char *header_end = strstr(response->data, "\r\n\r\n");

    if (header_end) {
        return header_end + 4;

    } else {
        return response->data;
    }
}


Buffer *http_url(const char *url) {
    char host[BUF_SIZE];
    strncpy(host, url, BUF_SIZE);
    char *page = strstr(host, "/");

    if (page) {
        page[0] = '\0';

        ++page;
        return http_query(host, page, 80);

    } else {
        fprintf(stderr, "could not split url into host/page %s\n", url);
        return NULL;
    }
}
