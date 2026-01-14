/*
 * sunxi mbus or nsi test app of Allwinner SOC
 * Copyright (c) 2021
 *
 * huafenghuang <huafenghuang@allwinnertech.com>
 * wuyan <wuyan@allwinnertech.com>
 * zhoujie <zhoujie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "config.h"
#include "mtop.h"

#define FILE_LEN 256
NodeInfo *nodeInfo;
NodeUnit *nodeUnit;

unsigned int max = 0;
unsigned long long total = 0, idx = 0;
int nodeCnt;
int iter = -1, latency = 0;
int mUnit = 0, show_duration = 0;
char output_fn[256];
char path_prefix[256];
static int bandwidth[32];
char hardware[PROPERTY_VALUE_MAX];
float delay = 1;
FILE *output_fp = NULL;

bus_para *main_bus_para;

static const struct option long_options[] =
{
	{"-n", required_argument, NULL, 'n'},
	{"-d", required_argument, NULL, 'd'},
	{"-o", required_argument, NULL, 'f'},
	{"-m", no_argument, NULL, 'm'},
	{"-t", no_argument, NULL, 't'},
	{"-l", no_argument, NULL, 'l'},
	{"-v", no_argument, NULL, 'v'},
	{"-h", no_argument, NULL, 'h'},
};
/*----------------------------------------------------------------------------*/
// Only for SoC which uses NSI like A100 -->
static void get_path(void)
{
	if (access(PMU_HWMON0_DIR, F_OK))
		strncpy(path_prefix, HWMON0_DIR, sizeof(path_prefix));
	else
		strncpy(path_prefix, PMU_HWMON0_DIR, sizeof(path_prefix));

}

static void set_nsi_pmu_timer(float time)
{
	char command[512];
	char pmu_timer[128];

	sprintf(pmu_timer, "%s/pmu_timer", path_prefix);
	sprintf(command, "echo %d > %s", (int)(time*1000*1000), pmu_timer);
	system(command);
}

static int get_nsi_pmu_id()
{
	char path[128];
	char buf[512];
	char bwbuf[32][16];
	char *p;
	int i = 0, j= 0;
	FILE *file = NULL;


	sprintf(path, "%s/available_pmu", path_prefix);
	file = fopen(path, "r");
	if (!file)
		return -1;
	fseek(file, 0, SEEK_SET);
	fgets(buf, 512, file);
	for (p = strtok(buf, " "); p != NULL; p = strtok(NULL, " "), i++) {
		memcpy(bwbuf[i], p, 16);
		for (j= 0; j < nodeCnt; j++) {
			if(!strcmp(nodeInfo[j].name, bwbuf[i])) {
				nodeInfo[j].id = i;
				break;
			}
		}
	}

	fclose(file);
	return 0;
}

static void usage(char *program)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "Usage: %s [-n iter] [-d delay] [-m] [-o FILE] [-h]\n", program);
	fprintf(stdout, "    -n NUM   Updates to show before exiting.\n");
	fprintf(stdout, "    -d NUM   Seconds to wait between update.\n");
	fprintf(stdout, "    -t show duration time.\n");
	fprintf(stdout, "    -m unit: %s\n", mUnit ? main_bus_para->node_uint[1].name : main_bus_para->node_uint[0].name);
	fprintf(stdout, "    -o FILE  Output to a file.\n");
	fprintf(stdout, "    -v Display mtop version.\n");
	fprintf(stdout, "    -h Display this help screen.\n");
	fprintf(stdout, "\n");
}

static void version(void)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "mtop version: %s\n", MTOP_VERSION);
	fprintf(stdout, "hardware support: sunxi platform \n");
	fprintf(stdout, "last update time : 2020-01-15 \n");
	fprintf(stdout, "\n");
}

static void *show_args()
{
	fprintf(stdout, "\n");
	fprintf(stdout, "iter: %d\n", iter);
	fprintf(stdout, "dealy: %2.1f\n", delay);
	fprintf(stdout, "muint: %2u\n", mUnit);
	fprintf(stdout, "unit: %s\n", mUnit ? main_bus_para->node_uint[1].name : main_bus_para->node_uint[0].name);
	fprintf(stdout, "output: %s\n", output_fn);
	fprintf(stdout, "\n");
}

