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

#define RC_SWITCH_PROTOCOL	-4

/*
 * Utils
 */

uint16_t meif_message_crc16(struct meif_message *meif_message, void *data, int length)
{
	void *crc_data = NULL;
	int crc_data_length = 0;
	uint16_t message_length = 0;
	uint16_t crc = 0;
	uint8_t d = 0;
	int i;

	if(meif_message == NULL || data == NULL || length <= 0)
		return 0;

	crc_data_length = sizeof(meif_message->command) + sizeof(meif_message->length) + length;
	crc_data = calloc(1, crc_data_length);

	memcpy(crc_data, &(meif_message->command), sizeof(meif_message->command));
	message_length = meif_message->length + sizeof(uint16_t);
	memcpy((void *) crc_data + sizeof(meif_message->command),  &(message_length), sizeof(message_length));
	memcpy((void *) crc_data + sizeof(meif_message->command) + sizeof(message_length), data, length);

	for(i=0 ; i < crc_data_length ; i++) {
		d = *((uint8_t *) crc_data + i);
		crc += d;
	}

	free(crc_data);

	return crc;
}

/*
 * Message
 */

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

int meif_message_to_data(struct meif_message *meif_message, void **data_p)
{
	struct meif_header meif_header;
	void *data = NULL;
	int length = 0;
	uint16_t marker = 0;

	if(meif_message == NULL)
		return -1;

	memset((void *) &meif_header, 0, sizeof(meif_header));

	if(meif_message->data == NULL || meif_message->length < 0)
		meif_message->length = 0;

	meif_header.marker = MEIF_START;
	meif_header.command = meif_message->command;
	meif_header.length = meif_message->length + sizeof(marker);

	length = sizeof(struct meif_header) + meif_message->length + sizeof(marker);
	data = calloc(1, length);

	memcpy(data, (void *) &meif_header, sizeof(meif_header));
	if(meif_message->length > 0 && meif_message->data != NULL)
		memcpy(data + sizeof(meif_header), meif_message->data, meif_message->length);
	marker = MEIF_END;
	memcpy(data + sizeof(meif_header) + meif_message->length, &marker, sizeof(marker));

	*data_p = data;
	return length;
}

struct meif_message *meif_message_create(uint16_t command, void *data, uint16_t length)
{
	struct meif_message *meif_message = calloc(1, sizeof(struct meif_message));

	meif_message->command = command;
	meif_message->data = data;
	meif_message->length = length;

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
		case  MEIF_SEND_PATCH_MSG:
			command = "MEIF_SEND_PATCH_MSG";
			break;
		default:
			command = "MEIF_UNKNOWN_MSG";
			break;
	}

	printf("MEIF message: %s with %d bytes of data:\n", command, meif_message->length);
	if(meif_message->data != NULL && meif_message->length > 0)
		hex_dump(meif_message->data, meif_message->length);
}

int meif_message_send(struct meif_message *meif_message, int fd)
{
	void *data = NULL;
	int length = 0;
	int chunk = 160;
	int written = 0;
	int count = 0;
	int rc;
	int i;

	if(meif_message == NULL)
		return -1;

	length = meif_message_to_data(meif_message, &data);
	if(length < 0 || data == NULL) {
		return -1;
	}

	printf("Sending %d bytes!\n", length);
	meif_message_log(meif_message);
	printf("\n");

	while(written < length) {
		count = length - written < chunk ? length - written : chunk;
		rc = bcm4751_serial_write(fd, data + written, count);
		if(rc <= 0) {
			printf("Write failed, written %d/%d\n", written, length);
			free(data);
			return -1;
		}

		written += rc;
	}

	free(data);

	return 0;
}

/*
 * Send queue
 */

struct meif_send_queue meif_send_queue;

