#include "thread_pool.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>

static void *thread_entry(void *arg);


/* 线程入口,线程池中的线程将在这里等待被唤醒 */
static void *thread_entry(void *arg)
{
    //线程分离，自动回收线程资源
    pthread_detach(pthread_self());

    thread_pool_t *pool = (thread_pool_t*)arg;
    pthread_mutex_lock(&(pool->qlock));
    
    while (pool->task_head->task_len == 0)
    {
        // 等待条件变量
        pthread_cond_wait(&(pool->cond), &(pool->qlock));
    }

    task_node *task = get_task(pool->task_head); 
    pthread_mutex_unlock(&(pool->qlock));

    //回调函数
    task->callback(task->arg);
    free(task->arg);
    free(task);

    pthread_exit((void *)NULL);
    return NULL;
}



/* 线程池初始化 */
thread_pool_t* thread_pool_init(int thread_num)
{
    if (thread_num <= 0)
    {
        printf("thread_pool_init:thread_num must grater than 0\n");
        return NULL;
    }

    thread_pool_t *pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if (!pool)
    {
        goto err;
    }
    
    pool->thread_pool_size = thread_num;
    pthread_mutex_init(&(pool->qlock), NULL);
    pthread_cond_init(&(pool->cond), NULL);
    pool->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * pool->thread_pool_size);
    pool->task_head = (task_head_node*)malloc(sizeof(task_head_node));
    if (!(pool->pthreads) || !(pool->task_head))
    {
        goto err;
    }
    pool->task_head->next = NULL;
    pool->task_head->task_len = 0;


    for (int i = 0; i < pool->thread_pool_size; ++i)
    {
        int err = pthread_create(&(pool->pthreads[i]), NULL, thread_entry, pool);
        if (err == -1)
        {
            fprintf(stderr, "pthread_creare error\n");
            free(pool->pthreads);
            goto err;
        }
    }

    return pool;

err:
    printf("thread_pool_init:error\n");
    return NULL;
}

/* 返回第一个task node，这个node的内存将在被调用后才回收 */
task_node *get_task(task_head_node *head)
{
    task_node *node = head->next;

    if (!node)
    {
        printf("get_task:error\n");
        return NULL;
    }
    head->next = head->next->next;
    head->task_len--;

    return node;
}

void add_task(task_head_node *head, task_node* node)
{
    if (!node)
    {
        printf("add_task:error\n");
        return;
    }

    node->next = head->next;
    head->next = node;

    head->task_len++;

}