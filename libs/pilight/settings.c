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
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <regex.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>

#include "json.h"
#include "settings.h"
#include "log.h"

/* Add a string value to the settings struct */
void settings_add_string(const char *name, char *value) {
	struct settings_t *snode = malloc(sizeof(struct settings_t));
	snode->name = malloc(strlen(name)+1);
	strcpy(snode->name, name);
	snode->value = malloc(strlen(value)+1);
	strcpy(snode->value, value);
	snode->type = 2;
	snode->next = settings;
	settings = snode;
}

/* Add an int value to the settings struct */
void settings_add_number(const char *name, int value) {
	struct settings_t *snode = malloc(sizeof(struct settings_t));
	char ctmp[256];
	snode->name = malloc(strlen(name)+1);
	strcpy(snode->name, name);
	sprintf(ctmp, "%d", value);
	snode->value = malloc(strlen(ctmp)+1);
	strcpy(snode->value, ctmp);
	snode->type = 1;
	snode->next = settings;
	settings = snode;
}

/* Retrieve a numeric value from the settings struct */
int settings_find_number(const char *name, int *out) {
	struct settings_t *tmp_settings = settings;

	while(tmp_settings) {
		if(strcmp(tmp_settings->name, name) == 0 && tmp_settings->type == 1) {
			*out = atoi(tmp_settings->value);
			return 0;
		}
		tmp_settings = tmp_settings->next;
	}
	free(tmp_settings);
	return 1;
}

/* Retrieve a string value from the settings struct */
int settings_find_string(const char *name, char **out) {
	struct settings_t *tmp_settings = settings;

	while(tmp_settings) {
		if(strcmp(tmp_settings->name, name) == 0 && tmp_settings->type == 2) {
			*out = tmp_settings->value;
			return 0;
		}
		tmp_settings = tmp_settings->next;
	}
	free(tmp_settings);
	return 1;
}

/* Check if a given path exists */
int settings_path_exists(char *fil) {
	struct stat s;
	char *filename = basename(fil);
	char path[1024];
	size_t i = (strlen(fil)-strlen(filename));

	memset(path, '\0', sizeof(path));
	memcpy(path, fil, i);
	snprintf(path, i, "%s", fil);
	
	if(strcmp(filename, fil) != 0) {
		int err = stat(path, &s);
		if(err == -1) {
			if(ENOENT == errno) {
				return 1;
			} else {
				return 1;
			}
		} else {
			if(S_ISDIR(s.st_mode)) {
				return 0;
			} else {
				return 1;
			}
		}
	}
	return 0;
}

/* Check if a given file exists */
int settings_file_exists(char *filename) {
	struct stat sb;   
	return stat(filename, &sb);
}

