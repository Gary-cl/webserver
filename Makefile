all : server client


# httpd : httpd.o tpool.o epoll.o \
	cc -o httpd httpd.o tpool.o epoll.o -lpthread

server : server.o thread_pool.o listen_handler.o socket_handler.o reactor.o reactor_impl.o epoll_demultiplexer.o rio.o
	g++ -o server server.o thread_pool.o listen_handler.o socket_handler.o reactor.o reactor_impl.o epoll_demultiplexer.o rio.o -lpthread 

client : client.o
	g++ -o client client.o

.PHONY :
	 clean

clean :
	rm *.o
	rm server
	rm client
