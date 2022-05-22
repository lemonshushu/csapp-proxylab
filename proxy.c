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
#include <string.h>
#include <stdlib.h>
#include "csapp.h"
#include <time.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_HEADERS 100

/* You won't lose style points for including this long line in your code */
static const char *user_agent = "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";

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
void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client);
void close_wrapper(int fd);
void print_full(char *string);
void print_struct(Request *req);

typedef struct CachedItem CachedItem;

struct CachedItem
{
    char *url;
    char *item;
    int size;
    CachedItem *prev;
    CachedItem *next;
};

/**
 * @brief Ordered by most recent
 *
 */
typedef struct
{
    CachedItem *head;
    CachedItem *tail;
    int size;
} CacheList;

extern void cache_init(CacheList *list);
extern void cache_URL(char *URL, void *item, size_t size, CacheList *list);
extern void evict(CacheList *list);
extern CachedItem *find(char *URL, CacheList *list);
extern void move_to_front(char *URL, CacheList *list);
extern void print_URLs(CacheList *list);
extern void cache_destruct(CacheList *list);
void cache_insert(char *URL, void *item, size_t size, CacheList *list);
char *get_from_cache_helper(char *key);

CacheList *cache;

int main(int argc, char **argv)
{
    cache = (CacheList *)malloc(sizeof(CacheList));
    cache_init(cache);
    int listenfd, *connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough space for any address */
    pthread_t tid;

    if (argc != 2)
    {
        printf("usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, handle_client, connfd);
    }
    printf("%s", user_agent);
    cache_destruct(cache);
    return 0;
}

void *handle_client(void *vargp)
{
    int clientfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    char request[MAXLINE];
    rio_t rio_to_client;
    rio_readinitb(&rio_to_client, clientfd);

    // read the request
    rio_readlineb(&rio_to_client, request, MAXLINE);

    // parse the request
    Request req;
    initialize_struct(&req);
    parse_request(request, &req);
    if (strcmp(req.method, "GET") != 0) // Only support get
    {
        printf("%s", user_agent);
        printf("%s", request);
        printf("%s", "HTTP/1.0 501 Not Implemented\r\n");
        printf("%s", "Content-type: text/html\r\n\r\n");
        printf("%s", "<html><head><title>Not Implemented</title></head>");
        printf("%s", "<body><p>HTTP request method not supported.</p></body></html>");
        close_wrapper(clientfd);
        return NULL;
    }
    add_headers(&req);
    print_struct(&req); // after

    // check if the request is in the cache
    // int in_cache = 0;
    int in_cache = get_from_cache(&req, clientfd);
    if (in_cache == 1)
    {
        printf("In cache\n");
    }
    else
    {
        printf("Not in cache\n");
        get_from_server(&req, request, clientfd, rio_to_client);
    }
    print_URLs(cache);
    close_wrapper(clientfd);
    return NULL;
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
    if (strcmp(token, "Host") == 0 || strcmp(token, "User-Agent") == 0 || strcmp(token, "Connection") == 0 || strcmp(token, "Proxy-Connection") == 0)
    {
        free(line);
        return;
    }
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
        if (strlen(req->port) == 0)
            sprintf(req->headers[req->num_headers].value, "%s", req->hostname);
        else
            sprintf(req->headers[req->num_headers].value, "%s:%s", req->hostname, req->port);
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

int get_from_cache(Request *req, int clientfd)
{
    char *key = req->url;
    char *value = get_from_cache_helper(key);
    if (value == NULL)
        return 0;
    else
    {
        printf("Found in cache\n");
        move_to_front(key, cache);
        Rio_writen(clientfd, value, strlen(value));
        return 1;
    }
}

/**
 * @brief: get the value of the key(url) from cache using find() function
 * @param key: the key(url) to be searched
 * @return: the value of the key if found, NULL otherwise
 */
char *get_from_cache_helper(char *key)
{
    CachedItem *item = find(key, cache);
    if (item == NULL)
        return NULL;
    else
        return item->item;
}

/**
 * @brief Get the from server object
 *
 * @param req The request object
 * @param request The request string
 * @param clientfd The client file descriptor
 * @param rio_to_client The rio object to the client
 */
void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client)
{
    size_t n;
    int serverfd;
    char buf[MAXLINE];
    rio_t rio_to_server;

    char *hostname = req->hostname;
    char *port = req->port;
    serverfd = Open_clientfd(hostname, port);

    Rio_readinitb(&rio_to_server, serverfd);
    assemble_request(req, request);
    printf("%s", request);
    Rio_writen(serverfd, request, strlen(request));
    void *full_response = malloc(MAX_OBJECT_SIZE);
    int full_response_size = 0;
    while ((n = Rio_readlineb(&rio_to_server, buf, MAXLINE)) != 0)
    {
        if (full_response_size + n <= MAX_OBJECT_SIZE)
        {
            memcpy(full_response + full_response_size, buf, n);
            full_response_size += n;
        }

        Rio_writen(clientfd, buf, n);
    }
    if (full_response_size <= MAX_OBJECT_SIZE)
    {
        cache_URL(req->url, full_response, strlen(full_response), cache);
    }
    free(full_response);
    Close(serverfd);
}
void close_wrapper(int fd)
{
    Close(fd);
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
    printf("\n");
}

extern void cache_init(CacheList *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}
extern void cache_URL(char *URL, void *item, size_t size, CacheList *list)
{
    while (list->size + size > MAX_CACHE_SIZE)
    {
        evict(list);
    }
    cache_insert(URL, item, size, list);
}

/**
 * @brief Evict the last item in the cache
 *
 * @param list
 */
extern void evict(CacheList *list)
{
    if (list->tail == NULL)
        return;
    CachedItem *item = list->tail;
    list->tail = item->prev;
    if (list->tail == NULL)
        list->head = NULL;
    else
        list->tail->next = NULL;
    list->size -= item->size;
    free(item->url);
    free(item->item);
    free(item);
}

void cache_insert(char *URL, void *item, size_t size, CacheList *list)
{
    CachedItem *node = malloc(sizeof(CachedItem));
    node->url = malloc((strlen(URL) + 1) * sizeof(char));
    strcpy(node->url, URL);
    node->item = malloc(size);
    memcpy(node->item, item, size);
    node->size = size;
    node->next = NULL;
    node->prev = NULL;
    if (list->head == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->size += size;
}

extern CachedItem *find(char *URL, CacheList *list)
{
    CachedItem *item = list->head;
    while (item != NULL)
    {
        if (strcmp(item->url, URL) == 0)
        {
            return item;
        }
        item = item->next;
    }
    return NULL;
}
extern void move_to_front(char *URL, CacheList *list)
{
    CachedItem *item = find(URL, list);
    if (item == NULL)
        return;
    if (item == list->head)
        return;
    if (item == list->tail)
    {
        list->tail = item->prev;
        list->tail->next = NULL;
    }
    else
    {
        item->prev->next = item->next;
        item->next->prev = item->prev;
    }
    item->next = list->head;
    list->head->prev = item;
    list->head = item;
    item->prev = NULL;
}
extern void print_URLs(CacheList *list)
{
    printf("-----------\n");
    CachedItem *item = list->head;
    while (item != NULL)
    {
        printf("%s\n", item->url);
        item = item->next;
    }
    printf("-----------\n");
}
extern void cache_destruct(CacheList *list)
{
    CachedItem *item = list->head;
    while (item != NULL)
    {
        CachedItem *next = item->next;
        free(item->url);
        free(item);
        item = next;
    }
}