int meif_send_queued(struct meif_message *meif_message)
{
	struct meif_message **messages;
	int messages_count = 0;
	int index = 0;
	int count = 0;

	if(meif_message == NULL)
		return -1;

	// Save the previous data pointer and count
	messages = meif_send_queue.messages;
	messages_count = meif_send_queue.messages_count;

	if(messages_count < 0)
		messages_count = 0;

	// Index is the sync request index in the sync requests array
	index = messages_count;
	// Count is the total count of sync requests in the array
	count = index + 1;

	// Alloc the array with the new size
	meif_send_queue.messages = malloc(sizeof(struct meif_message *) * count);
	meif_send_queue.messages_count = count;

	// Copy and free previous data
	if(messages != NULL && messages_count > 0) {
		memcpy(meif_send_queue.messages, messages, sizeof(struct meif_message *) * messages_count);
		free(messages);
	}

	// Get the new data pointer and count
	messages = meif_send_queue.messages;
	messages_count = meif_send_queue.messages_count;

	// Put the sync request in the queue
	messages[index] = meif_message;

	return 0;
}

struct meif_message *meif_send_dequeue(void)
{
	struct meif_message *message;
	struct meif_message **messages;
	int messages_count = 0;
	int pos;
	int i;

	// Save the previous data pointer and count
	messages = meif_send_queue.messages;
	messages_count = meif_send_queue.messages_count;

	if(messages_count <= 0 || messages == NULL) {
		return NULL;
	}

	for(i=0 ; i < messages_count ; i++) {
		if(messages[i] != NULL) {
			message = messages[i];
			pos = i;
			break;
		}
	}

	if(message == NULL || pos < 0) {
		printf("Found no valid request, aborting!\n");
		return NULL;
	}

	// Empty the found position in the requests array
	messages[pos] = NULL;

	// Move the elements back
	for(i=pos ; i < messages_count-1 ; i++) {
		messages[i] = messages[i+1];
	}

	// Empty the latest element
	if(pos != messages_count-1) {
		messages[messages_count-1] = NULL;
	}

	messages_count--;

	if(messages_count == 0) {
		free(messages);
		meif_send_queue.messages = NULL;
	}

	 meif_send_queue.messages_count = messages_count;

	return message;
}

/*
 * Requests
 */

int meif_send_patch(int stage)
{
	struct meif_message *meif_message = NULL;
	void *data = NULL;
	int length = 0;
	void *fw_data = NULL;
	int fw_length = MEIF_PATCH_LENGTH;
	int fw_fd = -1;
	uint16_t marker = 0;
	int rc = -1;

	fw_fd = open(MEIF_PATCH_FILE, O_RDONLY);
	if(fw_fd < 0) {
		printf("Unable to open the bcm4751 patch!\n");
		return -1;
	}

	fw_data = calloc(1, fw_length);

	rc = read(fw_fd, fw_data, fw_length);
	if(rc < fw_length) {
		printf("Unable to read the bcm4751 patch contents (%d/%d)!\n", rc, fw_length);
		free(data);
		return -1;
	}

	close(fw_fd);

	if(stage == 1) {
		printf("Sending the first part of the patch...\n");

		length = sizeof(marker) + MEIF_PATCH_PART1_L + sizeof(marker);
		data = calloc(1, length);

		meif_message = meif_message_create(MEIF_SEND_PATCH_MSG, data, length);

		// Part #1
		marker = 1;
		memcpy(data, &marker, sizeof(marker));

		// Fw data
		memcpy((void *) data + sizeof(marker), (void *) fw_data + MEIF_PATCH_PART1_O, MEIF_PATCH_PART1_L);

		// Checksum (without the last 2 bytes that are the checksum)
		marker = meif_message_crc16(meif_message, data, length - sizeof(marker));
		memcpy((void *) data + sizeof(marker) + MEIF_PATCH_PART1_L, &marker, sizeof(marker));
	} else if(stage == 2) {
		printf("Sending the second part of the patch...\n");

		length = sizeof(marker) + MEIF_PATCH_PART2_L + sizeof(marker);
		data = calloc(1, length);

		meif_message = meif_message_create(MEIF_SEND_PATCH_MSG, data, length);

		// Part #2
		marker = 2;
		memcpy(data, &marker, sizeof(marker));

		// Fw data
		memcpy((void *) data + sizeof(marker), (void *) fw_data + MEIF_PATCH_PART2_O, MEIF_PATCH_PART2_L);

		// Checksum (without the last 2 bytes that are the checksum)
		marker = meif_message_crc16(meif_message, data, length - sizeof(marker));
		memcpy((void *) data + sizeof(marker) + MEIF_PATCH_PART2_L, &marker, sizeof(marker));
	}

	free(fw_data);

	// data is freed with meif_message_free
	if(meif_message == NULL) {
		printf("Unable to create the MEIF message!\n");

		if(data != NULL)
			free(data);

		return -1;
	}

	rc = meif_send_queued(meif_message);
	if(rc < 0) {
		meif_message_free(meif_message);
		return -1;
	}

	return 0;
}

