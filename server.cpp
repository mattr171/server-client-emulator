#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

/**
 * Write method for safely writing to socket
 *
 * Identical implementation to client.cpp (see for detailed description)
 *
 * @param fd File descriptor for socket
 * @param buf Pointer to an array of characters to write to socket
 * @param len Number of bytes to write
 *
 * @return static ssize_t 0 if function executed properly, -1 if not
 ********************************************************************************/
static ssize_t safe_write(int fd, const char *buf, size_t len)
{
	while (len > 0)
	{
		ssize_t wlen = write(fd, buf, len);
		if (wlen == -1)
			return -1;

		len -= wlen;
		buf += wlen;
	}

	return 0;
}

/**
 * Main function for executing server program
 *
 * @param argc Command line argument count
 * @param argv Command line arguments
 *
 * @return int 0 indicating successful execution
 ********************************************************************************/
int main(int argc, char *argv[])
{
	int opt;
	int sock, msgsock;
	socklen_t length;
	struct sockaddr_in server;
	char buf[2048];
	int rval;

	unsigned int listener_port = 0;
	uint16_t checksum = 0; ///< Running sum of each byte read from client
	uint32_t byte_count = 0;

	/* Parse command line arguments and set listener
		   port for -l, display usage statement for bad
	   argument */
	while ((opt = getopt(argc, argv, "l:")) != -1)
	{
		switch (opt)
		{
		case 'l':
		{
			listener_port = atoi(optarg);
			break;
		}

		case '?':
		{
			std::cerr << "Usage: server [-l listener-port]" << std::endl;
			std::cerr << "\t-l Specify port number to which the server must listen." << std::endl;
			break;
		}

		default:
			break;
		}
	}

	/* Ignore broken pipes */
	signal(SIGPIPE, SIG_IGN);

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("opening stream socket");
		exit(1);
	}

	/* Name socket using wildcards */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(listener_port);
	if (bind(sock, (sockaddr *)&server, sizeof(server)))
	{
		perror("binding stream socket");
		exit(1);
	}

	/* Find out assigned port number and print it out */
	length = sizeof(server);
	if (getsockname(sock, (sockaddr *)&server, &length))
	{
		perror("getting socket name");
		exit(1);
	}
	std::cout << "Socket has port #" << ntohs(server.sin_port) << std::endl;

	/* Start accepting connections */
	listen(sock, 5);
	do
	{
		struct sockaddr_in from;
		socklen_t from_len = sizeof(from);
		msgsock = accept(sock, (struct sockaddr *)&from, &from_len);

		if (msgsock == -1)
			perror("accept");
		else
		{
			/* Display connecting IP and port */
			inet_ntop(from.sin_family, &from.sin_addr, buf, sizeof(buf));
			std::cout << "Accepted connection from " << buf << ", port " << ntohs(from.sin_port) << std::endl;

			do
			{
				if ((rval = read(msgsock, buf, sizeof(buf))) < 0)
					perror("reading stream message");

				if (rval == 0)
					std::cout << "Ending connection" << std::endl;
				else
				{
					/* Sum each byte in current buffer,
					   increment byte_count */
					for (int i = 0; i < rval; i++)
					{
						checksum += (uint8_t)buf[i];
						byte_count++;
					}
				}
			} while (rval != 0);

			/* Write response message to client,
			   ostringstream object converted to string
						   converted to c_string */
			std::ostringstream os;
			os << "Sum: " << checksum << " Len: " << byte_count << '\n';
			std::string str = os.str();
			const char *ch = str.c_str();
			if (safe_write(msgsock, ch, sizeof(str)) < 0)
				perror("write failed");

			/* Zero out byte_count and checksum for next client */
			byte_count = 0;
			checksum = 0;

			close(msgsock);
		}
	} while (true);

	return 0;
}