static int parse_cmd(int argc, char **argv)
{
	int opt;
	int options_index = 0;
	int ret = 0;

	while ((opt = getopt_long(argc, argv, "n:d:o:mtlvh", long_options, &options_index)) != EOF)
	{
		switch (opt) {
			case 'n':
				iter = atoi(optarg);
				break;
			case 'd':
				delay = atof(optarg);
				break;
			case 'o':
				strncpy(output_fn, optarg, 256);
				if (strlen(output_fn)) {
					output_fp = fopen(output_fn, "w");
					if (!output_fp) {
						fprintf(stdout, "Could not open file %s: %s\n", output_fn, strerror(errno));
						ret = -1;
					}
				}
				break;
			case 'm':
				mUnit = 1;
				break;
			case 't':
				show_duration = 1;
				break;
			case 'l':
				latency = 1;
				break;
			case 'v':
				version();
				break;
			case 'h':
				usage(argv[0]);
				break;
			default:
				return -1;
		}

	}

	return ret;
}

static int get_hardname(void)
{
	int ret = 0;
#if PLATFORM == Linux
	ret = get_hardware_name(hardware, PROPERTY_VALUE_MAX);
	if (ret < 0) {
		printf("get hardware name fail!, ret:%d\n", ret);
		return ret;
	}
#elif
	property_get("ro.hardware", hardware, "sun9i");
#endif
	main_bus_para->nodeinfo_name = malloc(50);
	if (!main_bus_para->nodeinfo_name)
	{
		return -1;
	}
	sprintf(main_bus_para->nodeinfo_name,"nodeInfo_%s",hardware);
	return 0;
}

static void *get_bus_data()
{
	unsigned long duration,max_duration = 0;
	struct timeval cur_t, pre_t;

	if (main_bus_para->bus_type) {
		set_nsi_pmu_timer(delay);
		get_nsi_pmu_id();
		gettimeofday(&pre_t, NULL);
		while(iter == -1 || iter-- > 0) {
			usleep(delay*1000*1000);
			mtop_read_bandwidth();
			mtop_nsi_update();
			if (show_duration) {
				gettimeofday(&cur_t, NULL);
				duration = (cur_t.tv_sec - pre_t.tv_sec)*1000000 + (cur_t.tv_usec - pre_t.tv_usec);
				pre_t = cur_t;
				if (duration > max_duration)
					max_duration = duration;
				printf("duration:%luus, max:%luus\n", duration, max_duration);
			}
		}
	} else {
		mtop_read();
		mtop_post();

		while (iter == -1 || iter-- > 0) {
			sleep(delay);
			mtop_read();
			mtop_update();
			mtop_post();
		}
	}
}

static int readNodeUnitArray(char* section, struct NodeUnit *nodeunit, int n, char *file)
{
	char sect[50];
	FILE *fd;
	int i = 0;
	int ret = 0;
	char lineContent[256];

	memset(sect, 0, 50);
	sprintf(sect, "[%s]", section);
	if ((fd = fopen(file, "r")) == NULL) {
		printf("fopen is error\n");
		ret = -1;
	}
	while (feof(fd) == 0)
	{
		memset(lineContent, 0, 256);
		fgets(lineContent, 256, fd);
		if((lineContent[0] == ';') || (lineContent[0] == '\0') || (lineContent[0] == '\r') || (lineContent[0] == '\n'))
		{
			continue;
		}
		if((strncmp(lineContent, sect, strlen(sect))) == 0)
		{
			nodeunit[i].name = malloc(10);
			if (!nodeunit[i].name)
			{
				ret = -1;
				break;
			}
			while(fscanf(fd,"{%d, %[a-zA-Z], %d},", &nodeunit[i].id, nodeunit[i].name, &nodeunit[i].div) == 3) {
				if (++i == n) {
					break;
				}
				fgets(lineContent, 256, fd);
				nodeunit[i].name = malloc(10);
				if (!nodeunit[i].name)
				{
					ret = -1;
					break;
				}
			}
			break;
		}
	}
	if (!fd)
		fclose(fd);

	return ret;
}


