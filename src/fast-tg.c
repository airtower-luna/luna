#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>

/* default server port (can be changed by -p command line argument) */
#define DEFAULT_PORT 4567

/* size of the buffer for one message */
#define MSG_BUF_SIZE 1500
/* length for address string */
#define ADDR_STR_LEN 100

/* exit code for invalid command line arguments */
#define EXIT_INVALID 1



int run_client(struct sockaddr_in6 *addr)
{
	int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1)
		perror("Error while creating socket");

	if (connect(sock, (struct sockaddr *) addr, sizeof(*addr)) == -1)
		perror("Error setting destination");

	int seq = 0;
	for (int i = 0; i < 5; i++)
	{
		seq = htonl(i);
		if (send(sock, &seq, sizeof(seq), 0) == -1)
			perror("Error while sending");
	}

	close(sock);
}



int run_server(struct sockaddr_in6 *addr)
{
	int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1)
		perror("Error while creating socket");

	if (bind(sock, (struct sockaddr *) addr, sizeof(*addr)) == -1)
		perror("Error binding port");

	size_t buflen = MSG_BUF_SIZE;
	void *buf = malloc(buflen);
	ssize_t recvlen = 0;
	int seq = 0;
	int addrbuf_size = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 *addrbuf = malloc(addrbuf_size);
	socklen_t addrlen = 0;
	char *addrstr = malloc(ADDR_STR_LEN);
	uint16_t sourceport = 0;

	int work = 1; // could be changed by SIGTERM later
	while (work)
	{
		addrlen = addrbuf_size;
		recvlen = recvfrom(sock, buf, buflen, 0,
				   (struct sockaddr *) addrbuf, &addrlen);
		if (addrlen > addrbuf_size)
			fprintf(stderr, "recv: addr buffer too small!\n");
		inet_ntop(AF_INET6, &(addrbuf->sin6_addr), addrstr, ADDR_STR_LEN);
		// TODO: error check
		sourceport = ntohs(addrbuf->sin6_port);

		seq = ntohl(*((int *) buf));
		printf("Received packet %i (%i bytes) from %s, port %i.\n",
		       seq, (int) recvlen, addrstr, sourceport);
	}

	free(addrbuf);
	free(addrstr);
	free(buf);
	close(sock);
}



int main(int argc, char *argv[])
{
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(DEFAULT_PORT);
	addr.sin6_addr = in6addr_loopback;

	int opt = getopt(argc, argv, "sc:p:");
	int server = 0;
	int client = 0;
	while (opt != -1)
	{
		switch (opt)
		{
		case 'p':
			addr.sin6_port = htons(atoi(optarg));
			break;
		case 's': // act as server
			if (client != 0)
			{
				fprintf(stderr, "Select client or server mode, never both!\n");
				exit(EXIT_INVALID);
			}
			server = 1;
			addr.sin6_addr = in6addr_any;
			break;
		case 'c': // act as client
			if (server != 0)
			{
				fprintf(stderr, "Select client or server mode, never both!\n");
				exit(EXIT_INVALID);
			}
			// TODO: evaluate optarg into server address
			client = 1;
			break;
		default:
			break;
		}
		opt = getopt(argc, argv, "scp:");
	}

	if (client)
		return run_client(&addr);
	if (server)
		return run_server(&addr);
}
