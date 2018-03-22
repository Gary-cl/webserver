#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

int main(int argc, char *argv[])
{
    int cntfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    if (argc != 2)
    {
        printf("please input port\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
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

    my_write(stdin, cntfd);

    return 0;
}