static int readNodeInfoArray(char* section, struct NodeInfo *nodeinfo, int n, char *file)
{
	char sect[50];
	FILE *fd;
	int i = 0;
	int ret = 0;
	char lineContent[256];

	memset(sect, 0, 50);
	sprintf(sect, "[%s]", section);
	if ((fd = fopen(file, "r")) == NULL) {
		printf("fopen is error\n");
		ret = -1;
	}

	while (feof(fd) == 0)
	{
		memset(lineContent, 0, 256);
		fgets(lineContent, 256, fd);
		if((lineContent[0] == ';') || (lineContent[0] == '\0') || (lineContent[0] == '\r') || (lineContent[0] == '\n'))
		{
			continue;
		}
		if((strncmp(lineContent, sect, strlen(sect))) == 0)
		{
			nodeinfo[i].name = malloc(10);
			nodeinfo[i].fp = malloc(10);
			if (!nodeinfo[i].name)
			{
				ret = -1;
				break;
			}

			if (!nodeinfo[i].fp)
			{
				free(nodeinfo[i].name);
				ret = -1;
				break;
			}

			i = 0;
			while(fscanf(fd,"{%d, %[a-zA-Z0-9_], %[a-zA-Z], %d, %d, %d}",
						&nodeinfo[i].id, nodeinfo[i].name, nodeinfo[i].fp,
						&nodeinfo[i].curcnt, &nodeinfo[i].precnt,
						&nodeinfo[i].delta) == 6) {
				if (++i == n) {
					break;
				}
				fgets(lineContent, 256, fd);
				nodeinfo[i].name = malloc(10);
				nodeinfo[i].fp = malloc(10);
				if (!nodeinfo[i].name)
				{
					ret = -1;
					break;
				}

				if (!nodeinfo[i].fp)
				{
					free(nodeinfo[i].name);
					ret = -1;
					break;
				}
			}
			main_bus_para->length = i;
			break;
		}
	}
	if (!i) {
		printf("not support platform %s,please make sure that the platform information has been configured in mtop.cfg\n",sect);
		ret = -1;
	}
	if (!fd)
		fclose(fd);

	return ret;
}

int parse_config()
{
	int ret = 0;
	char *filename = "/bin/mtop.cfg";

	main_bus_para->length = get_int_from_ini("max_length", "max_length", filename);
	main_bus_para->bus_type = get_int_from_ini("bus_type", "nsi", filename);
	main_bus_para->hardware_name = malloc(50);
	if (!main_bus_para->hardware_name)
	{
		return -1;
	}
	main_bus_para->nodeunit_name = malloc(50);
	if (!main_bus_para->nodeunit_name)
	{
		free(main_bus_para->hardware_name);
		return -1;
	}
	//ret = readStringValue("NodeInfo", "node_info", main_bus_para->nodeinfo_name, filename);
	ret = readStringValue("NodeUnit", "node_unit", main_bus_para->nodeunit_name, filename);

	main_bus_para->node_uint = (NodeUnit *)malloc(2 * sizeof(NodeUnit));
	if (!main_bus_para->node_uint)
	{
		free(main_bus_para->hardware_name);
		free(main_bus_para->nodeunit_name);
		return -1;
	}
	main_bus_para->node_info = (NodeInfo *)malloc(main_bus_para->length * sizeof(NodeInfo));
	if (!main_bus_para->node_info)
	{
		free(main_bus_para->hardware_name);
		free(main_bus_para->nodeunit_name);
		free(main_bus_para->node_uint);
		return -1;
	}
	ret = readNodeUnitArray(main_bus_para->nodeunit_name, main_bus_para->node_uint, 2, filename);
	ret = readNodeInfoArray(main_bus_para->nodeinfo_name, main_bus_para->node_info, main_bus_para->length, filename);

	return ret;
}

