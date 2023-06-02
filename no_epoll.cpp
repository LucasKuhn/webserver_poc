#include "Server.cpp"

#define PORT 8000
#define MAX_CLIENTS 500
#define BUFFER_SIZE 1024

int main()
{
	Server server(PORT);

	server.bind();
	server.nonblocking();
	server.listen();

	int clientSockets[MAX_CLIENTS] = {0};
	while (true)
	{
		int newClientSocket = server.acceptConnection(); // Non blocking

		if (newClientSocket == -1)
		{
			// No pending connections, continue with other operations
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				continue;
			}
			else
			{
				std::cerr << "Failed to accept client connection" << std::endl;
				continue;
			}
		}

		// Adicione a conexão em um slot vazio da lista de conexões
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientSockets[i] == 0)
			{
				std::cout << "New client: " << newClientSocket << " at slot "
				          << i << std::endl;
				clientSockets[i] = newClientSocket;
				break;
			}
			std::cerr << "Max clients reached" << std::endl;
		}

		// Lida com a lista de conexões
		char buffer[BUFFER_SIZE];
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			int clientSocket = clientSockets[i];
			if (clientSocket != 0)
			{
				int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
				if (bytesRead <= 0) // Deu erro
				{
					std::cout << "Client disconnected: " << clientSocket
					          << std::endl;
					close(clientSocket);
					clientSockets[i] = 0;
					continue;
				}

				std::cout << "Received data: " << std::string(buffer, bytesRead)
				          << std::endl;
				if (bytesRead < BUFFER_SIZE) // Leu tudo, manda a resposta
				{
					std::cout << "Sending response" << std::endl;
					std::string response = "HTTP/1.1 200 OK\r\nContent-Type: "
					                       "text/plain\r\nContent-Length: "
					                       "3\r\n\r\nOK\n\r\n";
					send(clientSocket, response.c_str(), response.size(), 0);
					close(clientSocket);
					clientSockets[i] = 0;
				}
				else // Não leu tudo, continua lendo
				{
				}
			}
		}
	}
}
