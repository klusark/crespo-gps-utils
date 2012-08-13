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

struct meif_message *meif_message_from_data(void *data, int length)
{
	struct meif_message *meif_message = NULL;
	struct meif_header *meif_header = NULL;
	void *message_data = NULL;
	int message_data_length = 0;
	uint16_t marker = 0;

	if(data == NULL || length < sizeof(struct meif_header))
		return NULL;

	meif_header = (struct meif_header *) data;

	marker = meif_header->marker;
	if(marker != MEIF_START)
		return NULL;

	marker = *((uint16_t *) ((uint8_t *) data + length - sizeof(marker)));
	if(marker != MEIF_END)
		return NULL;

	meif_message = calloc(1, sizeof(struct meif_message));
	message_data_length = length - sizeof(struct meif_header) - sizeof(marker);
	if(message_data_length > 0) {
		message_data = calloc(1, message_data_length);
		memcpy(message_data, (void *) ((uint8_t *) data + sizeof(struct meif_header)), message_data_length);

		meif_message->length = message_data_length;
		meif_message->data = message_data;
	}

	meif_message->command = meif_header->command;

	return meif_message;
}

void meif_message_free(struct meif_message *meif_message)
{
	if(meif_message == NULL)
		return;

	if(meif_message->data != NULL)
		free(meif_message->data);

	memset(meif_message, 0, sizeof(struct meif_message));
	free(meif_message);
}

void meif_message_log(struct meif_message *meif_message)
{
	char *command = NULL;

	if(meif_message == NULL)
		return;

	switch(meif_message->command) {
		case MEIF_ACK_MSG:
			command = "MEIF_ACK_MSG";
			break;
		case MEIF_NACK_MSG:
			command = "MEIF_NACK_MSG";
			break;
		case MEIF_STATE_REPORT_MSG:
			command = "MEIF_STATE_REPORT_MSG";
			break;
		case MEIF_CONFIG_VALUES_MSG:
			command = "MEIF_CONFIG_VALUES_MSG";
			break;
		default:
			command = "MEIF_UNKNOWN_MSG";
			break;
	}

	printf("MEIF message: %s with %d bytes of data:\n", command, meif_message->length);
	if(meif_message->data != NULL && meif_message->length > 0)
		hex_dump(meif_message->data, meif_message->length);
}

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
		meif_message = NULL;
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

		printf("Read %d bytes\n", rc);

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
					meif_message = meif_message_from_data((void *) meif_data, i + sizeof(marker));
					if(meif_message == NULL)
						printf("Failed to create a MEIF message!\n");
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

		if(meif_message != NULL) {
			meif_message_log(meif_message);
			// TODO: dispatch
			meif_message_free(meif_message);
		}
	}
}
