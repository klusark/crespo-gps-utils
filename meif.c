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
#include <unistd.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "meif.h"

void meif_read_loop(int fd)
{
	void *data = NULL;
	int length = 0x100;

	uint16_t *meif_data_waiting = NULL;
	int meif_data_waiting_length = 0;

	struct meif_message *meif_message = NULL;
	struct meif_header *meif_header = NULL;
	uint16_t *meif_data = NULL;
	int meif_data_length = 0;
	uint16_t marker = 0;

	int run = 1;
	int rc = -1;
	int i;

	data = malloc(length);

	while(run) {
		meif_header = NULL;
		marker = 0;

		if(meif_data == NULL) {
			rc = bcm4751_serial_read(fd, data, length, NULL);
			if(rc <= 0) {
				free(data);
				return;
			}
		} else if(meif_data_length > 0) {
			if(meif_data_length > length)
				meif_data_length = length;

			memcpy(data, (void *) meif_data, meif_data_length);
			rc = meif_data_length;

			free(meif_data);
			meif_data = NULL;
			meif_data_length = 0;
		}

		if(meif_data_waiting == NULL) {
			// Read the start marker
			for(i=0 ; i < rc ; i++) {
				marker = *((uint16_t *) (data + i));
				if(marker == MEIF_START) {
					// Raw MEIF data and header
					meif_data_length = rc - i;
					meif_data = (uint16_t *) malloc(meif_data_length);
					memcpy((void *) meif_data, (void *) (data + i), meif_data_length);
					meif_header = (struct meif_header *) meif_data;
					break;
				}
			}
		} else if(meif_data_waiting_length > 0) {
			// There is an uncomplete message waiting

			// Raw data and header from the waiting and the new data
			meif_data_length = meif_data_waiting_length + rc;
			meif_data = malloc(meif_data_length);
			memcpy((void *) meif_data, (void *) meif_data_waiting, meif_data_waiting_length);
			memcpy((void *) meif_data + meif_data_waiting_length, (void *) data, rc);
			meif_header = (struct meif_header *) meif_data;

			free(meif_data_waiting);
			meif_data_waiting = NULL;
			meif_data_waiting_length = 0;
		}

		if(meif_data == NULL || meif_header == NULL) {
			printf("No usable data found, skipping!\n");
			continue;
		}

		// The message wasn't fully retrieved
		if(meif_data_length < sizeof(struct meif_header) || (meif_data_length - i - sizeof(struct meif_header)) < meif_header->length) {
			meif_data_waiting = meif_data;
			meif_data_waiting_length = meif_data_length;
			meif_data = NULL;
			meif_data_length = 0;

			continue;
		} else {
			// Read the end marker
			for(i=meif_header->length ; i < meif_data_length ; i++) {
				// We go with a 6 left offset from the announced end of the message
				marker = *((uint16_t *) ((void *) meif_data + i));
				if(marker == MEIF_END) {
					// TODO: convert to meif_message
					printf("MEIF message (%d bytes):\n", meif_data_length);
					hex_dump(meif_data, i + sizeof(marker));

					break;
				}
			}

			// No end marker was found
			if(i == meif_data_length) {
				// meif_data and meif_data_length are still set, so they will be used
				continue;
			}

			// There is still data after the end of the message
			if(meif_data_length - i - 1 >= sizeof(marker)) {
				meif_data_waiting_length = meif_data_length - i - 2;
				meif_data_waiting = (uint16_t *) malloc(meif_data_waiting_length);
				memcpy((void *) meif_data_waiting, (void *) meif_data + i + 2, meif_data_waiting_length);

				free(meif_data);
				meif_data = meif_data_waiting;
				meif_data_length = meif_data_waiting_length;
			} else {
				free(meif_data);
				meif_data = NULL;
				meif_data_length = 0;
			}

			meif_data_waiting = NULL;
			meif_data_waiting_length = 0;
		}

		// TODO: if there is a meif message, dispatch and free and set to NULL
	}
}
