//for basic abilities
#include "csapp.h"
#define SBUFSIZE 1024
#define THREAD_NUM 10
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
typedef struct {
	char method[MAXLINE]; //GET only
	char host[MAXLINE], port[MAXLINE], path[MAXLINE]; //uri
	char version[MAXLINE];  //always 1.0 ?
} Line;

typedef struct {
	char name[MAXLINE], value[MAXLINE];	
} Header;

typedef struct {
	Line line;			
	Header headers[20];
	int header_num;				
} Request;




