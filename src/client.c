#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>



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
