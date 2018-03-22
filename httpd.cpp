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


void *accept_request(void *arg)
{
    pthread_detach(pthread_self());
    
    char buf[1024] = {0};
    const int client = *(int*)arg;
    //memset(buf, 0, sizeof(buf));
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

int main()
{
    u_short port = 0;
    int lstfd = startup(&port); //listen socket
    int cntfd = -1;             //connect sockt
    struct sockaddr_in client_addr;
    pthread_t newthread;

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);
        if ((cntfd = accept(lstfd, (struct sockaddr*)&client_addr, &addr_len)) == -1)
        {
            error_die("accept");
        }
        else
        {
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
        }

        if (pthread_create(&newthread, NULL, accept_request, (void*)&cntfd) != 0)
        {
            perror("pthread_create");
        }
    }

    close(lstfd);

    return 0;
}