#ifndef TPOOL_H
#define TPOOL_H

#include <pthread.h>


typedef void (*task_handler)(void *arg);

// task节点
typedef struct task_node
{
    task_handler func;
    void *arg;
    struct task_node *next;
}task_node;

// task头结点
typedef struct task_head_node
{
    int task_len;
    struct task_node *next;
}task_head_node;

// 线程池
typedef struct thread_pool_t
{
    pthread_mutex_t qlock;
    pthread_cond_t cond;
    pthread_t *pthreads;
    task_head_node *task_head;
    int thread_pool_size;

}thread_pool_t;


thread_pool_t* thread_pool_init(int thread_num);
task_node *get_task(task_head_node *head);
void add_task(task_head_node *head, task_node *node);


#endif