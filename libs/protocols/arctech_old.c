/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

    pilight is free software: you can redistribute it and/or modify it under the 
	terms of the GNU General Public License as published by the Free Software 
	Foundation, either version 3 of the License, or (at your option) any later 
	version.

    pilight is distributed in the hope that it will be useful, but WITHOUT ANY 
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR 
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "log.h"
#include "protocol.h"
#include "binary.h"
#include "gc.h"
#include "arctech_old.h"

void arctechOldCreateMessage(int id, int unit, int state) {
	arctech_old->message = json_mkobject();
	json_append_member(arctech_old->message, "id", json_mknumber(id));
	json_append_member(arctech_old->message, "unit", json_mknumber(unit));
	if(state == 1)
		json_append_member(arctech_old->message, "state", json_mkstring("on"));
	else
		json_append_member(arctech_old->message, "state", json_mkstring("off"));
}

void arctechOldParseBinary(int repeats) {
	int fp = 0;
	int i = 0;
	for(i=0;i<arctech_old->binlen;i++) {
		if((arctech_old->code[(4*i+0)] != 0) || (arctech_old->code[(4*i+1)] != 1)
			|| (arctech_old->code[(4*i+2)] == arctech_old->code[(4*i+3)])) {
			fp = 1;
		}
	}
	if((arctech_old->code[38] != 1) || (arctech_old->code[42] != 1)
		|| (arctech_old->code[48] != 0) || (arctech_old->code[49] != 1)) {
		fp = 1;
	}
	int unit = binToDec(arctech_old->binary, 0, 3);
	int state = arctech_old->binary[11];
	int id = binToDec(arctech_old->binary, 4, 8);
	if(fp == 0)
		arctechOldCreateMessage(id, unit, state);
}

void arctechOldCreateLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		arctech_old->raw[i]=(PULSE_LENGTH);
		arctech_old->raw[i+1]=(arctech_old->pulse*PULSE_LENGTH);
		arctech_old->raw[i+2]=(arctech_old->pulse*PULSE_LENGTH);
		arctech_old->raw[i+3]=(PULSE_LENGTH);
	}
}

void arctechOldCreateHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		arctech_old->raw[i]=(PULSE_LENGTH);
		arctech_old->raw[i+1]=(arctech_old->pulse*PULSE_LENGTH);
		arctech_old->raw[i+2]=(PULSE_LENGTH);
		arctech_old->raw[i+3]=(arctech_old->pulse*PULSE_LENGTH);
	}
}

void arctechOldClearCode(void) {
	arctechOldCreateHigh(0,35);
	arctechOldCreateLow(36,47);
}

void arctechOldCreateUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			arctechOldCreateLow(x, x+3);
		}
	}
}

void arctechOldCreateId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			arctechOldCreateLow(16+x, 16+x+3);
		}
	}
}

void arctechOldCreateState(int state) {
	if(state == 0) {
		arctechOldCreateHigh(44,47);
	}
}

void arctechOldCreateFooter(void) {
	arctech_old->raw[48]=(PULSE_LENGTH);
	arctech_old->raw[49]=(arctech_old->footer*PULSE_LENGTH);
}

int arctechOldCreateCode(JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	char *tmp;

	if(json_find_string(code, "id", &tmp) == 0)
		id=atoi(tmp);
	if(json_find_string(code, "off", &tmp) == 0)
		state=0;
	else if(json_find_string(code, "on", &tmp) == 0)
		state=1;
	if(json_find_string(code, "unit", &tmp) == 0)
		unit = atoi(tmp);

	if(id == -1 || unit == -1 || state == -1) {
		logprintf(LOG_ERR, "arctech_old: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 31 || id < 0) {
		logprintf(LOG_ERR, "arctech_old: invalid id range");
		return EXIT_FAILURE;
	} else if(unit > 15 || unit < 0) {
		logprintf(LOG_ERR, "arctech_old: invalid unit range");
		return EXIT_FAILURE;
	} else {
		arctechOldCreateMessage(id, unit, state);
		arctechOldClearCode();
		arctechOldCreateUnit(unit);
		arctechOldCreateId(id);
		arctechOldCreateState(state);
		arctechOldCreateFooter();
	}
	return EXIT_SUCCESS;
}

void arctechOldPrintHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
}

void arctechOldInit(void) {

	protocol_register(&arctech_old);
	arctech_old->id = malloc(13);
	strcpy(arctech_old->id, "archtech_old");
	protocol_device_add(arctech_old, "kaku_old", "Old KlikAanKlikUit Switches");
	protocol_device_add(arctech_old, "cogex", "Cogex Switches");
	protocol_device_add(arctech_old, "intertechno_old", "Old Intertechno Switches");
	arctech_old->type = SWITCH;
	arctech_old->pulse = 4;
	arctech_old->footer = 39;
	arctech_old->rawlen = 50;
	arctech_old->binlen = 12;
	arctech_old->lsb = 2;

	arctech_old->bit = 0;
	arctech_old->recording = 0;

	options_add(&arctech_old->options, 't', "on", no_value, config_state, NULL);
	options_add(&arctech_old->options, 'f', "off", no_value, config_state, NULL);
	options_add(&arctech_old->options, 'u', "unit", has_value, config_id, "^([0-9]{1}|[1][0-5])$");
	options_add(&arctech_old->options, 'i', "id", has_value, config_id, "^(3[012]?|[012][0-9]|[0-9]{1})$");

	protocol_setting_add_string(arctech_old, "states", "on,off");
	protocol_setting_add_number(arctech_old, "readonly", 0);
	
	arctech_old->parseBinary=arctechOldParseBinary;
	arctech_old->createCode=&arctechOldCreateCode;
	arctech_old->printHelp=&arctechOldPrintHelp;
}
