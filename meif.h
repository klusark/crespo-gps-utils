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

#define MEIF_START	0xA593
#define MEIF_END	0xA568

/*
 * Commands
 */

#define MEIF_ACK_MSG		0x0000
#define MEIF_NACK_MSG		0x0001
//#define MEIF_ERROR_MSG	0x0002 or 0x0003
#define MEIF_STATE_REPORT_MSG	0x0004
#define MEIF_CONFIG_VALUES_MSG	0x0008

/*
 * Data
 */

/*
 * Structures
 */

struct meif_message {
	uint16_t command;
	uint16_t length;
	void *data;
} __attribute__((__packed__));

struct meif_config_values_report {
	uint8_t unknown1[36];
	char vendor[16];
	char product[16];
	uint8_t unknown2[2];
} __attribute__((__packed__));

/*
 * Functions
 */

void meif_read_loop(int fd);

#endif
