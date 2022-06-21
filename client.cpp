#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * Write method for safely writing to socket
 *
 * This function begins a loop while there are still bytes to send
 * through the socket. The write() method is called using the function's
 * parameters, and checks for error code -1. The wlen variable is then
 * subtracted from len to account for the number of bytes read and
 * wlen is added to the buf pointer. Upon successful execution, 0 is
 * returned.
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

		len -= wlen; // reduce the remaining number of bytes to send
		buf += wlen; // advance the buffer pointer past the written data
	}

	return 0;
}

/**
 * Print server response
 *
 * This function enters a read loop while there is still data to be read.
 * Data is read from the socket into a char array buffer. If the read
 * encounters an error (-1), the function returns an error code. Otherwise,
 * the contents of the buffer are sent to stdout.
 *
 * @param fd File descriptor for socket to read from
 *
 * @return int 0 upon successful execution, -1 to indicate an error
 ********************************************************************************/
static int print_response(int fd)
{
	char buf[1024];

	int rval = 1;
	while (rval > 0)
	{
		if ((rval = read(fd, buf, sizeof(buf))) == -1)
		{
			perror("reading stream message");
			return -1;
		}
		else if (rval > 0)
			std::cout << buf;
	}

	return 0;
}

/**
 * Usage function for client
 ********************************************************************************/
void usage()
{
	std::cerr << "Usage: client [-s server-ip] server-port" << std::endl;
	std::cerr << "\t-s Specify server's IPv4 number." << std::endl;
	std::cerr << "\tserver-port: Server port number to which client must connect." << std::endl;
	exit(1);
}

/**
 * Main function for executing client program
 *
 * @param argc Command line argument count
 * @param argv Command line arguments
 *
 * @return int 0 indicating successful execution
 ********************************************************************************/
int main(int argc, char *argv[])
{
	int opt;
	int sock;
	char buf[2048];
	char *server_ip;
	struct sockaddr_in server;		 // socket address for server connection
	char default_ip[] = "127.0.0.1"; // default IP set to 127.0.0.1

	/* Parse command line arguments and set server ip
	   for -s, call usage function for bad argument */
	while ((opt = getopt(argc, argv, "s:")) != -1)
	{
		switch (opt)
		{
		case 's':
		{
			server_ip = optarg;
			break;
		}

		case '?':
		{
			usage();
			break;
		}

		default:
			break;
		}
	}

	/* Set server IP to default if one has not been given */
	if (server_ip == NULL)
		server_ip = default_ip;

	/* Error out with usage if required port is not provided */
	if (argv[optind] == NULL)
		usage();

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("opening stream socket");
		exit(1);
	}

	/* Connect socket using name specified by command line (if one was specified). */
	server.sin_family = AF_INET;
	if (inet_pton(AF_INET, server_ip, &server.sin_addr) <= 0)
	{
		std::cerr << server_ip << ": invalid address/format" << std::endl;
		exit(2);
	}
	server.sin_port = htons(atoi(argv[optind]));

	/* Attempt to connect to server */
	if (connect(sock, (sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("connecting stream socket");
		exit(1);
	}

	/* Read client input from stdin */
	ssize_t len;
	do
	{
		len = read(fileno(stdin), buf, sizeof(buf));

		if (len < 0)
			perror("reading to stream socket");

		if (safe_write(sock, buf, len) < 0)
			perror("writing on stream socket");
	} while (len != 0);

	/* Partially close  socket after read */
	shutdown(sock, SHUT_WR);

	print_response(sock);

	close(sock);

	return 0;
}
