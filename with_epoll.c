#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/socket_test.sock"

bool setnonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return false;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return false;
	return true;
}

void panic(char *s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

int server_socket()
{
	// SOCK_NONBLOCK
	int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd == -1)
		panic("socket");
	return server_fd;
}

int server_bind(int server_fd)
{
	unlink(SOCKET_PATH);
	//
	struct sockaddr_un server_addr;
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_PATH);
	if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
		panic("bind");
	return 0;
}

int server_listen(int server_fd)
{
	// Listen for incoming connections
	if (listen(server_fd, 1) == -1)
		panic("listen");

	printf("Server is listening...\n");

	return 0;
}

int server_accept(int server_fd)
{
	struct sockaddr_un server_addr, client_addr;
	socklen_t          client_len;
	client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
	return client_fd;
}

int server_read(int client_fd)
{
	char buffer[1024];
	// Receive data from the client
	bzero(buffer, 1024);
	int bytes_read = read(client_fd, buffer, sizeof(buffer));
	if (bytes_read == -1 && errno == EAGAIN)
		printf("EAGAIN\n");
	printf("Received data (size %d): %s\n", bytes_read, buffer);
	return bytes_read;
}

int server_write(int client_fd)
{
	// Send a response back to the client
	const char *response = "Server says: Hello, Client!";
	write(client_fd, response, strlen(response));
	return 0;
}

int epoll_add(int epoll_fd, int fd, uint32_t events)
{
	struct epoll_event event;
	event.events = events;
	event.data.fd = fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		return -1;
	}
	return 0;
}

int epoll_remove(int epoll_fd, int fd)
{
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0) == -1)
	{
		return -1;
	}
	return 0;
}

int server_disconnect(int epoll_fd, int fd)
{
	printf("Client disconnect %d\n", fd);
	epoll_remove(epoll_fd, fd);
	close(fd);
	return 0;
}

int server(int MAX_EVENTS)
{
	struct epoll_event events[MAX_EVENTS];
	int                server_fd, client_fd;

	int epoll_fd = epoll_create(1);

	server_fd = server_socket();
	setnonblocking(server_fd);
	server_bind(server_fd);

	epoll_add(epoll_fd, server_fd, EPOLLIN | EPOLLOUT | EPOLLET);

	server_listen(server_fd);

	printf("server_fd %d\n", server_fd);

	while (true)
	{
		int fds = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);

		for (int i = 0; i < fds; i++)
		{
			sleep(1);
			printf("epoll_wait fds %d i %d data.fd %d\n", fds, i, events[i].data.fd);
			if (events[i].data.fd == server_fd)
			{
				printf("waiting...\n");
				client_fd = server_accept(server_fd);
				setnonblocking(client_fd);

				if (client_fd == -1)
					panic("accept");

				printf("Client connected\n");
				printf("client_fd %d\n", client_fd);
				epoll_add(epoll_fd, client_fd, EPOLLIN | EPOLLOUT);
			}
			else if (events[i].events & (EPOLLRDHUP | EPOLLHUP))
			{
				server_disconnect(epoll_fd, events[i].data.fd);
			}
			else if (events[i].events & EPOLLIN)
			{
				int ret = server_read(events[i].data.fd);
				printf("read %d\n", ret);
				if (true)
				{
					printf("write\n");
					server_write(events[i].data.fd);
				}
			}
			else if (events[i].events & EPOLLOUT)
			{
				printf("EPOLLOUT\n");
				server_disconnect(epoll_fd, events[i].data.fd);
			}
		}
	}

	close(server_fd);
	unlink(SOCKET_PATH);
	return 0;
}

int client_socket()
{
	// Create a UNIX socket
	int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_fd == -1)
		panic("client_socket");
	return client_fd;
}

int client_connect(int client_fd)
{
	struct sockaddr_un server_addr;
	// Connect to the server
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_PATH);
	if (connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) ==
	    -1)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int client_write(int client_fd, char *data)
{
	write(client_fd, data, strlen(data));
	return 0;
}

int client_read(int client_fd)
{
	// Receive response from the server
	char buffer[1024];
	bzero(buffer, 1024);
	int bytes_read = read(client_fd, buffer, sizeof(buffer));
	printf("Received response: %s\n", buffer);
	return 0;
}

int client(int time)
{
	int client_fd = client_socket();

	client_connect(client_fd);

	sleep(time);

	client_write(client_fd, "Hello, Server!");

	client_read(client_fd);

	close(client_fd);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc <= 2)
	{
		printf("./bin server EVENTS\n");
		printf("or\n");
		printf("./bin client TIME\n");
		return 1;
	}

	int value = atoi(argv[2]);

	if (strcmp(argv[1], "client") == 0)
		client(value);
	else if (strcmp(argv[1], "server") == 0)
		server(value);
	else
		printf("arg err");
}
