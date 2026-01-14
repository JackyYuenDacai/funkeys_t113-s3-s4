/*
 * sunxi mbus or nsi test app of Allwinner SoC
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

#include "config.h"

#define SECTION_MAX_LEN 256
#define STRVALUE_MAX_LEN 256
#define LINE_CONTENT_MAX_LEN 256

int IniReadValue(char* section, char* key,char* val, char* file)
{
	FILE* fp;
	int i = 0;
	int lineContentLen = 0;
	int position = 0;
	char lineContent[LINE_CONTENT_MAX_LEN];
	char val_temp[LINE_CONTENT_MAX_LEN];
	int bFoundSection = 0;
	int bFoundKey = 0;
	int ret = 0;
	fp = fopen(file, "r");
	if (fp == NULL) {
		printf("%s: Opent file %s failed.\n", __FILE__, file);
		return -1;
 	}
	while(feof(fp) == 0) {
		memset(lineContent, 0, LINE_CONTENT_MAX_LEN);
		fgets(lineContent, LINE_CONTENT_MAX_LEN, fp);
		if ((lineContent[0] == ';') || (lineContent[0] == '\0') || (lineContent[0] == '\r') || (lineContent[0] == '\n')) {
			continue;
		}

		//check section
		if (strncmp(lineContent, section, strlen(section)) == 0) {
			bFoundSection = 1;
			while(feof(fp) == 0) {
				memset(lineContent, 0, LINE_CONTENT_MAX_LEN);
				memset(val_temp, 0, LINE_CONTENT_MAX_LEN);
				fgets(lineContent, LINE_CONTENT_MAX_LEN, fp);
				if (strncmp(lineContent, key, strlen(key)) == 0) {
					bFoundKey = 1;
					lineContentLen = strlen(lineContent);
					for (i = strlen(key); i < lineContentLen; i++) {
						if (lineContent[i] == '=') {
							position = i + 1;
							break;
						}
					}
					if (i >= lineContentLen) break;
					strncpy(val_temp, lineContent + position, strlen(lineContent + position)-1);//-1为了防止后面的换行符读进去
					sscanf(val_temp, "%49[a-zA-Z0-9_]", val);
					lineContentLen = strlen(val);
					for (i = 0; i < lineContentLen; i++) {
						if((lineContent[i] == '\0') || (lineContent[i] == '\r') || (lineContent[i] == '\n')) {
							val[i] = '\0';
							break;
						}
					}
				} else if (lineContent[0] == '[') {
					break;
				}
			}
			break;
		}
	}
	if (!bFoundSection) {
		printf("No section = %s\n", section);
		ret = -1;
	}
	if (!bFoundKey) {
		printf("No key = %s\n", key);
		ret = -1;
	}
	fclose(fp);

	return ret;
}

int readStringValue(char* section, char* key, char* val, char* file)
{
	char sect[SECTION_MAX_LEN];
	int ret = 0;

	if (section == NULL || key == NULL || val == NULL || file == NULL) {
		printf("%s: input parameter(s) is NULL!\n", __func__);
		return -1;
	}

	memset(sect, 0, SECTION_MAX_LEN);
	sprintf(sect, "[%s]", section);
	ret = IniReadValue(sect, key, val, file);

	return ret;
}

//read string from config file
char *get_string_from_ini(char *title, char *key, char *filename)
{
	FILE *fp;
	char szLine[1024];
	static char tmpstr[1024];
	int rtnval;
	int i = 0;
	int flag = 0;
	char *tmp;

	if ((fp = fopen(filename, "r")) == NULL) {
		perror("fopen()");
		exit(-4);
	}

	while(!feof(fp)) {
		rtnval = fgetc(fp);
		if (rtnval == EOF) {
			break;
		} else {
			szLine[i++] = rtnval;
		}

		if (rtnval == '\n') {
			szLine[--i] = '\n';
			i = 0;
			tmp = strchr(szLine, '=');

			if ((tmp != NULL) && (flag == 1)) {
				if (strstr(szLine, key) != NULL) {
					if ('#' == szLine[0]);
					else if ('/' == szLine[0] && '/' == szLine[1]);
					else {
						//local key position
						strcpy(tmpstr, tmp+1);
						fclose(fp);
						return tmpstr;
					}
		         	}
			} else {
				strcpy(tmpstr, "[");
				strcat(tmpstr, title);
				strcat(tmpstr, "]");
				if (strncmp(tmpstr, szLine, strlen(tmpstr)) == 0) {
					//encounter title
					flag = 1;
				}
			}
		}
	}

	fclose(fp);
	return "";
}

int get_int_from_ini(char *title, char *key, char *filename)
{
	return atoi(get_string_from_ini(title, key, filename));
}

