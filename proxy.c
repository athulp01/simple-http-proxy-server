#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_BUF_SIZE MAXLINE

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *http_version = "HTTP/1.0\r\n";
static const char *proxy_hdr = "Proxy-Connection: close\r\n";
static const char *connection_hdr = "Connection: close\r\n";

int parse_get(char *request_src, char *hostname, char *path, char *port, char **url) {
    char *request = strdup(request_src);
    char *saveptr = request;
    char *field = strtok_r(request, " ", &saveptr);
    if(strcmp(request, "GET"))
        return 1;
    char *addr = strtok_r(NULL, " ", &saveptr);
    *url = addr;
    addr += 7;
    int size = strlen(addr);
    char* end_host_idx = strchr(addr, '/');
    strcpy(path, end_host_idx);
    char *start_port_idx = strchr(addr, ':');
    if(start_port_idx) {
        strncpy(hostname, addr, start_port_idx-addr);
        start_port_idx++;
        while(isdigit(*start_port_idx) && (*port++=*start_port_idx++));
    } else {
        strncpy(hostname, addr, end_host_idx - addr);
    }
    return 0;
}

char *generate_req(int clientfd, char *hostname, char *port, char **url) {
    char *request = malloc(MAX_BUF_SIZE);
    rio_t rio;
    char buf[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE];
    Rio_readinitb(&rio, clientfd);
    int n = rio_readlineb(&rio, buf, MAX_BUF_SIZE);
    parse_get(buf, hostname, path, port, url);
    printf("%s %s %s\n",hostname, path, port);
    sprintf(request, "GET %s HTTP/1.0\r\n", path);
    int ishostpres = 0;
    while((n = rio_readlineb(&rio, buf, MAX_BUF_SIZE)) > 0) {
        if(!strcmp(buf, "\r\n")) break;
        if(strstr(buf, "Host:") == buf) {
            ishostpres = 1;
            continue;
        }
        if(strstr(buf, "Connection:") == buf) continue;
        sprintf(request, "%s%s",request, buf);
    }
    sprintf(request, "%s%s",request,proxy_hdr);
    if(ishostpres) sprintf(request, "%sHost: %s\r\n",request, hostname);
    sprintf(request, "%s\r\n", request);
    return request;
}

void *proxy_requests(void *varargs) {
    int clientfd = *((int*)varargs);
    Pthread_detach(Pthread_self());
    char hostname[MAX_BUF_SIZE], buf[MAX_BUF_SIZE], *url;
    char port[10], default_port[] = "80";
    memset(port, 0, 10);
    memset(hostname, 0, MAX_BUF_SIZE);
    memset(buf, 0, MAX_BUF_SIZE);
    char *request = generate_req(clientfd, hostname, port, &url);
    struct web_object *cached = search_cache(url);
    if(cached) {
        printf("Serving from cache %s\n", url);
        printf("data = %s\n", cached->data);
        Rio_writen(clientfd, cached->data, cached->size);
        Close(clientfd);
        free(request);
        free(varargs);
        return NULL;
    }
    if(port[0]==0) strcat(port, "80");
    printf("Opening a connection to %s at %s\n", hostname, port);
    int remote_conn_fd = Open_clientfd(hostname, port);
    if(remote_conn_fd < 0) {
        Close(clientfd);
        Free(varargs);
        return NULL;
    }
    rio_t rio;
    Rio_readinitb(&rio, remote_conn_fd);
    rio_writen(remote_conn_fd, request, strlen(request));
    int size, cur_size=0;
    char full_file[MAX_OBJECT_SIZE];
    int valid = 1;
    while((size = rio_readlineb(&rio, buf, MAX_BUF_SIZE)) != 0) {
        if(cur_size+size < MAX_BUF_SIZE) {
            memcpy(full_file+cur_size, buf, size);
            cur_size += size;
        } else valid = 0;
        rio_writen(clientfd, buf, size);
    }
    if(valid && cur_size <= MAX_OBJECT_SIZE) add_to_cache(url, full_file, cur_size);
    Close(clientfd);
    Close(remote_conn_fd);
    free(varargs);
    free(request);
    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd = Open_listenfd(argv[1]);
    int *clientfd;
    struct sockaddr_storage client;
    unsigned int clientlen = sizeof(client);
    pthread_t tid;
    while(1) {
        clientfd = malloc(sizeof(int));
        *clientfd = Accept(listenfd, (struct sockaddr*)&client, &clientlen);
        Pthread_create(&tid, NULL, proxy_requests, clientfd);
    }
    Close(listenfd);
    return 0;
}
