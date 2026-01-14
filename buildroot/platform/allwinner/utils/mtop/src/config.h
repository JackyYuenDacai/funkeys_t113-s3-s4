/*
 * sunxi mbus or nsi test app of Allwinner SOC
 * Copyright (c) 2021
 *
 * zhoujie <zhoujie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

char *get_string_from_ini(char *title, char *key, char *filename);
int get_int_from_ini(char *title,char *key, char *filename);
int IniReadValue(char* section, char* key,char* val, char* file);
int readStringValue(char* section, char* key,char* val, char* file);
#endif
