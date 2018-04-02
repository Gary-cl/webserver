#include "tpool.h"
#include <cstdio>
#include <cstdlib>


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
    task->func(task->arg);
    free(task);

    /*
    void *task;
    pthread_t id = *(int*)arg;

    pthread_mutex_lock(&my_thread_pool.qlock);
    while (my_thread_pool.ready <= 0)
    {
        pthread_cond_wait(&my_thread_pool.ready_cond, &my_thread_pool.qlock);
    }
    task = get_task(my_thread_pool.tasks);
    pthread_mutex_unlock(&my_thread_pool.qlock);

    my_thread_pool.task_handler(task); //调用回调函数
    */

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


/*
    tp->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * size);
    if (tp->pthreads == NULL)
    {
        fprintf(stderr, "malloc fail in thread_pool_init");
    }

    tp->thread_pool_size = size;



    tp->tasks = list_create();
    if(tp->tasks == NULL) {
        fprintf(stderr, "list_create() error\n");
        return ;
    }

    tp->task_handler = func;
    tp->ready = 0;

    for (int i = 0; i < size; ++i)
    {
        int err = pthread_create(&(tp->pthreads[i]), NULL, thread_entry, &i);
        if (err == -1)
        {
            fprintf(stderr, "pthread_creare error\n");
            free(tp->pthreads);
            return;
        }
    }
*/
    return pool;

err:
    printf("thread_pool_init:error\n");
    return NULL;
}

/* 第一个task出队并被返回，这个task的内存将在被调用后才回收 */
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

/*
    node = tasks->front->next;
    if (node == tasks->tail)
    {
        printf("error\n");
    }
    tasks->front->next = tasks->front->next->next;
    my_thread_pool.ready--;
*/
    return node;
}

/* task入队 */
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

    /*
    tasks->tail->task = node.task;
    tasks->tail->arg = node.arg;
    tasks->tail->next = (task_node *)malloc(sizeof(task_node));
    if (my_thread_pool.tasks->tail->next == NULL)
    {
        printf("malloc fail in add_task\n");
    }

    tasks->tail = tasks->tail->next;

    tasks->tail->task = NULL;
    tasks->tail->arg = -1;
    tasks->tail->next = NULL;

    my_thread_pool.ready++;
    */
}