int settings_parse(JsonNode *root) {
	int have_error = 0;
	int is_node = 0;
	char *server_ip = malloc(17);
	int server_port = 0;
	int web_port = 0;
	int own_port = 0;
	int has_config = 0;
	int hw_mode = 0;
	int has_socket = 0;
	int gpio_in = -1;
	int gpio_out = -1;

#ifndef __FreeBSD__	
	regex_t regex;
	int reti;
#endif	
	
	JsonNode *jsettings = json_first_child(root);

	while(jsettings) {
		if(strcmp(jsettings->key, "port") == 0 || strcmp(jsettings->key, "send-repeats") == 0 || strcmp(jsettings->key, "receive-repeats") == 0) {
			if((int)jsettings->number_ == 0) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a number larger than 0", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				if(strcmp(jsettings->key, "port") == 0) {
					own_port = (int)jsettings->number_;
				}
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "mode") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must be \"server\" or \"client\"", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				if(strcmp(jsettings->string_, "client") == 0) {
					is_node = 1;
				}
				if(strcmp(jsettings->string_, "server") == 0 || strcmp(jsettings->string_, "client") == 0) {
					settings_add_string(jsettings->key, jsettings->string_);
				} else {
					logprintf(LOG_ERR, "setting \"%s\" must be \"server\" or \"client\"", jsettings->key);
					have_error = 1;
					goto clear;
				}
			}
		} else if(strcmp(jsettings->key, "log-level") == 0) {
			if((int)jsettings->number_ == 0 || (int)jsettings->number_ > 5) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a number from 0 till 5", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "pid-file") == 0 || strcmp(jsettings->key, "log-file") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain an existing file path", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				if(settings_path_exists(jsettings->string_) != 0) {
					logprintf(LOG_ERR, "setting \"%s\" must point to an existing folder", jsettings->key);
					have_error = 1;
					goto clear;				
				} else {
					settings_add_string(jsettings->key, jsettings->string_);
				}
			}
		} else if(strcmp(jsettings->key, "config-file") == 0 || strcmp(jsettings->key, "process-file") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain an existing file path", jsettings->key);
				have_error = 1;
				goto clear;
			} else if(strlen(jsettings->string_) > 0) {
				if(settings_file_exists(jsettings->string_) == 0) {
					if(strcmp(jsettings->key, "config-file") == 0) {
						has_config = 1;
					}
					settings_add_string(jsettings->key, jsettings->string_);
				} else {
					logprintf(LOG_ERR, "setting \"%s\" must point to an existing file", jsettings->key);
					have_error = 1;
					goto clear;
				}
			} else if(strcmp(jsettings->key, "config-file") == 0) {
				has_config = 1;
			}
		} else if(strcmp(jsettings->key, "whitelist") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain valid ip addresses", jsettings->key);
				have_error = 1;
				goto clear;
			} else if(strlen(jsettings->string_) > 0) {
#ifndef __FreeBSD__			
				char validate[] = "^((\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))\\.(\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))\\.(\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))\\.(\\*|[0-9]|[1-9][0-9]|1[0-9][0-9]|2([0-4][0-9]|5[0-5]))(,[\\ ]|,|$))+$";
				reti = regcomp(&regex, validate, REG_EXTENDED);
				if(reti) {
					logprintf(LOG_ERR, "could not compile regex");
					have_error = 1;
					goto clear;
				}
				reti = regexec(&regex, jsettings->string_, 0, NULL, 0);
				if(reti == REG_NOMATCH || reti != 0) {
					logprintf(LOG_ERR, "setting \"%s\" must contain valid ip addresses", jsettings->key);
					have_error = 1;
					regfree(&regex);
					goto clear;
				}
				regfree(&regex);
#endif
				int l = (int)strlen(jsettings->string_)-1;
				if(jsettings->string_[l] == ' ' || jsettings->string_[l] == ',') {
					logprintf(LOG_ERR, "setting \"%s\" must contain valid ip addresses", jsettings->key);
					have_error = 1;
					goto clear;
				}
				settings_add_string(jsettings->key, jsettings->string_);
			}
		} else if(strcmp(jsettings->key, "hw-socket") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must point an existing socket", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				has_socket = 1;
				settings_add_string(jsettings->key, jsettings->string_);
			}
		} else if(strcmp(jsettings->key, "hw-mode") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must be either \"gpio\", \"module\", or \"none\"", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				if(strcmp(jsettings->string_, "module") == 0) {
					hw_mode = 1;
				} else if(strcmp(jsettings->string_, "none") == 0) {
					hw_mode = 0;
				} else if(strcmp(jsettings->string_, "gpio") == 0) {
					hw_mode = 2;
				}
				settings_add_string(jsettings->key, jsettings->string_);
			}
		} else if(strcmp(jsettings->key, "webserver-port") == 0) {
			if(jsettings->number_ < 0) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a number larger than 0", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				web_port = (int)jsettings->number_;
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "webserver-root") == 0) {
			if(!jsettings->string_) {
				logprintf(LOG_ERR, "setting \"%s\" must contain a valid path", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_string(jsettings->key, jsettings->string_);
			}
		} else if(strcmp(jsettings->key, "webserver-enable") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "webserver-cache") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 1) {
				logprintf(LOG_ERR, "setting \"%s\" must be either 0 or 1", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}
		} else if(strcmp(jsettings->key, "gpio-receiver") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 7) {
				logprintf(LOG_ERR, "setting \"%s\" must be between 0 and 7", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				gpio_out = (int)jsettings->number_;
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}			
		} else if(strcmp(jsettings->key, "gpio-sender") == 0) {
			if(jsettings->number_ < 0 || jsettings->number_ > 7) {
				logprintf(LOG_ERR, "setting \"%s\" must be between 0 and 7", jsettings->key);
				have_error = 1;
				goto clear;
			} else {
				gpio_in = (int)jsettings->number_;
				settings_add_number(jsettings->key, (int)jsettings->number_);
			}			
		} else if(strcmp(jsettings->key, "server") == 0) {
			JsonNode *jserver = json_find_member(root, "server");
			JsonNode *jtmp = json_first_child(jserver);
			unsigned short i = 0;
			while(jtmp) {
				i++;
				if(jtmp->tag == JSON_STRING) {
					server_ip = realloc(server_ip, strlen(jtmp->string_)+1);
					strcpy(server_ip, jtmp->string_);
				} else if(jtmp->tag == JSON_NUMBER) {
					server_port = (int)jtmp->number_;
				}
				if(i > 2) {
					i++;
					break;
				}
				jtmp = jtmp->next;
			}
			if(2 < i) {
				logprintf(LOG_ERR, "setting \"%s\" must be in the format of [ \"x.x.x.x\", xxxx ]", jsettings->key);
				have_error = 1;
				goto clear;
			} else if(strlen(server_ip) > 0 && server_port > 0 && is_node == 1) {
				settings_add_string("server-ip", server_ip);
				settings_add_number("server-port", server_port);
			} else {	
				logprintf(LOG_ERR, "setting \"%s\" must be in the format of [ \"x.x.x.x\", xxxx ]", jsettings->key);
				have_error = 1;
				goto clear;
			}
		} else {
			logprintf(LOG_ERR, "setting \"%s\" is invalid", jsettings->key);
			have_error = 1;
			goto clear;
		}
		jsettings = jsettings->next;
	}
	json_delete(jsettings);
	if((hw_mode == 1 || hw_mode == 0) && gpio_in > -1) {
		logprintf(LOG_ERR, "setting \"gpio-receiver\" and \"module\" or \"none\" hw-mode cannot be combined");
		have_error = 1;
		goto clear;
	}
	if((hw_mode == 1 || hw_mode == 0) && gpio_out > -1) {
		logprintf(LOG_ERR, "setting \"gpio-sender\" and \"module\" or \"none\" hw-mode cannot be combined");
		have_error = 1;
		goto clear;
	}	
	if((hw_mode == 2 || hw_mode == 0) && has_socket == 1) {
		logprintf(LOG_ERR, "setting \"hw-socket\" must be combined with \"module\" hw-mode");
		have_error = 1;
		goto clear;
	}
	if(server_port > 0 && server_port == own_port && is_node == 1) {
		logprintf(LOG_ERR, "setting \"port\" and \"server port\" cannot be the same");
		have_error = 1;
		goto clear;
	}
	if(web_port == own_port) {
		logprintf(LOG_ERR, "setting \"port\" and \"webserver-port\" cannot be the same");
		have_error = 1;
		goto clear;
	}
	if(is_node == 1 && has_config == 1) {
		logprintf(LOG_ERR, "a daemon running as client cannot have a config file defined");
		have_error = 1;
		goto clear;		
	}
	free(server_ip);
