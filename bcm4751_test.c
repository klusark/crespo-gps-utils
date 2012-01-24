/*
 *   Copyright (C) 2012 Paul Kocialkowski <contact@paulk.fr>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <termio.h>
#include <errno.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

int wake_lock_fd = -1;
int wake_unlock_fd = -1;

int wake_lock(char *lock_name, int size)
{
    int rc = 0;

    if(wake_lock_fd < 0)
        wake_lock_fd = open("/sys/power/wake_lock", O_RDWR);

    rc = write(wake_lock_fd, lock_name, size);

    return rc;
}

int wake_unlock(char *lock_name, int size)
{
    int rc = 0;

    if(wake_lock_fd < 0)
        wake_unlock_fd = open("/sys/power/wake_unlock", O_RDWR);

    rc = write(wake_unlock_fd, lock_name, size);

    return rc;
}

void gpio_write_ascii(char *gpio_path, char *value)
{
	int fd;
	fd = open(gpio_path, O_RDWR);

	if(fd < 0)
		return;

	write(fd, value, strlen(value));

	close(fd);
}

int main(int argc, char *argv[])
{
	char *gpio_standby_path = "/sys/class/sec/gps/GPS_PWR_EN/value";
	char *gpio_reset_path = "/sys/class/sec/gps/GPS_nRST/value";
	unsigned char *data;
	unsigned char init_val[21];
	int serial_fd;
	int rc;
	int i;

	struct termios termios;
	struct timeval timeout;
	int serial;
	fd_set fds;

	if(argc > 1) {
		if(strcmp(argv[1], "stop") == 0) {
			printf("Stopping it\n");
			wake_unlock("gpsd-interface", 14);
			gpio_write_ascii(gpio_standby_path, "0\n");

			return 0;
		}
	}

	printf("Wake lock on gpsd-interface\n");

	wake_lock("gpsd-interface", 14);

	printf("GPIO 1 write on %s\n", gpio_standby_path);
	gpio_write_ascii(gpio_standby_path, "1\n");

	usleep(250000);

	printf("Opening /dev/s3c2410_serial1\n");
	serial_fd = open("/dev/s3c2410_serial1", O_RDWR|O_NOCTTY);

	if(serial_fd < 0) {
		printf("Serial fd is wrong, aborting\n");
		return 1;
	}

	tcgetattr(serial_fd, &termios);
//	ioctl(serial_fd, TIOCMGET, &serial);
    ioctl(serial_fd, TCFLSH, 0x2);

	cfmakeraw(&termios);
	cfsetispeed(&termios, B115200);
	cfsetospeed(&termios, B115200);

    tcsetattr(serial_fd, TCSANOW, &termios);
//	ioctl(serial_fd, TIOCMSET, &serial);
    ioctl(serial_fd, TCFLSH, 0x2);

	//ioctl(serial_fd, TIOCMGET, &serial);
	tcgetattr(serial_fd, &termios);

//	usleep(250000);

	FD_ZERO(&fds);
	FD_SET(serial_fd, &fds);

	memset(init_val, 0x80, 21);
	data = malloc(512);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

	printf("Writing init bits\n");
	rc = write(serial_fd, init_val, 20);
	printf("Written %d bytes!\n",rc);

	for(i=0 ; i < 3 ; i++) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		rc = read(serial_fd, data, 512);
		printf("read %d bytes!\n", rc);

		if(rc < 0) {
			wake_unlock("gpsd-interface", 14);
			gpio_write_ascii(gpio_standby_path, "0\n");
			return 0;
		}
		hex_dump(data, rc);
		break;
		write(serial_fd, init_val, 20);
	}

	for(i=0 ; i < 3 ; i++) {
			rc = read(serial_fd, data, 512);
			printf("read %d bytes!\n", rc);

			if(rc < 0) {
				perror("Here comes the error");

			} else
    			hex_dump(data, rc);

    	usleep(250000);
    }

	wake_unlock("gpsd-interface", 14);
	gpio_write_ascii(gpio_standby_path, "0\n");

    return 0;

	printf("Writing MEIF init bits\n");

	unsigned char meif_init_bits[9] = {
	0xff, 0x00, 0xfd, 0x40, 0x00, 0x00, 0xe6, 0xfc
	};

	write(serial_fd, meif_init_bits, 8);


	while(1) {
		FD_ZERO(&fds);
		FD_SET(serial_fd, &fds);

		timeout.tv_sec = 1;
		timeout.tv_usec = 499000;

		rc = read(serial_fd, data, 512);
		printf("read %d bytes!\n");
		hex_dump(data, rc);

	}

	free(data);

	return 0;
}
