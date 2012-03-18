// Copyright (C) 2012 Paul Kocialkowski, contact@paulk.fr, GNU GPLv3+
// BCM4751 daemon code

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cutils/sockets.h>

int main(void)
{
	int fd;
	int cfd;
	int rc;

	int clen;
	char buf[50];
	char status[] = {
		0x08, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	struct sockaddr_un caddr;

	fd = socket_local_server("gps", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_SEQPACKET);
	listen(fd, 1);
	printf("socket_local_server passed: %d\n", fd);

	cfd = accept(fd, 0, &clen);
	printf("accept passed: %d\n", cfd);

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(cfd, &fds);

	while(1)
	{
		memset(buf, 0, sizeof(buf));
		select(cfd+1,  &fds, NULL, NULL, NULL);
		rc = read(cfd, buf, 50);
	
		printf("read %d bytes!\n", rc);

		if(rc == 50) {
			write(cfd, status, sizeof(status));
			printf("wrote %d bytes!\n", sizeof(status));
		}
	}

	return 0;
}
