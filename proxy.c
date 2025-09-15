#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "caches.h"
/* Recommended max cache and object sizes */
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
Cache cache;
sbuf_t sbuf;

void parser(char *uri, char* host, char *path, char *port) {
	char *path_start, *port_start;

	uri += 7; //skip for strlen("http://")

	if ((path_start = strstr(uri, "/")) == NULL) {
		strcpy(path, "/");
	} //default path /
    else {
		strcpy(path, path_start);
		*path_start = '\0';
	}

	if ((port_start = strstr(uri, ":")) == NULL) {
		strcpy(port, "80");
	}//default port 80
    else {
		strcpy(port, port_start + 1);
		*port_start = '\0';
	}

	strcpy(host, uri);
}

void read_request(int fd, Request *request) {
	char buf[MAXLINE], uri[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", request->line.method, uri, request->line.version);
	if (strcasecmp(request->line.method, "GET"))	exit(1);
	//parse uri
    parser(uri, request->line.host, request->line.path, request->line.port);
    //read headers
	Header *header = request->headers;
	request->header_num = 0;
	while (strcmp(buf, "\r\n")) 
    {	
        char *c = strstr(buf, ":");
        if (!c)	exit(1);
        sscanf(buf, "%s: %s", header->name, header->value);
		request->header_num++;
		Rio_readlineb(&rio, buf, MAXLINE);
	}
}

int send_to_server(Request *request) {
	int clientfd;
	char req[MAXLINE];
	Line *line = &request->line;
	int header_len = request->header_num;
	rio_t rio;

	clientfd = Open_clientfd(line->host, line->port);
	Rio_readinitb(&rio, clientfd);
	sprintf(req, "%s %s %s\r\n", line->method, line->path, line->version);
	Rio_writen(clientfd, req, strlen(req));
	sprintf(req, "Host: %s:%s\r\n", line->host, line->port);
	Rio_writen(clientfd, req, strlen(req));
	sprintf(req, "User-Agent: %s", user_agent_hdr);
	Rio_writen(clientfd, req, strlen(req));
	sprintf(req, "Connection: close\r\n");//always close !
	Rio_writen(clientfd, req, strlen(req));
	sprintf(req, "Proxy-Connection: close\r\n");//always close !
	Rio_writen(clientfd, req, strlen(req));

    for(int i=0;i<header_len;i++)
    {
        Header hd = request->headers[i];
		sprintf(req, "%s: %s\r\n", hd.name, hd.value);
		Rio_writen(clientfd, req, strlen(req));
    }

	Rio_writen(clientfd, "\r\n", strlen("\r\n"));	

	return clientfd;
}
void send_to_client(int fd, int clientfd, Line *line) {
    char buf[MAXLINE], res[MAX_OBJECT_SIZE];
    int response_bytes = 0;
    int n;
    rio_t rio;

    Rio_readinitb(&rio, clientfd);
    while ((n = Rio_readnb(&rio, buf, MAXLINE)) > 0) {
        Rio_writen(fd, buf, n);
        if (response_bytes < MAX_OBJECT_SIZE && response_bytes + n <= MAX_OBJECT_SIZE)
            memcpy(res + response_bytes, buf, n);
        response_bytes += n;
    }
    if (response_bytes > 0 && response_bytes <= MAX_OBJECT_SIZE)    cache_insert(&cache, line, res);
}

void doit(int fd) {
	Request request;
	read_request(fd, &request);	//read request from client, with preprocessing
	 if (!cache_find(&cache, &request.line, fd)) {	
		int clientfd = send_to_server(&request);	//send request to server
		send_to_client(fd, clientfd, &request.line);	//send response to client
		Close(clientfd);
	 }
     //else do nothing
}

void *thread_action(void *vargp) {
	Pthread_detach(pthread_self());
	while(1) {
		int connfd = sbuf_remove(&sbuf); //multi-threading
		doit(connfd);
		Close(connfd);
	}
}
int main(int argc, char **argv)
{
	int listenfd, *connfd;// for independent thread	
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	Signal(SIGPIPE, SIG_IGN);//ignore SIGPIPE
	cache_init(&cache, MAX_CACHE_SIZE / MAX_OBJECT_SIZE);
	listenfd = Open_listenfd(argv[1]);

	sbuf_init(&sbuf, SBUFSIZE);
	for (int i = 0; i < THREAD_NUM; i++) 
		Pthread_create(&tid, NULL, thread_action, NULL);

	while(1) 
    {
		clientlen = sizeof(clientaddr);
		connfd = Malloc(sizeof(int));
		*connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA*) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		sbuf_insert(&sbuf, *connfd);
	}

    return 0;
}
