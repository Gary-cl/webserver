#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>

#define err_sys(info)                                                        \
        {                                                                        \
                fprintf(stderr, "%s:%s\n", info, strerror(errno));                \
                exit(EXIT_FAILURE);                                                \
        }

#define INET_ADDR_LEN 100

void error_die(const char *func_name)
{
    perror(func_name);
    exit(1);
}

void my_write(FILE *fp, int socket)
{
    char wbuffer[1024] = {0}, rbuffer[1024] = {0};

    while (fgets(wbuffer, sizeof(wbuffer), fp) != NULL)
    {
        send(socket, wbuffer, strlen(wbuffer), 0);
        memset(wbuffer, 0, sizeof(wbuffer));
        if (recv(socket, rbuffer, sizeof(rbuffer), 0) < 0)
        {
            error_die("recv");
        }
        fputs(rbuffer, stdout);
        memset(rbuffer, 0, sizeof(wbuffer));
    }
}

int tcp_connct(int port)
{
    int cntfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr) == -1)
    {
        error_die("inet_pton");
    }

    if (connect(cntfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_die("connect");
    }
    else
    {
        struct sockaddr_in client_addr, serv_addr;
        socklen_t client_len = sizeof(client_addr), serv_len = sizeof(serv_addr);
        if (getpeername(cntfd, (struct sockaddr *)&serv_addr, &serv_len) == -1)
        {
            error_die("getsockname");
        }
        if (getsockname(cntfd, (struct sockaddr *)&client_addr, &client_len) == -1)
        {
            error_die("getpeername");
        }
        char client_ip[INET_ADDR_LEN], serv_ip[INET_ADDR_LEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        inet_ntop(AF_INET, &serv_addr.sin_addr, serv_ip, sizeof(serv_ip));
        printf("server(\"%s\":%d) connect to client(\"%s\":%d)\n", serv_ip, ntohs(serv_addr.sin_port), client_ip, ntohs(client_addr.sin_port));
    }

    return cntfd;
}

int Fork()
{
    int ret = fork();
    if (ret < 0)
    {
        perror("fork");
    }
    return ret;
}

int main(int argc, char *argv[])
{
    char request[100], reply[100];
    if (argc != 5)
    {
        printf("input error\n");
        exit(1);
    }
    int port = atoi(argv[1]);   // 端口
    int nchildren = atoi(argv[2]); // 子进程数
    int nloop = atoi(argv[3]);  // 每个子进程请求数
    int nbyte = atoi(argv[4]);  // 每个请求要求的字节

    snprintf(request, sizeof(request), "%d\n", nbyte);

    for (int i = 0; i < nchildren; i++)
    {
        int pid;
        if ((pid = Fork()) == 0)    // child
        {
            for (int j = 0; j < nloop; j++)
            {
                int fd = tcp_connct(port);
                
                write(fd, request, strlen(request));
                printf("send: %s\n", request);
                int n = 0;
                if ((n = read(fd, reply, nbyte)) != nbyte)
                {
                    printf("read %d data\n", n);
                }
                printf("reply: %s\n", reply);
                memset(reply, 0, sizeof(reply));
                
                close(fd);
            }
            printf("child %d done\n", i);
            exit(0);
        }
        // parent
    }
    while (wait(NULL) > 0)
    ;
    if (errno != ECHILD)
        err_sys("wait error");
    
    return 0;
}