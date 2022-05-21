#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct
{
    char *hostname;
    char *port;
    char *path;
    char *buf;
    int size;
} Request;

void *handle_client(void *vargp);
void initialize_struct(Request *req);
void parse_request(char request[MAXLINE], Request *req);
void parse_absolute(Request *req);
void parse_relative(Request *req);
void parse_header(char header[MAXLINE], Request *req);
void assemble_request(Request *req, char *request);
int get_from_cache(Request *req, int clientfd);
void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client);
void close_wrapper(int fd);
void print_full(char *string);
void print_struct(Request *req);

typedef struct CachedItem CachedItem;

struct CachedItem
{
};

typedef struct
{

} CacheList;

extern void cache_init(CacheList *list);
extern void cache_URL(char *URL, void *item, size_t size, CacheList *list);
extern void evict(CacheList *list);
extern CachedItem *find(char *URL, CacheList *list);
extern void move_to_front(char *URL, CacheList *list);
extern void print_URLs(CacheList *list);
extern void cache_destruct(CacheList *list);

int main(int argc, char **argv)
{
    printf("%s", user_agent_hdr);
    return 0;
}

void *handle_client(void *vargp) {}
void initialize_struct(Request *req) {}
void parse_request(char request[MAXLINE], Request *req) {}
void parse_absolute(Request *req) {}
void parse_relative(Request *req) {}
void parse_header(char header[MAXLINE], Request *req) {}
void assemble_request(Request *req, char *request) {}
int get_from_cache(Request *req, int clientfd) {}
void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client) {}
void close_wrapper(int fd) {}
void print_full(char *string) {}
void print_struct(Request *req) {}

extern void cache_init(CacheList *list) {}
extern void cache_URL(char *URL, void *item, size_t size, CacheList *list) {}
extern void evict(CacheList *list) {}
extern CachedItem *find(char *URL, CacheList *list) {}
extern void move_to_front(char *URL, CacheList *list) {}
extern void print_URLs(CacheList *list) {}
extern void cache_destruct(CacheList *list) {}