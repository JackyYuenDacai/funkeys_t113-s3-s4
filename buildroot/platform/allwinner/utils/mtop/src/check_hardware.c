#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * get hardware spec value
 * @hardware:soc hardware name
 * @revision:
 * @chipid:soc diff-serial
 */
int get_hardware_name(char *hardware, int len)
{
	int dt_fd;
	ssize_t size;
	dt_fd = open("/proc/device-tree/model", O_RDONLY);
	if (dt_fd < 0) {
		printf("open /proc/device-tree/model fail!, ret:%d\n", dt_fd);
		return dt_fd;
	}

	size = read(dt_fd, hardware, len);
	if (size < 0) {
		printf("read hardware name fail!, ret:%ld\n", size);
		close(dt_fd);
		return size;
	}

	close(dt_fd);

	return 0;
}
