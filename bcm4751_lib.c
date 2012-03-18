// Copyright (C) 2012 Paul Kocialkowski, contact@paulk.fr, GNU GPLv3+
// BCM4751 lib code

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

/*
535   socket(PF_UNIX, SOCK_SEQPACKET, 0) = 3
535   connect(3, {sa_family=AF_UNIX, path="/dev/socket/gps"}, 110) = 0

535   write(3, "\x08\x00\x00\x00\x6b\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x14\x00\x00\x00\x03\x00\x00\x00\xff\xff\xff\xff"..., 44) = 44
535   write(3, "\x08\x00\x00\x00\x6c\x00\x00\x00\x0c\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 24) = 24
535   write(3, "\x08\x00\x00\x00\x17\x00\x00\x00\x0c\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00", 24) = 24
535   write(3, "\x08\x00\x00\x00\x08\x00\x00\x00\x30\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00"..., 60) = 60
*/
int main(void)
{
	int fd = socket_local_client("gps", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_SEQPACKET);
	if(fd < 0) {
		printf("Socket is dead!\n");
		return 1;
	}

	char data1[] = {
		0x08, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x05, 0x93, 0xd2, 0xaf, 0x14, 0xcc, 0xc0, 0xbe
	};
	char data2[] = {
		0x08, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	char data3[] = {
		0x08, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	char data4[] = {
		0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
	};
/*
	write(fd, data1, sizeof(data1));
	usleep(500000);
	write(fd, data2, sizeof(data2));
	usleep(500000);
	write(fd, data3, sizeof(data3));
	usleep(500000);
*/
	// Ask GPSD to start obtaining data from the chip
	write(fd, data4, sizeof(data4));
	usleep(500000);

	sleep(60);
	
	return 0;
}