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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sched.h>


#define PLATFORM Linux

#if PLATFORM == Linux
#include "check_hardware.h"
#define PROPERTY_VALUE_MAX 20
#elif
#include <cutils/properties.h>
#endif

#define MTOP_VERSION "1.2"

#define GTBUS_PMU_DIR "/sys/devices/platform/GTBUS_PMU"
#define MBUS_PMU_DIR "/sys/class/hwmon/mbus_pmu/device"
#define HWMON0_DIR "/sys/class/hwmon/hwmon0"
#define HWMON1_DIR "/sys/class/hwmon/hwmon1"
#define PMU_HWMON0_DIR "/sys/class/nsi-pmu/hwmon0"

typedef struct NodeInfo {
	int id;  //@TODO: can be removed
	char *name;
	FILE *fp;
	unsigned int curcnt;
	unsigned int precnt;
	unsigned long long delta;
} NodeInfo;

typedef struct NodeUnit {
	int id;  //@TODO: can be removed
	char *name;
	unsigned int div;
} NodeUnit;

typedef struct bus_parameter {
	char *hardware_name;
	char *nodeinfo_name;
	char *nodeunit_name;
	NodeInfo *node_info;
	NodeUnit *node_uint;
	unsigned length;
	char *patch;
	unsigned bus_type;
} bus_para;

/**
 * NOTE: we always put "totddr" at first array whether this node exist or not,
 * for the totddr is caculated every time.
 */
static int mtop_baner();
static int mtop_read();
static void mtop_post();
static void mtop_update();
static void mtop_nsi_update();
static int mtop_read_bandwidth();
static int get_plat_param();

