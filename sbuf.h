//for multi-thread
#include "csapp.h"
typedef struct {
	int *buf, n;				
	int front, back;			
	sem_t mutex;		
	sem_t remain, produce;//producer & consumer		
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n) {
	sp->buf = Malloc(n * sizeof(int));
	sp->n = n;
	sp->front = sp->back = 0;
	Sem_init(&sp->mutex, 0, 1);
	Sem_init(&sp->produce, 0, 0);
	Sem_init(&sp->remain, 0, n);
}

void sbuf_insert(sbuf_t *sp, int val) {//add item
	P(&sp->remain);
	P(&sp->mutex);
	sp->buf[(++sp->back) % (sp->n)] = val;
	V(&sp->mutex);
	V(&sp->produce);
}

int sbuf_remove(sbuf_t *sp) {//take item
	int val;
	P(&sp->produce);
	P(&sp->mutex);
	val = sp->buf[(++sp->front) % (sp->n)];
	V(&sp->mutex);
	V(&sp->remain);
	return val;
}