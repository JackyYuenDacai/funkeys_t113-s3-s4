#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <wifi_log.h>

char *del_left_trim(char *str)
{
	assert(str != NULL);
	for(;*str != '\0' && isblank(*str); ++str);
	return  str;
}

char *del_both_trim(char * str)
{
	char *p;
	char * szOutput;
	szOutput = del_left_trim(str);
	for(p = szOutput + strlen(szOutput) - 1; p >= szOutput && isblank(*p);--p);
		*(++p) =  '\0';
	return  szOutput;
}

int get_config(char *config_file_name, char *config_name, char *config_buf)
{
	FILE * fp = NULL;
	/*open config file*/
	fp = fopen(config_file_name, "r");
	char buf[64];
	char s[64];
	char *delim = "=";
	char *p;
	char ch;
	while (!feof(fp)) {
		if((p = fgets(buf, sizeof(buf), fp)) != NULL) {
			strcpy(s, p);
			ch=del_left_trim(s)[0];

			if(ch == '#' || isblank(ch) || ch== '\n' )
				continue ;
			p=strtok(s, delim);
			if(p) {
				if(!strcmp(config_name, del_both_trim(p))) {
					while((p = strtok(NULL, delim))) {
						strcpy(config_buf, p);
						WMG_DEBUG("get config: %s=%s\n", config_name, config_buf);
						fclose(fp);
						return 0;
					}
				} else {
					p = strtok(NULL, delim);
				}
			}
		}
	}
	WMG_WARNG("Can't get config: %s\n", config_name);
	fclose(fp);
	return  -1;
}
