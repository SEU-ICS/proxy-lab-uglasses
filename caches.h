//for cache
#include "structs.h"
#include "sbuf.h"
typedef struct {
	int valid;			
	char request[MAXLINE];	
	char response[MAXLINE];		
	int cnt;				// LRU cnt
} CacheEntry; //basic unit

typedef struct {
	sem_t mutex;		
	CacheEntry *cache_set;	
	int num;
} Cache;

void cache_init(Cache *cache, int n) 
{
	Sem_init(&cache->mutex, 0, 1);
	cache->cache_set = Calloc(n ,sizeof(CacheEntry)); //Calloc good
	cache->num = n;
}

 int cache_find(Cache *cache, Line *line, int fd) {//1 hit 0 miss
	int cache_hit = 0;		
	char match_str[MAXLINE];
	CacheEntry *cache_entry;

	sprintf(match_str, "%s http://%s:%s%s %s", line->method, line->host, line->port, line->path, line->version);	

	P(&cache->mutex);

	for (int i = 0; i < cache->num; i++) {
		cache_entry = cache->cache_set + i;

		if (cache_entry->valid == 0) continue;

		if (!strcmp(match_str, cache_entry->request)) 
        {
			cache_hit = 1;
			Rio_writen(fd, cache_entry->response, MAX_OBJECT_SIZE); //direct write to client
			cache_entry->cnt = 0;
			fprintf(stderr, "cache hit!\n");
		}
        else
            cache_entry->cnt++; //LRU
	}

	V(&cache->mutex);

	if (!cache_hit) {
		fprintf(stderr, "cache miss!\n");
	}

	return cache_hit;
}
void cache_insert(Cache *cache, Line *line, char *response) {
	CacheEntry *cache_entry;
	int LRU_cnt = -1, insert_index = -1;
	int cache_full = 1;
	char match_str[MAXLINE];

	sprintf(match_str, "%s http://%s:%s%s %s", line->method, line->host, line->port, line->path, line->version);

	P(&cache->mutex);
	for (int i = 0; i < cache->num; i++)
    {
		cache_entry = cache->cache_set + i;

		if (!cache_entry->valid) 
        {
			cache_entry->cnt = 0;
			cache_entry->valid = 1;
			strcpy(cache_entry->request, match_str);
			strcpy(cache_entry->response, response);
			cache_full = 0;
			break;
		}

		if (LRU_cnt < cache_entry->cnt) 
        {
			LRU_cnt = cache_entry->cnt;
			insert_index = i;
		}

	}

	if (cache_full) 
    {
		cache_entry = cache->cache_set + insert_index;
		cache_entry->cnt = 0;
		strcpy(cache_entry->request, match_str);
		strcpy(cache_entry->response, response);
	}

	V(&cache->mutex);
}