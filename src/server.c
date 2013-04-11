#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

/* size of the buffer for one message */
#define MSG_BUF_SIZE 1500
/* length for address string */
#define ADDR_STR_LEN 100



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
