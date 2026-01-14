#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <librpmsg.h>

static void print_help_msg(void)
{
	printf("\n");
	printf("USAGE:\n");
	printf("  rpmsg_demo [OPTIONS]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("  -r name     : remoteproc process name.\n");
	printf("  -c name     : create specifies name endpoint\n");
	printf("  -d id       : delete specifies id endpoint\n");
	printf("e.g.\n");
	printf("  rpbuf_demo -r dsp_rproc -c test : create an"
					" endpoint to communicate with dsp_rproc"
					" called test\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int c;

	int do_create = 0;
	int do_delete = 0;
	const char *ctrl_name = NULL;
	const char *ept_name = NULL;
	int ept_id = -1;
	long long_tmp;

	if (argc <= 1) {
		print_help_msg();
		ret = -1;
		goto out;
	}

	while ((c = getopt(argc, argv, "hr:c:d:")) != -1) {
		switch (c) {
		case 'h':
			print_help_msg();
			ret = 0;
			goto out;
		case 'r':
			ctrl_name = optarg;
			break;
		case 'c':
			ept_name = optarg;
			do_create = 1;
			break;
		case 'd':
			long_tmp = strtol(optarg, NULL, 0);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				printf("Invalid input %s.\n", optarg);
				print_help_msg();
				ret = -EINVAL;
				goto out;
			}
			ept_id = long_tmp;
			do_delete = 1;
			break;
		default:
			printf("Invalid option: -%c\n", c);
			print_help_msg();
			ret = -1;
			goto out;
		}
	}

	if (!ctrl_name) {
		printf("Please use -r option to specifies the remoteproc process name.\r\n");
		goto out;
	}

	if (do_create) {
		if (!ept_name) {
			printf("Please use -c option to specifies the endpoint name.\r\n");
			goto out;
		}
		ret = rpmsg_alloc_ept(ctrl_name, ept_name);
		if (!ret) {
			printf("rpmsg_alloc_ept for ept %s (%s) failed\n", ept_name, ctrl_name);
			ret = -1;
			goto out;
		}

		printf("Create new rpmsg endpoint file: /dev/rpmsg%d\n", ret);
		printf("you can access the /dev/rpmsg%d file like a normal file.\r\n", ret);
		printf("e.g.\r\n");
		printf("\techo hello > /dev/rpmsg%d\r\n", ret);
		printf("\tcat /dev/rpmsg%d\r\n", ret);
	}

	if (do_delete) {
		if (ept_id < 0) {
			printf("Please use -d option to specifies the rpmsg id.\r\n");
			goto out;
		}

		ret = rpmsg_free_ept(ctrl_name, ept_id);
		if (ret) {
			printf("rpmsg_free_ept for /dev/rpmsg%d (%s) failed\n", ept_id, ctrl_name);
			ret = -1;
			goto out;
		}

		printf("Delete rpmsg endpoint file: /dev/rpmsg%d\n", ept_id);
	}

out:
	return ret;
}
