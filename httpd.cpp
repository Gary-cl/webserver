#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <pthread.h>


#define INET_ADDR_LEN 100


typedef void (*thread_pool_task_handler)(void *arg);

typedef struct task_node
{
    void *task;
    int arg;
    struct task_node *next;
}task_node;

typedef struct task_list
{
    task_node *front;
    task_node *tail;
}task_list;

typedef struct thread_pool_t
{
    pthread_mutex_t qlock;
    pthread_cond_t ready_cond;
    pthread_t *pthreads;
    task_list *tasks;
    int thread_pool_size;
    int ready;
    thread_pool_task_handler task_handler;
}thread_pool_t;

thread_pool_t my_thread_pool;


task_node* get_task(task_list* tasks);
void add_task(task_list* tasks, task_node node);
void *thread_entry(void *arg);
void thread_pool_init(thread_pool_t *tp, int size, thread_pool_task_handler func);
void error_die(const char *func_name);
int startup(u_short *port);
void *accept_request(void *arg);
void my_thread_task_handler(void *arg);
void *test(void *arg);


/* 创建task队列，front与tail为空节点，task存放于两个空节点之间 */
task_list* list_create()
{
    task_list *list = (task_list*)malloc(sizeof(task_list));
    list->front = (task_node*)malloc(sizeof(task_node));
    list->tail = (task_node*)malloc(sizeof(task_node));
    list->front->task = NULL;
    list->front->arg = -1;
    list->front->next = list->tail;
    list->tail->task = NULL;
    list->tail->arg = -1;
    list->tail->next = NULL;
    return list;
}


/* 第一个task出队并被返回，这个task的内存将在被调用后才回收 */
task_node* get_task(task_list* tasks)
{
    task_node *node;
    
    node = tasks->front->next;
    if (node == tasks->tail)
    {
        printf("error\n");
    }
    tasks->front->next = tasks->front->next->next;
    my_thread_pool.ready--;

    return node;
}

/* task入队 */
void add_task(task_list* tasks, task_node node)
{
    tasks->tail->task = node.task;
    tasks->tail->arg = node.arg;
    tasks->tail->next = (task_node*)malloc(sizeof(task_node));
    if (my_thread_pool.tasks->tail->next == NULL)
    {
        printf("malloc fail in add_task\n");
    }

    tasks->tail = tasks->tail->next;

    tasks->tail->task = NULL;
    tasks->tail->arg = -1;
    tasks->tail->next = NULL;

    my_thread_pool.ready++;}


/* 线程入口,线程池中的线程将在这里等待被唤醒 */
void *thread_entry(void *arg)
{
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
}

/* 线程池初始化 */
void thread_pool_init(thread_pool_t *tp, int size, thread_pool_task_handler func)
{

    tp->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * size);
    if (tp->pthreads == NULL)
    {
        fprintf(stderr, "malloc fail in thread_pool_init");
    }

    tp->thread_pool_size = size;

    pthread_mutex_init(&(tp->qlock), NULL);
    pthread_cond_init(&(tp->ready_cond), NULL);

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

}

/* 输出error */
void error_die(const char *func_name)
{
    perror(func_name); 
    exit(1);
}


/* 配置socket并返回描述符 */
int startup(u_short *port)
{
    int lstfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lstfd == -1)
    {
        error_die("socket");
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(*port);

    if (bind(lstfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error_die("bind");
    }
    else
    {
        socklen_t len = sizeof(serv_addr);
        if (getsockname(lstfd, (struct sockaddr*)&serv_addr, &len) == -1)
        {
            error_die("getsockname");
        }
        char serv_ip[INET_ADDR_LEN];
        inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
        printf("bind in socket(\"%s\":%d)\n", serv_ip, ntohs(serv_addr.sin_port));
    }


    if (listen(lstfd, 5) < 0)
    {
        error_die("listen");
    }

    return lstfd;
}

/* socket通信函数，此处为echo */
void *accept_request(void *arg)
{
    pthread_detach(pthread_self());
    

    char buf[1024] = {0};
    const int client = *(int*)arg;

    printf("client socket = %d\n", client);
    int n = recv(client, buf, sizeof(buf), 0);
    while (n > 0)
    {
        if (send(client, buf, strlen(buf), 0) < 0)
        {
            close(client);
            error_die("send");
        }
        memset(buf, 0, sizeof(buf));
        n = recv(client, buf, sizeof(buf), 0);
    }

    close(client);

    pthread_exit(0);
}

/* 用于测试回调函数 */
void *test(void *arg)
{
    printf("this is a test : %d\n", *(int*)arg);
}

/* 回调函数 */
void my_thread_task_handler(void *arg)
{
    task_node *tmp = (task_node *)arg;
    thread_pool_task_handler func= (thread_pool_task_handler)(tmp->task);
    //void (*func)(void*) = (void (*)(void*))(tmp->task);
    func((void*)&(tmp->arg));

    free(tmp);  //从get_task获得的实例，直到task被完成后才回收内存。
}



int main()
{
    u_short port = 0;
    int lstfd = startup(&port); //listen socket
    int cntfd = -1;             //connect sockt
    struct sockaddr_in client_addr;
    pthread_t newthread;
    thread_pool_init(&my_thread_pool, 20, my_thread_task_handler);
    

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);
        if ((cntfd = accept(lstfd, (struct sockaddr*)&client_addr, &addr_len)) == -1)
        {
            error_die("accept");
        }
        else
        {
            /* accept成功， 则打印socket连接信息 */
            struct sockaddr_in client_addr, serv_addr;
            socklen_t client_len = sizeof(client_addr), serv_len = sizeof(serv_addr);
            if (getsockname(cntfd, (struct sockaddr *)&serv_addr, &serv_len) == -1)
            {
                error_die("getsockname");
            }
            if (getpeername(cntfd, (struct sockaddr *)&client_addr, &client_len) == -1)
            {
                error_die("getpeername");
            }
            char client_ip[INET_ADDR_LEN], serv_ip[INET_ADDR_LEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
            printf("server(\"%s\":%d) connect to client(\"%s\":%d)\n", serv_ip, ntohs(serv_addr.sin_port), client_ip, ntohs(client_addr.sin_port));


            /* 将accept_request函数作为task，cntfd作为参数， 入队 */
            task_node tmp = {(void *)accept_request, cntfd, NULL};
            pthread_mutex_lock(&my_thread_pool.qlock);
            add_task(my_thread_pool.tasks, tmp);
            pthread_mutex_unlock(&my_thread_pool.qlock);
            pthread_cond_signal(&my_thread_pool.ready_cond);    //唤醒与一个线程
        }
    }

    close(lstfd);

    return 0;
}


