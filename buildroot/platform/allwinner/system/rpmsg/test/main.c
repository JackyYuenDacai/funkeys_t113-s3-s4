#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include <librpmsg.h>

#include "md5.h"

#define POLL_RECV

#define PRINTF_CNT				1000

static int do_verbose = 0;
static unsigned int rx_threshold  = 0;
static unsigned int tx_threshold  = 0;
struct rpmsg_ept_instance *eptdev;
const char *ctrl_name;

struct rpmsg_ept_instance {
	int ept_id;
	int ept_fd;

	pthread_t recv_thread;
	volatile int should_exit;
};

static void print_help_msg(void)
{
	printf("\n");
	printf("USAGE:\n");
	printf("  rpmsg_test [OPTIONS]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("  -r name     : remoteproc process name.\n");
	printf("  -c name     : create specifies name endpoint\n");
	printf("  -d id       : delete specifies id endpoint\n");
	printf("  -L tx_len   : specifies tx_len\n");
	printf("  -t ms       : specifies tx_delay\n");
	printf("  -v          : printf verbose info\n");
	printf("  -R threshold : rx warn threshold\n");
	printf("  -T threshold : tx warn threshold\n");
	printf("e.g.\n");
	printf("  rpmsg_test -r dsp_rproc@0 -c test -L 64 -t 500: create an"
					" endpoint to communicate with dsp_rproc@0"
					" called test\n");
	printf("\n");
}

static void delay(uint32_t t)
{
	struct timeval d;

	d.tv_sec = t / 1000;
	d.tv_usec = (t % 1000) * 1000;

	select(0, NULL, NULL, NULL, &d);
}

static long long get_currenttime()
{
	struct timespec time;

	clock_gettime(CLOCK_REALTIME_COARSE, &time);

	return (time.tv_sec * 1000 * 1000 + time.tv_nsec / 1000);
}

static void mkdata(uint8_t *buffer, int size, int verbose, const char *name)
{
	int i;
	int data_len = size - 16;
	uint32_t *pdate = (uint32_t *)buffer;

	srand((int)time(NULL));

	/* generate random data */
	for (i = 0; i < (data_len / 4); i++)
		pdate[i] = (uint32_t)rand();

	md5_digest(buffer, data_len, &buffer[data_len]);

	if (data_len > 16)
		data_len = 16;

	if (verbose) {
		printf("[%s]data:", name);
		for (i = 0; i < data_len; i++)
			printf("%02x", buffer[i]);
		printf("... [md5:");
		for (i = 0; i < 16; i++)
			printf("%02x", buffer[size - 16 + i]);
		printf("]\n\n");
	}
}

static int checkdata(uint8_t *buffer, int size, int verbose, const char *name)
{
	int i;
	int data_len = size - 16;
	uint8_t digest[16];

	md5_digest(buffer, data_len, digest);

	if (data_len > 16)
		data_len = 16;

	if (verbose) {
		printf("data:");
		for (i = 0; i < data_len; i++)
			printf("%02x", buffer[i]);
		printf("... check:");
	}

	for (i = 0; i < 16; i++) {
		if (verbose)
			printf("%02x", buffer[size - 16 + i]);
		if (buffer[size - 16 + i] != digest[i])
			break;
	}

	if (i != 16) {
		printf("[%s] failed\n\n", name);
		for (i = 0; i < 16; i++)
			printf("%02x", buffer[size - 16 + i]);
		printf(" <-> cur:");
		for (i = 0; i < 16; i++)
			printf("%02x", digest[i]);
		return 0;
	} else {
		if (verbose)
			printf("[%s] success\n\n", name);
		return 1;
	}
}

static void *rpmsg_recv_thread(void *arg)
{
	struct rpmsg_ept_instance *eptdev = arg;
	uint8_t rx_buf[RPMSG_DATA_MAX_LEN];
	char name[64];
	uint32_t ret;
	long long start_usec, end_usec;
	float delta_usec = 0;
	unsigned long rev_cnt = 0, rev_bytes = 0;
#ifdef POLL_RECV
	struct pollfd poll_fds[1] = {
		{
			.fd = eptdev->ept_fd,
			.events = POLLIN,
		}
	};
#endif

	snprintf(name, sizeof(name), "rpmsg%d", eptdev->ept_id);
	printf("%s recv_thread start.\r\n", name);

	while (1) {
		if (eptdev->should_exit)
			break;

		start_usec = get_currenttime();
#ifdef POLL_RECV
		ret = poll(poll_fds, 1, 1000);
		if (ret < 0) {
			if (errno == EINTR) {
				printf("ept %s: signal occurred\n", name);
				break;
			} else {
				printf("ept %s: poll error (%s)\n", name, strerror(errno));
				continue;
			}
		} else if (ret == 0) { /* timeout */
			if (eptdev->should_exit) {
				break;
			} else {
				printf("polling /dev/%s timeout.\r\n", name);
				continue;
			}
		}
#endif
		ret = read(eptdev->ept_fd, rx_buf, sizeof(rx_buf));
		if (ret < 0) {
			printf("%s read file error=%s.\r\n", name, strerror(ret));
			continue;
		}
		end_usec = get_currenttime();
		delta_usec += (end_usec - start_usec);
		rev_cnt++;
		rev_bytes += ret;
		if (rx_threshold && (end_usec - start_usec) > (rx_threshold * 1000)) {
			printf("[%s] receive too long: expect < %dus,cur %dus  \n",
							name,
							rx_threshold * 1000,
							(int)(end_usec - start_usec));
		}
		if (do_verbose || !(rev_cnt % PRINTF_CNT)) {
			printf("[%s] receive : %fKb %fms %fMb/s\n", name,
							rev_bytes / 1000.0,
							delta_usec / 1000.0,
							rev_bytes / delta_usec);
			rev_bytes = 0;
			delta_usec = 0;
		}

		checkdata(rx_buf, ret, do_verbose, name);
	}

	printf("%s recv_thread exit.\r\n", name);
	pthread_exit(NULL);
}

static void when_signal(int sig)
{
	switch (sig) {
	case SIGINT:
		printf("SIGIN occurred, stop\n");
		eptdev->should_exit = 1;
		break;
	default:
		printf("Signal %d occurred, do nothing\n", sig);
		break;
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int c;

	const char *ept_name = NULL;
	int ept_id = -1;
	long long_tmp, tx_delay = 500, tx_len = 32;
	char ept_file_path[64];
	uint8_t send_buf[RPMSG_DATA_MAX_LEN];
	long long start_usec, end_usec;
	unsigned long send_cnt = 0;
	unsigned long send_bytes = 0;
	float delta_usec = 0;

	if (argc <= 1) {
		print_help_msg();
		ret = -1;
		goto out;
	}

	while ((c = getopt(argc, argv, "hr:c:d:L:t:R:T:v")) != -1) {
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
			break;
		case 'L':
			long_tmp = strtol(optarg, NULL, 0);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX ||
							long_tmp > RPMSG_DATA_MAX_LEN) {
				printf("Invalid input %s.\n", optarg);
				print_help_msg();
				ret = -EINVAL;
				goto out;
			}
			tx_len = long_tmp;
			break;
		case 't':
			long_tmp = strtol(optarg, NULL, 0);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				printf("Invalid input %s.\n", optarg);
				print_help_msg();
				ret = -EINVAL;
				goto out;
			}
			tx_delay = long_tmp;
			break;
		case 'R':
			long_tmp = strtol(optarg, NULL, 0);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				printf("Invalid input %s.\n", optarg);
				print_help_msg();
				ret = -EINVAL;
				goto out;
			}
			rx_threshold = long_tmp;
			break;
		case 'T':
			long_tmp = strtol(optarg, NULL, 0);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				printf("Invalid input %s.\n", optarg);
				print_help_msg();
				ret = -EINVAL;
				goto out;
			}
			tx_threshold = long_tmp;
			break;
		case 'v':
			do_verbose = 1;
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

	if (!ept_name) {
		printf("Please use -c option to specifies the endpoint name.\r\n");
		goto out;
	}

	ret = rpmsg_alloc_ept(ctrl_name, ept_name);
	if (ret < 0) {
		printf("rpmsg_alloc_ept for ept %s (%s) failed\n", ept_name, ctrl_name);
		ret = -1;
		goto out;
	}
	ept_id = ret;

	eptdev = malloc(sizeof(*eptdev));
	if (!eptdev) {
		printf("malloc eptdev for rpmsg%d failed\r\n", ept_id);
		ret = -ENOMEM;
		goto free_ept;
	}
	memset(eptdev, 0, sizeof(*eptdev));
	eptdev->ept_id = ept_id;

	snprintf(ept_file_path, sizeof(ept_file_path),
				"/dev/rpmsg%d", ept_id);
	eptdev->ept_fd = open(ept_file_path, O_RDWR);
	if (eptdev->ept_fd < 0) {
		printf("open %s failed\r\n", ept_file_path);
		goto free_eptdev;
	}

	ret = pthread_create(&eptdev->recv_thread, NULL,
			rpmsg_recv_thread, eptdev);
	if (ret != 0) {
		printf("rpmsg%d recv_thread create failed\n", ept_id);
		ret = -1;
		goto close_ept;
	}

	signal(SIGINT, when_signal);

	while (1) {
		mkdata(send_buf, tx_len, do_verbose, &ept_file_path[5]);
		start_usec = get_currenttime();
		ret = write(eptdev->ept_fd, send_buf, tx_len);
		if (ret < 0) {
			printf("%s write error=%s\n", &ept_file_path[5], strerror(ret));
			goto close_thread;
		}
		end_usec = get_currenttime();
		send_cnt++;
		send_bytes += ret;
		delta_usec += (end_usec - start_usec);
		if (tx_threshold && (end_usec - start_usec) > (tx_threshold * 1000)) {
			printf("[%s] send too long: expect < %dus,cur %dus  \n",
							&ept_file_path[5],
							tx_threshold * 1000,
							(int)(end_usec - start_usec));
		}
		if (do_verbose || !(send_cnt % PRINTF_CNT)) {
			printf("[%s] send: %fKb %fms %fM/s\n", &ept_file_path[5],
							send_bytes / 1000.0,
							delta_usec / 1000.0,
							send_bytes / delta_usec);
			delta_usec = 0.0;
			send_bytes = 0;
		}
		if (eptdev->should_exit)
			break;
		delay(tx_delay);
	}

close_thread:
	eptdev->should_exit = 1;
	pthread_join(eptdev->recv_thread, NULL);
close_ept:
	close(eptdev->ept_fd);
free_eptdev:
	free(eptdev);
free_ept:
	ret = rpmsg_free_ept(ctrl_name, ept_id);
	if (ret) {
		printf("rpmsg_free_ept for /dev/rpmsg%d (%s) failed\n", ept_id, ctrl_name);
		ret = -1;
		goto out;
	}
	ret = 0;
	printf("Delete rpmsg endpoint file: /dev/rpmsg%d\n", ept_id);
out:
	return ret;
}
