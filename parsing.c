/**
 * @file proxy.c
 * @author Subin Song (lemonshushuu@gmail.com)
 * @brief Proxy server
 * @version 0.1
 * @date 2022-05-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdio.h>
// #include "csapp.h"
#include <string.h>
#include <stdlib.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_HEADERS 100
#define MAXLINE 1000

/* You won't lose style points for including this long line in your code */
static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct
{
    char name[MAXLINE];
    char value[MAXLINE];
} header_t;
typedef struct
{
    char method[10];
    char url[MAXLINE];
    char hostname[MAXLINE];
    char port[10];
    char path[MAXLINE];
    char version[20];
    int num_headers;
    header_t headers[MAX_HEADERS];
} Request;

void *handle_client(void *vargp);
void initialize_struct(Request *req);
void parse_request(char request[MAXLINE], Request *req);
void parse_absolute(Request *req);
void parse_relative(Request *req);
void parse_header(char header[MAXLINE], Request *req);
void add_headers(Request *req);
void assemble_request(Request *req, char *request);
int get_from_cache(Request *req, int clientfd);
// void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client);
void close_wrapper(int fd);
void print_full(char *string);
void print_struct(Request *req);

int main(int argc, char **argv)
{

    char request[] = "GET /index.html HTTP/1.1\r\nHost: www.google.com\r\nConnection: keep-alive\r\n\r\n";

    // parse the request
    Request req;
    initialize_struct(&req);
    parse_request(request, &req);
    print_struct(&req); // before
    add_headers(&req);
    print_struct(&req); // after
    char assembled[1000];
    assemble_request(&req, assembled);
}

void initialize_struct(Request *req)
{
    strcpy(req->method, "");
    strcpy(req->url, "");
    strcpy(req->hostname, "");
    strcpy(req->port, "80");
    strcpy(req->path, "");
    strcpy(req->version, "");
    req->num_headers = 0;
}
void parse_request(char request[MAXLINE], Request *req)
{
    char *token;
    char *saveptr;
    char *line = strdup(request);
    token = strtok_r(line, " ", &saveptr);
    strcpy(req->method, token);
    if (strcmp(req->method, "GET") != 0) // only support GET
    {
        free(line);
        return;
    }
    token = strtok_r(NULL, " ", &saveptr);
    strcpy(req->url, token);

    token = strtok_r(NULL, "\r\n", &saveptr);
    strcpy(req->version, token);
    if (strncmp(req->url, "http://", 7) == 0)
    {
        parse_absolute(req);
    }
    else
    {
        parse_relative(req);
    }

    token = strtok_r(NULL, "\r\n", &saveptr);
    while (token != NULL)
    {
        parse_header(token, req);
        token = strtok_r(NULL, "\r\n", &saveptr);
    }
    free(line);
}
void parse_absolute(Request *req)
{
    char *token;
    char *saveptr;
    char *line = strdup(req->url + 7);
    if (strchr(line, ':') != NULL) // has port number
    {
        token = strtok_r(line, ":", &saveptr);
        strcpy(req->hostname, token);
        token = strtok_r(NULL, "/", &saveptr);
        strcpy(req->port, token);
        strcpy(req->path, saveptr);
    }
    else // doesn't have port number
    {
        token = strtok_r(line, "/", &saveptr);
        strcpy(req->hostname, token);
        strcpy(req->path, saveptr);
    }
    free(line);
}
void parse_relative(Request *req)
{
    char *token;
    char *saveptr;
    char *line = strdup(req->url);
    token = strtok_r(line, "/", &saveptr);
    strcpy(req->path, token);
    free(line);
}
void parse_header(char header[MAXLINE], Request *req)
{
    char *token;
    char *saveptr;
    char *line = strdup(header);
    token = strtok_r(line, ": ", &saveptr);
    if (strcmp(token, "User-Agent") == 0 || strcmp(token, "Connection") == 0 || strcmp(token, "Proxy-Connection") == 0)
        return;
    strcpy(req->headers[req->num_headers].name, token);
    strcpy(req->headers[req->num_headers].value, saveptr);
    req->num_headers++;
    free(line);
}

void add_headers(Request *req)
{
    int host_header_exists = 0;
    for (int i = 0; i < req->num_headers; i++)
    {
        if (strcmp(req->headers[i].name, "Host") == 0)
        {
            host_header_exists = 1;
            break;
        }
    }
    if (!host_header_exists)
    {
        strcpy(req->headers[req->num_headers].name, "Host");
        strcpy(req->headers[req->num_headers].value, req->hostname);
        req->num_headers++;
    }
    strcpy(req->headers[req->num_headers].name, "User-Agent");
    strcpy(req->headers[req->num_headers].value, user_agent);
    req->num_headers++;
    strcpy(req->headers[req->num_headers].name, "Connection");
    strcpy(req->headers[req->num_headers].value, "close");
    req->num_headers++;
    strcpy(req->headers[req->num_headers].name, "Proxy-Connection");
    strcpy(req->headers[req->num_headers].value, "close");
    req->num_headers++;
}

void assemble_request(Request *req, char *request)
{
    strcpy(request, req->method);
    strcat(request, " /");
    strcat(request, req->path);
    strcat(request, " ");
    strcat(request, "HTTP/1.0\r\n");
    int i;
    for (i = 0; i < req->num_headers; i++)
    {
        strcat(request, req->headers[i].name);
        strcat(request, ": ");
        strcat(request, req->headers[i].value);
        strcat(request, "\r\n");
    }
    strcat(request, "\r\n");
}
void print_full(char *string)
{
    printf("%s\n", string);
}
void print_struct(Request *req)
{
    printf("Method: %s\n", req->method == NULL ? "NULL" : req->method);
    printf("URL: %s\n", req->url == NULL ? "NULL" : req->url);
    printf("Hostname: %s\n", req->hostname == NULL ? "NULL" : req->hostname);
    printf("Port: %s\n", req->port == NULL ? "NULL" : req->port);
    printf("Path: %s\n", req->path == NULL ? "NULL" : req->path);
    printf("Version: %s\n", req->version == NULL ? "NULL" : req->version);
    for (int i = 0; i < req->num_headers; i++)
    {
        printf("Header %d: %s: %s\n", i, req->headers[i].name, req->headers[i].value);
    }
}