clear:
	return have_error;
}

int settings_write(char *content) {
	FILE *fp;

	/* Overwrite config file with proper format */
	if(!(fp = fopen(settingsfile, "w+"))) {
		logprintf(LOG_ERR, "cannot write settings file: %s", settingsfile);
		return EXIT_FAILURE;
	}
	fseek(fp, 0L, SEEK_SET);
 	fwrite(content, sizeof(char), strlen(content), fp);
	fclose(fp);

	return EXIT_SUCCESS;
}

int settings_gc(void) {
	struct settings_t *tmp;

	while(settings) {
		tmp = settings;
		free(tmp->name);
		free(tmp->value);
		settings = settings->next;
		free(tmp);
	}
	free(settings);
	
	if(settingsfile) {
		free(settingsfile);
		settingsfile = NULL;
	}
	logprintf(LOG_DEBUG, "garbage collected settings library");
	return 1;
}

int settings_read(void) {
	FILE *fp;
	char *content;
	size_t bytes;
	JsonNode *root;
	struct stat st;

	/* Read JSON config file */
	if(!(fp = fopen(settingsfile, "rb"))) {
		logprintf(LOG_ERR, "cannot read settings file: %s", settingsfile);
		return EXIT_FAILURE;
	}

	fstat(fileno(fp), &st);
	bytes = (size_t)st.st_size;

	if(!(content = calloc(bytes+1, sizeof(char)))) {
		logprintf(LOG_ERR, "out of memory");
		return EXIT_FAILURE;
	}

	if(fread(content, sizeof(char), bytes, fp) == -1) {
		logprintf(LOG_ERR, "cannot read settings file: %s", settingsfile);
	}
	fclose(fp);

	/* Validate JSON and turn into JSON object */
	if(json_validate(content) == false) {
		logprintf(LOG_ERR, "settings are not in a valid json format", content);
		free(content);
		return EXIT_FAILURE;
	}

	root = json_decode(content);

	if(settings_parse(root) != 0) {
		free(content);
		return EXIT_FAILURE;
	}
	char *output = json_stringify(root, "\t");
	settings_write(output);
	json_delete(root);
	free(output);	
	free(content);
	return EXIT_SUCCESS;
}

int settings_set_file(char *settfile) {
	if(access(settfile, R_OK | W_OK) != -1) {
		settingsfile = realloc(settingsfile, strlen(settfile)+1);
		strcpy(settingsfile, settfile);
	} else {
		fprintf(stderr, "%s: the settings file %s does not exists\n", progname, settfile);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
