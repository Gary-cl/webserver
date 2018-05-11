#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <map>

#include "event_handler.h"
#include "reactor.h"
#include "socket_handler.h"
#include "thread_pool.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "rio.h"
#include <fcntl.h>
#include <sys/mman.h>
 #include <sys/wait.h>


SocketHandler::SocketHandler(int fd) : sock_fd(fd)
{
    my_thread_pool = thread_pool_init(20);
}

SocketHandler::~SocketHandler()
{
}

handle_t SocketHandler::get_handle() const
{
    return sock_fd;
}


extern char* root_dir;
extern char* home_page;

#define MAXLINE 8192
#define MAXBUF 8192  


void read_requesthdrs(rio_t *rp)
{
	char buf[MAXLINE];

	rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {   
		rio_readlineb(rp, buf, MAXLINE);
		//printf("%s", buf);
	}
	return;
}


void clienterror(int fd, char *cause, char *errnum,
	char *shortmsg, char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];

	/* http 响应报文 主体 */
	sprintf(body, "<html><title>Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Web server</em>\r\n", body);

	/* http 响应报文 */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	rio_writen(fd, buf, strlen(buf));
	rio_writen(fd, body, strlen(body));
}


// 解析uri
int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;

	printf("url = %s\n", uri);

	if ((ptr = index(uri, '?')) == NULL) // 静态资源
	{
		strcpy(cgiargs, "");                    
		strcpy(filename, root_dir);                 
		strcat(filename, uri);               
		if (uri[strlen(uri) - 1] == '/')            
			strcat(filename, home_page);      
		return 1;
	}
	else {  						// 动态资源     
		// 未实现
		return 0;
	}
}


void get_filetype(char *filename, char *filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg"); //image/png
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".css"))
		strcpy(filetype, "text/css");
	else if (strstr(filename, ".ttf") || strstr(filename, ".otf"))
		strcpy(filetype, "application/octet-stream");
	else
		strcpy(filetype, "text/plain");
}

// 获取静态资源
void serve_static(int fd, char *filename, int filesize)
{
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	// 响应 首部 
	get_filetype(filename, filetype); 
	sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
	sprintf(buf, "%sServer: Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	rio_writen(fd, buf, strlen(buf));   

	// 响应 主体
	srcfd = open(filename, O_RDONLY, 0);    
	srcp = (char *)mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 文件映射到内存，0：自动瘦
	close(srcfd);                         
	rio_writen(fd, srcp, filesize);      
	munmap(srcp, filesize);       // 解除映射
}




static void *real_read(void *arg)
{
    int is_static;
    struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    const int fd = *(int *)arg;
	char filename[MAXLINE], cgiargs[MAXLINE];

    printf("real_read: client socket = %d\n", fd);


	// 初始化io
    rio_t rio;
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);

    printf("recv: %s\n", buf);
	
	// 请求行
	sscanf(buf, "%s %s %s", method, uri, version);   
	if (strcasecmp(method, "GET")) {
		clienterror(fd, method, (char *)"501", (char *)"Not Implemented",
			(char *)"HTTP request method not supported");
		return (void*)NULL;
	}
	read_requesthdrs(&rio);	// 读出头部，并未进行处理

	// 解析uri
	is_static = parse_uri(uri, filename, cgiargs);  
	printf("filename = %s\n", filename);

	// 获取文件信息
	if (stat(filename, &sbuf) < 0) {      
		clienterror(fd, filename, (char *)"404", (char *)"Not found",
			(char *)"Couldn't find this file");  //没有找到文件 
		return (void*)NULL;
	}   

	if (is_static) { // 静态资源
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // S_ISREG:常规文件， S_IRUSR:	文件所有者的读权限
        	clienterror(fd, filename, (char *)"403", (char *)"Forbidden",
				(char *)"Couldn't read the file"); // 权限不够 
			return (void*)NULL;
		}
		serve_static(fd, filename, sbuf.st_size); 
	}
	else { // 动态资源
		/*
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // S_IXUSR:文件所有者的执行权限
            clienterror(fd, filename, "403", "Forbidden",
				"Couldn't run the CGI program"); 
			return (void*)NULL;
		}
		*/
		// 动态资源功能未实现
		clienterror(fd, method, (char *)"501", (char *)"Not Implemented",
			(char *)"HTTP request method not supported");
	}

    close(fd);
    printf("close socket = %d\n", fd);
    return (void*)NULL;
}



/*
static void *real_read(void *arg)
{
    char buf[MAXLINE] = {0};
    const int client = *(int *)arg;

    printf("real_read: client socket = %d\n", client);


    rio_t rio;
    rio_readinitb(&rio, client);
    rio_readlineb(&rio, buf, MAXLINE);

    printf("recv: %s\n", buf);
    int num = atoi(buf);
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < num; i++)
    {
        buf[i] = 'x';
    }

    rio_writen(client, buf, strlen(buf));
    if (send(client, buf, strlen(buf), 0) < 0)
    {
        close(client);
        perror("send");
        exit(1);
    }
    close(client);
    printf("close socket = %d\n", client);
    return (void*)NULL;
}
*/

/*
static void *real_read(void *arg)
{
    char buf[1024] = {0};
    const int client = *(int *)arg;

    printf("client socket = %d\n", client);
    int n = recv(client, buf, sizeof(buf), 0);
    while (n > 0)
    {
        printf("recv: %s\n", buf);
        int num = atoi(buf);
        memset(buf, 0, sizeof(buf));
        for (int i = 0; i < num; i++)
        {
            buf[i] = 'x';
        }
        if (send(client, buf, strlen(buf), 0) < 0)
        {
            close(client);
            perror("send");
            exit(1);
        }
        memset(buf, 0, sizeof(buf));
        n = recv(client, buf, sizeof(buf), 0);
    }

    close(client);
    printf("close socket = %d\n", client);
    return (void*)NULL;
}
*/

void SocketHandler::handle_read()
{
    task_node *task = (task_node *)malloc(sizeof(task_node));
    task->callback = (task_handler)real_read;
	
	// 避免资源竞争，将fd备份到堆中，callback函数中回收内存。
	int *tmpfd = (int*)malloc(sizeof(int));
	*tmpfd = this->sock_fd;
    task->arg = tmpfd;
    task->next = NULL;
    
    // real_read加入task链表
    pthread_mutex_lock(&(my_thread_pool->qlock));
    
    add_task(my_thread_pool->task_head, task);
    pthread_mutex_unlock(&(my_thread_pool->qlock));

    
    /*
        为了保证close前调用epoll_ctl注销事件
        handle_error中epoll_ctl注销了该描述符的事件
        real_read函数中close描述符
    */
    handle_error();
    pthread_cond_signal(&(my_thread_pool->cond)); //唤醒与一个线程    
}

void SocketHandler::handle_error()
{
    //printf("remove handle\n");
    Reactor& r = Reactor::get_instance();
    r.remove( this );
}

