#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

class Server
{
private:
	int _serverSocket;
	struct sockaddr_in _serverAddress;

public:
	Server(uint16_t port)
	{
		_serverAddress.sin_family = AF_INET;
		_serverAddress.sin_port = htons(port);
		_serverAddress.sin_addr.s_addr = INADDR_ANY;
		_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	}
	~Server()
	{
	}

	int getSocket()
	{
		return _serverSocket;
	}

	int bind()
	{
		::bind(_serverSocket, (struct sockaddr *)&_serverAddress, sizeof(_serverAddress));
		return 0;
	}

	int listen()
	{
		::listen(_serverSocket, SOMAXCONN);
		return 0;
	}

	int acceptConnection()
	{
		struct sockaddr_in clientAddr;
		socklen_t clientAddrLen = sizeof(clientAddr);

		std::cout << "Waiting for client connections..." << std::endl;
		int newClientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
		return newClientSocket;
	}

	bool nonblocking()
	{
		int flags = fcntl(_serverSocket, F_GETFL, 0);
		if (flags == -1)
			return false;
		return (fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) == 0) ? true : false;
	}
};