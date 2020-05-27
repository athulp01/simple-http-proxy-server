#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_BUF_SIZE MAXLINE

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *http_version = "HTTP/1.0\r\n";
static const char *proxy_hdr = "Proxy-Connection: close\r\n";
static const char *connection_hdr = "Connection: close\r\n";

int parse_get(char *request_src, char *hostname, char *path) {
    char *request = strdup(request_src);
    char *saveptr = request;
    char *field = strtok_r(request, " ", &saveptr);
    if(strcmp(request, "GET"))
        return 1;
    char *addr = strtok_r(NULL, " ", &saveptr);
    int count = 0, i;
    int size = strlen(addr);
    for(i=0; i<size && count<=2; i++) {
        if(addr[i] == '/') count++;
    }
    strncpy(hostname, addr+7, i-1-7);
    strcpy(path, addr+i-1);
    return 0;
}

char *generate_req(int clientfd, char *hostname) {
    char *request = malloc(MAX_BUF_SIZE);
    rio_t rio;
    char buf[MAX_BUF_SIZE];
    char path[MAX_BUF_SIZE];
    Rio_readinitb(&rio, clientfd);
    int n = rio_readlineb(&rio, buf, MAX_BUF_SIZE);
    parse_get(buf, hostname, path);
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
    char hostname[MAX_BUF_SIZE], buf[MAX_BUF_SIZE];
    memset(hostname, 0, MAX_BUF_SIZE);
    memset(buf, 0, MAX_BUF_SIZE);
    char *request = generate_req(clientfd, hostname);
    printf("%s %s\n", hostname, request);
    int remote_conn_fd = Open_clientfd(hostname, "80");
    rio_t rio;
    Rio_readinitb(&rio, remote_conn_fd);
    rio_writen(remote_conn_fd, request, strlen(request));
    int size;
    while((size = rio_readlineb(&rio, buf, MAX_BUF_SIZE)) != 0) {
        rio_writen(clientfd, buf, size);
    }
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
        printf("Got connection\n");
        Pthread_create(&tid, NULL, proxy_requests, clientfd);
        printf("Over\n");
    }
    return 0;
}