/*
 * Dispatch
 */

int meif_dispatch(struct meif_message *meif_message)
{
	struct meif_config_values *config_values;
	struct meif_nack *nack;
	static int patch_send_stage = 0;

	switch(meif_message->command) {
		case MEIF_ACK_MSG:
			printf("Got an ACK message\n\n");

			if(patch_send_stage == 1) {
				patch_send_stage = 2;
				meif_send_patch(patch_send_stage);
			} else if(patch_send_stage == 2) {
				patch_send_stage = 3;
				printf("Ready to switch protocol!\n");

				return RC_SWITCH_PROTOCOL;
			}
			break;
		case MEIF_NACK_MSG:
			printf("Got a NACK message\n");

			if(meif_message->data != NULL && meif_message->length >= sizeof(struct meif_nack)) {
				nack = (struct meif_nack *) meif_message->data;
				switch(nack->reason) {
					case MEIF_NACK_GARBAGE_RECEIVED:
						printf("Reason is: MEIF_NACK_GARBAGE_RECEIVED\n");
						break;
					case MEIF_NACK_CHECKSUM_ERROR:
						printf("Reason is: MEIF_NACK_CHECKSUM_ERROR\n");
						if(patch_send_stage > 0) {
							patch_send_stage = 0;
							printf("Patch send request failed, aborting!\n");
						}
						break;
					default:
						printf("Reason is: MEIF_NACK_UNKNOWN\n");
						break;
				}
			}

			printf("\n");

			break;
		case MEIF_STATE_REPORT_MSG:
			printf("Got a STATE_REPORT message\n\n");
			break;
		case MEIF_CONFIG_VALUES_MSG:
			if(meif_message->data != NULL && meif_message->length >= sizeof(struct meif_config_values)) {
				config_values = (struct meif_config_values *) meif_message->data;
				printf("Got config values:\n\tvendor: %s\n\tproduct: %s\n\n", config_values->vendor, config_values->product);
			}

			patch_send_stage = 1;
			meif_send_patch(patch_send_stage);
			break;
	}

	return 0;
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

	// Init send queue
	memset(&meif_send_queue, 0, sizeof(meif_send_queue));

	while(run) {
		meif_message = NULL;
		meif_header = NULL;
		marker = 0;

		if(meif_data == NULL && meif_data_waiting == NULL) {
			meif_message = meif_send_dequeue();
			if(meif_message != NULL) {
				meif_message_send(meif_message, fd);
				meif_message_free(meif_message);
				meif_message = NULL;
			}
		}

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
			rc = meif_dispatch(meif_message);
			if(rc < 0)
				run = 0;
			meif_message_free(meif_message);
		}
	}

	switch(rc) {
		case RC_SWITCH_PROTOCOL:
			bcm4751_switch_protocol(fd);
			break;
	}
}