static int get_plat_param(void)
{
	nodeCnt = main_bus_para->length;
	nodeInfo = main_bus_para->node_info;
	nodeUnit = main_bus_para->node_uint;
	if (main_bus_para->bus_type)
		get_path();
	else
		strncpy(path_prefix, HWMON0_DIR, sizeof(path_prefix));
	/*
	 * linux4.4 use /sys/class/hwmon/hwmon0
	 */
	{
		FILE *check_file;
		check_file = fopen(path_prefix, "r");
		if (check_file == NULL) {
			memset(path_prefix, 0, sizeof(path_prefix));
			strncpy(path_prefix, HWMON0_DIR, sizeof(path_prefix));
		}
		else {
			fclose(check_file);
		}
	}

	return 0;
}

static int mtop_baner(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++) {
		fwrite(nodeInfo[i].name, strlen(nodeInfo[i].name), 1, output_fp);
		if (i+1 < nodeCnt) {
			fwrite(" ", 1, 1, output_fp);
		}
	}

	fwrite("\n", 1, 1, output_fp);

	return 0;
}

static int node_path(int num)
{
	int try_num = 0;
	char path[256];

try:
	snprintf(path, sizeof(path), "%s/pmu_%s", path_prefix, nodeInfo[num].name);
	main_bus_para->node_info[num].fp = fopen(path, "r");
	if (main_bus_para->node_info[num].fp)
		return 0;

	if (try_num)
		return -1;

	memset(path_prefix, 0, sizeof(path_prefix));
	strncpy(path_prefix, HWMON1_DIR, sizeof(path_prefix));
	try_num++;
	goto try;
}

static int mtop_read_bandwidth(void)
{
	char path[256];
	char buf[512];
	int i = 0, j= 0;
	char bwbuf[32][16];
	char *p;

	FILE *file = NULL;

	snprintf(path, sizeof(path), "%s/pmu_%s", path_prefix, "bandwidth");
	file = fopen(path, "r");
	if (!file)
		return -1;
	fseek(file, 0, SEEK_SET);
	fgets(buf, 512, file);
	for (p = strtok(buf, " "); p != NULL; p = strtok(NULL, " "), i++) {
		memcpy(bwbuf[i], p, 16);
		bandwidth[i] = 0;
		for (j = 0; bwbuf[i][j] >='0' && bwbuf[i][j] <= '9'; j++)
			bandwidth[i] = 10 * bandwidth[i] + (bwbuf[i][j] - '0');
	}

	fclose(file);
	return 0;
}

static int mtop_read(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++) {
		if (node_path(i)) {
			fprintf(stderr, "Could not open file %s: %s\n",
				main_bus_para->node_info[i].name, strerror(errno));
			goto open_error;
		}

		fscanf(main_bus_para->node_info[i].fp, "%u", &main_bus_para->node_info[i].curcnt);
		fclose(main_bus_para->node_info[i].fp);
		main_bus_para->node_info[i].fp = NULL;
	}

	return 0;

open_error:
	for (i = 1; i < nodeCnt; i++) {
		if (main_bus_para->node_info[i].fp) {
			fclose(main_bus_para->node_info[i].fp);
			main_bus_para->node_info[i].fp = NULL;
		}
	}
	return -1;
}

static void mtop_post(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++)
		main_bus_para->node_info[i].precnt = main_bus_para->node_info[i].curcnt;
}

