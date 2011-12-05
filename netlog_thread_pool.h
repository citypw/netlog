#ifndef _NETLOG_THREAD_POOL_
#define _NETLOG_THREAD_POOL_
#endif

#include "netlog.h"
#include <assert.h> /* assert() */

void *routine(void *arg);  
int pool_add_job(void *(*process)(void * arg),void *arg);  
int pool_init(unsigned int thread_num);  
int pool_destroy(void);  

/* We define a queue of jobs which will be processed in thread pool*/  
typedef struct job{  
  void *(*process)(void *arg); /* process() will employed on job*/  
  void *arg;/* argument to process */  
  struct job *next;  
}netlog_queue;

typedef struct tp_info_s{
  unsigned short int *used;
  unsigned short int *curr_thread;
}tp_info;

typedef struct thread_pool{ 
  pthread_mutex_t pool_lock;
  pthread_cond_t  cond_job;  
  netlog_queue *jobs;  
  int destroy; 
  pthread_t *threads;
  unsigned int thread_num; /* max size of threads */
  int size; /* current size of threads */
  tp_info thread_info;
}Thread_pool;

/* global Thread_pool variable*/  
static Thread_pool *pool=NULL;  
  
/*Initialize the thread pool*/  
int pool_init(unsigned int thread_num)  
{  
  pool=(Thread_pool *)malloc(sizeof(Thread_pool));  
  if(NULL==pool)  
    return -1;

  pthread_mutex_init(&(pool->pool_lock),NULL);  
  pthread_cond_init(&(pool->cond_job),NULL);  
  pool->jobs=NULL;  
  pool->thread_num=thread_num;  
  pool->size=0;  
  pool->destroy=0;  
  pool->threads=(pthread_t *)malloc(thread_num * sizeof(pthread_t));  
  pool->thread_info.used = (unsigned short int*)malloc(thread_num * sizeof(unsigned short int));
  pool->thread_info.curr_thread = (unsigned short int*)malloc(thread_num * sizeof(unsigned short int));

  int i;  
  for(i=0;i<thread_num;i++){ 
    pool->thread_info.curr_thread[i] = i; 
    pthread_create(&(pool->threads[i]),NULL,routine, NULL);  
  }
  
  return 0;  
}  
/* 
 * Add job into the pool 
 * assign it to some thread 
 */  
int pool_add_job(void *(*process)(void *),void *arg)  
{  
  netlog_queue * newjob=(netlog_queue *)malloc(sizeof(netlog_queue));  
  newjob->process=process;  
  newjob->arg=arg;  
  newjob->next=NULL;
 
  pthread_mutex_lock(&(pool->pool_lock));
  netlog_queue *temp=pool->jobs;  
  unsigned short int tmp_arg = *(unsigned short int*)arg;

  if(temp!=NULL){  
    while(temp->next)  
      temp=temp->next;  
    temp->next=newjob;  
  }  
  else{  
    pool->jobs=newjob;  
  }  
      
  /* For debug */  
  assert(pool->jobs!=NULL);  
  pool->size++;  
  pool->thread_info.used[tmp_arg] = 1;
  printf("work thread %d is busy now\n", pool->thread_info.curr_thread[tmp_arg]);
  pthread_mutex_unlock(&(pool->pool_lock));

  /* Rouse a thread to process this new job */  
  pthread_cond_signal(&(pool->cond_job));  
  return 0;  
}
  
/*Destroy the thread pool*/  
int pool_destroy(void)  
{  
  if(pool->destroy)/*Alread destroyed!*/  
    return -1;  

  int i;  
  pool->destroy=1;  
  pthread_cond_broadcast(&(pool->cond_job)); /*notify all threads*/  

  for(i=0;i<pool->thread_num;i++)  
    pthread_join(pool->threads[i],NULL);  
  free(pool->threads);
  free(pool->thread_info.used);
  free(pool->thread_info.curr_thread);

  netlog_queue * head=NULL;  
  while(pool->jobs){  
    head=pool->jobs;  
    pool->jobs=pool->jobs->next;  
    free(head);  
  }  
  pthread_mutex_destroy(&(pool->pool_lock));  
  pthread_cond_destroy(&(pool->cond_job));  
  free(pool);  
  pool=NULL;  
  return 0;  
}

unsigned long int glb_i = 0;
/**
 * routine - every threads will call this func after created
 *
 * It doesn't need return value
 */  
void *routine(void *arg)  
{
  while(1){  
    pthread_mutex_lock(&(pool->pool_lock));
    while(pool->size==0 && !pool->destroy){  
      printf("thread %u is waiting\n",pthread_self());
      pthread_cond_wait(&(pool->cond_job),&(pool->pool_lock));  
    }
  
    if(pool->destroy){  
      pthread_mutex_unlock(&(pool->pool_lock));  
      printf("thread %u will exit\n",pthread_self());  
      pthread_exit(NULL);  
    }  
    printf("thread %u is starting to work\n",pthread_self());

    /*For debug*/  
    assert(pool->size!=0);
    assert(pool->jobs!=NULL);

    pool->size--;
    netlog_queue *job=pool->jobs;
    pool->jobs=job->next;
    pthread_mutex_unlock(&(pool->pool_lock));  

    (*(job->process))(job->arg);
    unsigned short int tmp_arg = *(unsigned short int*)job->arg;
    printf("Work thread %d is free now\n", tmp_arg);
    pool->thread_info.used[tmp_arg] = 0;
    free(job);  
    job=NULL;  
  }

  pthread_exit(NULL);  
}
