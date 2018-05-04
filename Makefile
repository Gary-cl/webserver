all : httpd client


# httpd : httpd.o tpool.o epoll.o \
	cc -o httpd httpd.o tpool.o epoll.o -lpthread

httpd : httpd.o thread_pool.o listen_handler.o socket_handler.o reactor.o reactor_impl.o epoll_demultiplexer.o
	g++ -o httpd httpd.o thread_pool.o listen_handler.o socket_handler.o reactor.o reactor_impl.o epoll_demultiplexer.o -lpthread 

client : client.o
	g++ -o client client.o

.PHONY :
	 clean

clean :
	rm *.o
	rm httpd
	rm client
