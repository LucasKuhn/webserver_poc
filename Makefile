all:
	c++ no_epoll.cpp -o no_epoll.out

run: all
	./no_epoll.out