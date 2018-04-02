all : httpd client

httpd : httpd.o tpool.o epoll.o
	cc -o httpd httpd.o tpool.o epoll.o -lpthread

client : client.o
	cc -o client client.o

.PHONY :
	 clean

clean :
	rm *.o
	rm httpd
	rm client