static void mtop_update(void)
{
	int i;
	unsigned int cur_total;
	unsigned int average;

	cur_total = 0;
	main_bus_para->node_info[0].delta = 0;

	for (i = 1; i < nodeCnt; i++) {
		if (main_bus_para->node_info[i].precnt <= main_bus_para->node_info[i].curcnt)
			main_bus_para->node_info[i].delta = main_bus_para->node_info[i].curcnt - main_bus_para->node_info[i].precnt;
		else
			main_bus_para->node_info[i].delta = (unsigned int)((main_bus_para->node_info[i].curcnt + (unsigned long long)(2^32)) - main_bus_para->node_info[i].precnt);

		if (mUnit)
			main_bus_para->node_info[i].delta /= main_bus_para->node_uint[1].div;

		cur_total += main_bus_para->node_info[i].delta;
	}
	main_bus_para->node_info[0].delta = cur_total;

	if (cur_total > max)
		max = cur_total;
	total += cur_total;
	idx++;
	average = total / idx;

	fprintf(stdout, "total: %llu, ", total);
	fprintf(stdout, "num: %llu, ", idx);
	fprintf(stdout, "Max: %u, ", max);
	fprintf(stdout, "Average: %u\n", average);

	for (i = 0; i < nodeCnt; i++)
		fprintf(stdout, " %7s", main_bus_para->node_info[i].name);
	fprintf(stdout, "\n");
	for (i = 0; i < nodeCnt; i++) {
		fprintf(stdout, " %7llu", main_bus_para->node_info[i].delta);
		if (output_fp)
			fprintf(output_fp, "%7llu", main_bus_para->node_info[i].delta);
	}

	fprintf(stdout, "\n");

	if (cur_total == 0)
		cur_total++;
	for (i = 0; i < nodeCnt; i++)
		fprintf(stdout, " %7.2f", (float)main_bus_para->node_info[i].delta*100/cur_total);
	fprintf(stdout, "\n\n");

	if (output_fp) {
		fwrite("\n", 1, 1, output_fp);
		fflush(output_fp);
	}
}

static void mtop_nsi_update(void)
{
	int i, total = 0;
	unsigned long count = 0, max = 0;

	nodeInfo[0].delta = 0;
	for (i = 0; i < nodeCnt; i++) {
		if(nodeInfo[i].id == -1)
			continue;
		nodeInfo[i].delta = bandwidth[nodeInfo[i].id];
		nodeInfo[i].delta /= delay;
		if (mUnit)
			nodeInfo[i].delta /= nodeUnit[1].div;
	}

	for (i = 2; i < nodeCnt; i++)
		total += nodeInfo[i].delta;

	nodeInfo[1].delta = total;

	if (nodeInfo[0].delta > max)
		max = nodeInfo[0].delta;

	if (count % 20 == 0) {
		fprintf(stdout, "\n");
		fprintf(stdout, "Max:%-9.2lu\n", max);
		for (i = 0; i < nodeCnt; i++)
			fprintf(stdout, "%-10s", nodeInfo[i].name);
		fprintf(stdout, "\n");
	}


	for (i = 0; i < nodeCnt; i++) {
		fprintf(stdout, "%-9.2f ", (float)nodeInfo[i].delta);
		if (output_fp)
			fprintf(output_fp, "%-9.2f ", (float)nodeInfo[i].delta);
	}

	count++;

	fprintf(stdout, "\n");
	fprintf(stdout, "\n");

	if (output_fp) {
		fwrite("\n", 1, 1, output_fp);
		fflush(output_fp);
	}
}

int main(int argc, char **argv)
{
	int ret = 0;
	char *filename = "/bin/mtop.cfg";

	main_bus_para = (bus_para *)malloc(sizeof(bus_para));
	if (!main_bus_para)
	{
		printf("memory allocation failed\n");
		exit(1);
	}

	if (get_hardname())
	{
		printf("get_hardname failed\n");
		exit(1);
	}

	ret = parse_config();
	if (ret) {
		printf("Error parsing config file, please make sure configuration is correct\n");
		exit(1);
	}

	ret = get_plat_param();
	if (ret)
		exit(-1);

	memset(output_fn, '\0', sizeof(FILE_LEN * sizeof(char)));
	if (parse_cmd(argc, argv)) {
		printf("parsing command error, please enter the correct command\n");
		exit(1);
	}
	show_args();

	if (output_fp) {
		mtop_baner();
	}

	get_bus_data();

	if (output_fp) {
		fclose(output_fp);
	}
	return 0;
}

