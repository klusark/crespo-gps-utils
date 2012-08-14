/**
 * This file is part of BCM4751 gpsd
 *
 * Copyright (C) 2011 Paul Kocialkowski <contact@oaulk.fr>
 *
 * BCM4751 gpsd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BCM4751 gpsd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BCM4751 gpsd.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "meif.h"

/*
 * Utils
 */

void hex_dump(void *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
     * [0000] 75 6E 6B 6E 6F 77 6E 20
     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
     * (in a single line of course)
     */

    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }

        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) {
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

/*
 * SEC
 */

int sec_gps_power(int power)
{
	char gpio_power_path[] = "/sys/class/sec/gps/GPS_PWR_EN/value";
	char gpio_on_value[] = "1\n";
	char gpio_off_value[] = "0\n";
	int fd = -1;
	int rc;

	fd = open(gpio_power_path, O_RDWR);
	if(fd < 0)
		return -1;

	if(power) {
		rc = write(fd, gpio_on_value, strlen(gpio_on_value));
	} else {
		rc = write(fd, gpio_off_value, strlen(gpio_off_value));
	}

	if(rc <= 0) {
		close(fd);
		return -1;
	}

	close(fd);

	usleep(250000);

	return 0;
}

int sec_gps_reset(int value)
{
	char gpio_reset_path[] = "/sys/class/sec/gps/GPS_nRST/value";
	char gpio_on_value[] = "1\n";
	char gpio_off_value[] = "0\n";
	int fd = -1;
	int rc;

	fd = open(gpio_reset_path, O_RDWR);
	if(fd < 0)
		return -1;

	if(value) {
		rc = write(fd, gpio_on_value, strlen(gpio_on_value));
	} else {
		rc = write(fd, gpio_off_value, strlen(gpio_off_value));
	}

	if(rc <= 0) {
		close(fd);
		return -1;
	}

	close(fd);

	usleep(250000);

	return 0;
}

/*
 * BCM4751
 */

int bcm4751_serial_open(void)
{
	struct termios termios;
	int fd = -1;

	// TODO: add more checks

	fd = open("/dev/s3c2410_serial1", O_RDWR|O_NOCTTY|O_NONBLOCK);
	if(fd < 0)
		return -1;

	tcgetattr(fd, &termios);

	// Flush
	ioctl(fd, TCFLSH, 0x2);

	cfmakeraw(&termios);
	cfsetispeed(&termios, B115200);
	cfsetospeed(&termios, B115200);

	// This is the magic to contact the chip
	termios.c_cflag = 0x800018b2;

	tcsetattr(fd, TCSANOW, &termios);

	// Flush
	ioctl(fd, TCFLSH, 0x2);

	return fd;
}

int bcm4751_serial_read(int fd, void *data, int length, struct timeval *timeout)
{
	fd_set fds;
	int rc = -1;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	rc = select(fd + 1, &fds, NULL, NULL, timeout);
	if(rc > 0 && length > 0) {
		rc = read(fd, data, length);
	}

	return rc;
}

int bcm4751_serial_write(int fd, void *data, int length)
{
	return write(fd, data, length);
}

int bcm4751_autobaud(int fd)
{
	struct timeval timeout;
	uint8_t autobaud[20];
	int ready = 0;
	int rc = -1;

	memset(autobaud, 0x80, sizeof(autobaud));

	// TODO: limit the number of attempts

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	while(!ready) {
		bcm4751_serial_write(fd, autobaud, sizeof(autobaud));

		rc = bcm4751_serial_read(fd, NULL, 0, &timeout);
		if(rc > 0)
			ready = 1;
	}

	return 0;
}

void bcm4751_read_dump(int fd)
{
	void *data = NULL;
	int length = 0x100;
	int rc = -1;

	data = malloc(length);

	while(1) {
		rc = bcm4751_serial_read(fd, data, length, NULL);
		if(rc > 0) {
			printf("Read %d bytes:\n", rc);
			hex_dump(data, rc);
		} else {
			free(data);
			return;
		}
	}
}

void bcm4751_switch_protocol(int fd)
{
	uint8_t data1[] = { 0xff, 0x00, 0xfd, 0x40, 0x00, 0x00, 0xe6, 0xfc };
	uint8_t data2[] = { 0xfe, 0x00, 0xfd, 0x6f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb2, 0xfc };
	uint8_t data3[] = { 0xfe, 0x00, 0xfd, 0x6f, 0x3a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x34, 0xfc };

	sleep(1);

	printf("Sending unknown bytes!\n");
	bcm4751_serial_write(fd, data1, sizeof(data1));
	bcm4751_serial_write(fd, data2, sizeof(data2));
	bcm4751_serial_write(fd, data3, sizeof(data3));

	bcm4751_read_dump(fd);
}

int main(void)
{
	int fd = -1;
	int rc = -1;

	printf("Turning the GPS on...\n");
	rc = sec_gps_power(1);
	if(rc < 0) {
		printf("Failed to turn the GPS on!\n");
		return 1;
	}

	printf("Opening the GPS serial...\n");
	fd = bcm4751_serial_open();
	if(fd < 0) {
		printf("Failed to open the GPS serial!\n");
		return 1;
	}

	printf("Sending autobaud...\n");
	rc = bcm4751_autobaud(fd);
	if(fd < 0) {
		printf("Failed to send autobaud!\n");
		return 1;
	}

	meif_read_loop(fd);

	return 0;
}
