#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "amp_device.h"

#define RAW_DATA_LINE_SIZE (512 - 16)

static amp_device *rdevice;
static amp_device *wdevice;

extern raw_device_ops rpmsg_ops;
static raw_device_ops *device_ops = &rpmsg_ops;

int amp_device_read(char *data, int len)
{
	if (!rdevice)
		return -1;
	return device_ops->read(rdevice, data, len);
}

int amp_device_write(char *data, int len)
{
	if (!wdevice)
		return -1;
	return device_ops->write(wdevice, data, len);
}

int amp_device_init(void)
{
	int err = -1;

	rdevice = malloc(sizeof(amp_device));
	if (!rdevice) {
		printf("alloc read device failed!\n");
		return -ENOMEM;
	}
	memset(rdevice, 0, sizeof(amp_device));
	rdevice->ops = device_ops;
	strncpy(rdevice->name, "rpmsg", sizeof(rdevice->name));
	wdevice = rdevice;
	if (device_ops->init) {
		err = device_ops->init(rdevice);
		if (err) {
			printf("raw device init failed!\n");
			goto error;
		}
	}

	return 0;
error:
	free(rdevice);
	rdevice = NULL;
	wdevice = NULL;
	return err;
}

int amp_device_deinit(void)
{
	/* The read task maybe use the device as the same time. */
	if (rdevice) {
		device_ops->deinit(rdevice);
		free(rdevice);
		rdevice = NULL;
		wdevice = NULL;
	}

	return 0;
}

int amp_device_getopt(int argc, char **argv)
{
	return device_ops->getopt(argc, argv);
}

void amp_device_usage(void)
{
	device_ops->usage();
}
