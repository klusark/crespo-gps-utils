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

#ifndef _MEIF_H_
#define _MEIF_H_

#define MEIF_START	0xA5 + (0x93 << 8)
#define MEIF_END	0xA5 + (0x68 << 8)

/*
 * Commands
 */

#define MEIF_ACK_MSG		0x0000
#define MEIF_NACK_MSG		0x0001
//#define MEIF_ERROR_MSG	0x0002 or 0x0003
#define MEIF_STATE_REPORT_MSG	0x0004
#define MEIF_CONFIG_VALUES_MSG	0x0008
#define MEIF_SEND_PATCH_MSG	0x0010

/*
 * Data
 */

#define MEIF_NACK_GARBAGE_RECEIVED	0x0003
#define MEIF_NACK_CHECKSUM_ERROR	0x0102

/*
 * Structures
 */

struct meif_header {
	uint16_t marker;
	uint16_t command;
	uint16_t length;
} __attribute__((__packed__));

struct meif_message {
	uint16_t command;
	uint16_t length;
	void *data;
} __attribute__((__packed__));

struct meif_nack {
	uint16_t seq;
	uint16_t reason;
	uint16_t unknown;
} __attribute__((__packed__));

struct meif_config_values {
	uint16_t seq;
	uint8_t unknown1[34];
	char vendor[16];
	char product[16];
	uint8_t unknown2[2];
} __attribute__((__packed__));

struct meif_send_queue {
	struct meif_message **messages;
	int messages_count;
};

struct meif_patch_part_info {
	int offset;
	int length;
};

struct meif_patch_info {
	char *product;
	char *patch_file;
	int patch_fd;
	int parts_count;
	struct meif_patch_part_info parts[];
};

/*
 * Functions
 */

void meif_read_loop(int fd);

#endif
