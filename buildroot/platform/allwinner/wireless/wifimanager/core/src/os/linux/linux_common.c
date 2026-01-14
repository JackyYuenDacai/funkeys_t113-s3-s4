#include <wmg_sta.h>
#include <wifi_log.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <net/if_arp.h>
#include <wpa_ctrl.h>
#include <linux_common.h>
#include <linux_get_config.h>
#include <linux_sta.h>
#include <linux_p2p.h>
#include <linux/event.h>
#include <utils.h>
#include <errno.h>
#include <poll.h>
#include <udhcpc.h>
#include <pthread.h>

uint8_t char2uint8(char* trs)
{
	uint8_t ret = 0;
	uint8_t tmp_ret[2] = {0};
	int i = 0;
	for(; i < 2; i++) {
		switch (*(trs + i)) {
			case '0' :
				tmp_ret[i] = 0x0;
				break;
			case '1' :
				tmp_ret[i] = 0x1;
				break;
			case '2' :
				tmp_ret[i] = 0x2;
				break;
			case '3' :
				tmp_ret[i] = 0x3;
				break;
			case '4' :
				tmp_ret[i] = 0x4;
				break;
			case '5' :
				tmp_ret[i] = 0x5;
				break;
			case '6' :
				tmp_ret[i] = 0x6;
				break;
			case '7' :
				tmp_ret[i] = 0x7;
				break;
			case '8' :
				tmp_ret[i] = 0x8;
				break;
			case '9' :
				tmp_ret[i] = 0x9;
				break;
			case 'a' :
				tmp_ret[i] = 0xa;
				break;
			case 'b' :
				tmp_ret[i] = 0xb;
				break;
			case 'c' :
				tmp_ret[i] = 0xc;
				break;
			case 'd' :
				tmp_ret[i] = 0xd;
				break;
			case 'e' :
				tmp_ret[i] = 0xe;
				break;
			case 'f' :
				tmp_ret[i] = 0xf;
		break;
	}
	WMG_DEBUG("change num[%d]: %d\n", i, tmp_ret[i]);
	}
	ret = ((tmp_ret[0] << 4) | tmp_ret[1]);
	return ret;
}

wmg_status_t linux_common_set_mac(const char *ifname, uint8_t *mac_addr)
{
	int sock_mac = -1;
	struct ifreq ifr_mac;
	int ret, i;

	/***** down the network *****/
	WMG_INFO("ioctl %s down\n", ifname);
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_mac == -1) {
		WMG_ERROR("down network(%s): create mac socket faile\n", ifname);
		return WMG_STATUS_FAIL;
	}
	strncpy(ifr_mac.ifr_name, ifname, (sizeof(ifr_mac.ifr_name) - 1));
	ifr_mac.ifr_flags &= ~IFF_UP;  //ifconfig   donw
	if((ioctl(sock_mac, SIOCSIFFLAGS, &ifr_mac)) < 0) {
		WMG_ERROR("down network(%s): mac ioctl error\n", ifname);
		close(sock_mac);
		return WMG_STATUS_FAIL;
	}
	close(sock_mac);
	sock_mac = -1;
	WMG_DEBUG("wait 3 second\n");
	sleep(3);

	/***** set mac addr to network *****/
	WMG_INFO("ioctl set %s mac: %02x:%02x:%02x:%02x:%02x:%02x\n",ifname, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	ifr_mac.ifr_addr.sa_family = ARPHRD_ETHER;
	strcpy(ifr_mac.ifr_name, ifname);

	for(i=0; i<6; i++) {
		ifr_mac.ifr_hwaddr.sa_data[i]= (char)mac_addr[i];
	}
	if((ioctl(sock_mac, SIOCSIFHWADDR, &ifr_mac)) < 0) {
		WMG_ERROR("set network(%s): mac ioctl error\n", ifname);
		close(sock_mac);
		return WMG_STATUS_FAIL;
	}
	close(sock_mac);
	sock_mac = -1;

	/***** up the network *****/
	WMG_INFO("ioctl %s up\n", ifname);
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	strncpy(ifr_mac.ifr_name, ifname, (sizeof(ifr_mac.ifr_name) - 1));
	ifr_mac.ifr_flags |= IFF_UP;    // ifconfig   up
	if((ioctl(sock_mac, SIOCSIFFLAGS, &ifr_mac)) < 0) {
		WMG_ERROR("up network(%s): mac ioctl error\n", ifname);
		close(sock_mac);
		return WMG_STATUS_FAIL;
	}

	close(sock_mac);
	return WMG_STATUS_SUCCESS;
}

wmg_status_t linux_common_get_mac(const char *ifname, uint8_t *mac_addr)
{
	int sock_mac = -1;
	char *pch;
	int i;

	struct ifreq ifr_mac;
	char mac_addr_buf[32];

	if(ifname == NULL) {
		WMG_ERROR("ifname is NULL\n");
		return WMG_STATUS_FAIL;
	}

	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_mac == -1)
	{
		WMG_ERROR("create mac socket faile\n");
		return WMG_STATUS_FAIL;
	}

	memset(&ifr_mac,0,sizeof(ifr_mac));
	strncpy(ifr_mac.ifr_name, ifname, (sizeof(ifr_mac.ifr_name) - 1));
	if((ioctl(sock_mac, SIOCGIFHWADDR, &ifr_mac)) < 0)
	{
		WMG_ERROR("mac ioctl(ifname:%s) error\n", ifname);
		close(sock_mac);
		return WMG_STATUS_FAIL;
	}

	sprintf(mac_addr_buf,"%02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned char)ifr_mac.ifr_hwaddr.sa_data[0],
			(unsigned char)ifr_mac.ifr_hwaddr.sa_data[1],
			(unsigned char)ifr_mac.ifr_hwaddr.sa_data[2],
			(unsigned char)ifr_mac.ifr_hwaddr.sa_data[3],
			(unsigned char)ifr_mac.ifr_hwaddr.sa_data[4],
			(unsigned char)ifr_mac.ifr_hwaddr.sa_data[5]);

	pch = strtok(mac_addr_buf, ":");
	for(i = 0;(pch != NULL) && (i < 6); i++){
		mac_addr[i] = char2uint8(pch);
		pch = strtok(NULL, ":");
	}

	WMG_DEBUG("local mac:%s\n",mac_addr_buf);

	close(sock_mac);
	return WMG_STATUS_SUCCESS;
}

//========================================== for linux wpa_supplicant =========================================
#define IFACE_VALUE_MAX 32
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char wifimanager_config_file_path[] = "/etc/wifi/wifimg.config";

static const char primary_iface_sta[] = "wlan0";
static const char primary_iface_p2p[] = "p2p-dev-wlan0";
static const char primary_iface_ap[] = "wlan0";
static const char WPA_IFACE_PATH[]           = "/etc/wifi/wpa_supplicant/sockets";
static const char HAPD_IFACE_PATH[]          = "/etc/wifi/hostapd/sockets";
static char primary_iface[IFACE_VALUE_MAX];
static const char SUPP_CONFIG_FILE[]    = "/etc/wifi/wpa_supplicant/wpa_supplicant.conf";
static const char SUPP_P2P_CONFIG_FILE[]= "/etc/wifi/wpa_supplicant/wpa_supplicant_p2p.conf";
static const char HAPD_CONFIG_FILE[]= "/etc/wifi/hostapd/hostapd.conf";

static const char IFNAME[]              = "IFNAME=";
#define IFNAMELEN (sizeof(IFNAME) - 1)
static const char WPA_EVENT_IGNORE[]    = "CTRL-EVENT-IGNORE ";

#define WPAD_STA_BITMAP    0x1
#define WPAD_P2P_BITMAP    0x2
#define WPAD_AP_BITMAP     0x4

#define XR8X9 1

//0x2: 010 -> XR8X9: (0)only use one wpa_supplicant
//                   (1)sta and p2p can coexist(if only sta or p2p is closed, wpa_supplicant cannot be closed)
//                   (0)wpa_supplicant and hostapd cannot exist at the same time
#define WPAD_INDEP_BITMAP_DEF  0x2

#define TRY_TIMES   3

static uint8_t linux_support_list[16] = {1,2,4,0,0,0,0,0,0,0,0,0,0,0,0,0};
static mode_support_list_t  linux_mode_support_list = {
	.item_num = 3,
	.item_table = &linux_support_list,
};

static wmg_status_t update_support_list(mode_support_list_t *support_list, char *support_list_config)
{
	uint8_t tmp_support_list[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int i;
	int tmp_value;
	char *pch;

	pch = strtok(support_list_config, ":");
	for(i=0;(pch != NULL) && (i < 16); i++) {
		tmp_value = atoi(pch);
		if(tmp_value < 16) {
			tmp_support_list[i] = (uint8_t)tmp_value;
			pch = strtok(NULL, ":");
		} else {
			WMG_ERROR("support list config illegal(%d)\n", tmp_value);
			return WMG_STATUS_FAIL;
		}
	}
	memcpy(support_list->item_table, tmp_support_list, 16 * sizeof(uint8_t));
	support_list->item_num = i + 1;
	return WMG_STATUS_SUCCESS;
}

mode_support_list_t* wmg_mode_support_list_register(void)
{
	char support_list_buf[64];
	if(!get_config(wifimanager_config_file_path, "support_list", support_list_buf)) {
		WMG_DEBUG("get linux support list: %s, update support list now\n", support_list_buf);
		update_support_list(&linux_mode_support_list, support_list_buf);
	} else {
		WMG_DEBUG("get linux support list fail, use default support list now\n", support_list_buf);
	}
	return &linux_mode_support_list;
}

static wmg_status_t update_wpad_indep_bitmap(uint8_t *bitmap)
{
	uint8_t tmp_bitmap;
	char wpad_indep_bitmap_buf[64];
	if(!get_config(wifimanager_config_file_path, "wpad_indep_bitmap", wpad_indep_bitmap_buf)) {
		tmp_bitmap = (uint8_t)atoi(wpad_indep_bitmap_buf);
		if(tmp_bitmap < 7) {
			WMG_DEBUG("get wpad indep bitmap %d, update bitmap now\n", tmp_bitmap);
			*bitmap = tmp_bitmap;
		} else {
			WMG_ERROR("wpad indep config illegal(%d)\n", tmp_bitmap);
		}
	} else {
		WMG_DEBUG("get wpad indep bitmap fail, use default %d\n", *bitmap);
	}
	return WMG_STATUS_SUCCESS;
}

typedef struct {
	pthread_mutex_t wpa_supplicant_mutex;
	struct wpa_ctrl *ctrl_conn[3];
	struct wpa_ctrl *monitor_conn[3];
	os_net_thread_t pid[3];
	void * thread_args[3];
	dispatch_event_t wpad_dispatch_event[3];
	void *private_data[3];
	uint8_t wpad_bitmap;
	/*
	 * This bitmap is used to indicate whether the mode is independent;
	 * bit[0]: Are there 2 wpa_supplicant? 1 yes, 0 no;
	 *        If there are 2 wpa_supplicant, sta and p2p are independent
	 * bit[1]: Auxiliary bit, used to indicate whether sta and p2p are independent, only works if bit 1 is 0;
	 * bit[2]: Are wpa_supplicant and hostapd independent and can they coexist? 1 yes, 0 no;
	 * */
	uint8_t wpad_indep_bitmap;
} wpad_object_t;

static wpad_object_t wpad_object;

static wmg_status_t stop_process(char* process)
{
	char cmd[128] = {0};
	cmd[127] = '\0';
	int try_cnt = 0;

	sprintf(cmd, "pidof %s | xargs kill -9", process);

	for(; check_process_is_exist(process, strlen(process)); try_cnt++) {
		if(try_cnt < TRY_TIMES) {
			system(cmd);
			sleep(1);
		} else {
			return WMG_STATUS_FAIL;
		}
	}

	return WMG_STATUS_SUCCESS;
}

//some process start with para, if process takes a para, use process_para, otherwise use process
static wmg_status_t start_process(char* process, char* process_para)
{
	char cmd[256] = {0};
	int try_cnt = 0;

	if(process_para != NULL) {
		sprintf(cmd, "%s", process_para);
	} else {
		sprintf(cmd, "%s", process);
	}

	for(; !check_process_is_exist(process, strlen(process)); try_cnt++) {
		if(try_cnt < TRY_TIMES) {
			system(cmd);
			sleep(1);
		} else {
			return WMG_STATUS_FAIL;
		}
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t start_wpad(int mode_type)
{
	char process[18] = {0};
	char cmd[256] = {0};
	char debug_flag[10] = {0};

	if(wmg_get_debug_level() != 6) {
		sprintf(debug_flag, "-B");
	} else {
		if(mode_type == WPAD_MODE_AP) {
			sprintf(debug_flag, "-dd -B");
		} else {
			sprintf(debug_flag, "-ddd &");
		}
	}

	update_wpad_indep_bitmap(&wpad_object.wpad_indep_bitmap);

	if(mode_type == WPAD_MODE_AP) {
		if(!((wpad_object.wpad_indep_bitmap >> 2) & 0x1)) {
			WMG_DEBUG("wpa_supplicant and hostapd cannot exist at the same time, need to close wpa_supplicant fist\n");
			if(stop_process("wpa_supplicant")) {
				WMG_ERROR("stop wpa_supplicant fail\n");
				return WMG_STATUS_FAIL;
			}
		}
		sprintf(process, "hostapd");
		sprintf(cmd, "hostapd %s %s", debug_flag, HAPD_CONFIG_FILE);
	} else {
		if(!((wpad_object.wpad_indep_bitmap >> 2) & 0x1)) {
			WMG_DEBUG("wpa_supplicant and hostapd cannot exist at the same time, need to close hostapd fist\n");
			if(stop_process("hostapd")) {
				WMG_ERROR("stop hostapd fail\n");
				return WMG_STATUS_FAIL;
			}
		}
		//2 wpa_supplicant
		if(wpad_object.wpad_indep_bitmap & 0x1) {
			if(mode_type == WPAD_MODE_STA) {
				sprintf(process, "wpa_supplicant");
				sprintf(cmd, "wpa_supplicant -i%s -Dnl80211 -c %s -O %s %s"
					, primary_iface_sta, SUPP_CONFIG_FILE, WPA_IFACE_PATH, debug_flag);
			} else if (mode_type == WPAD_MODE_P2P) {
				sprintf(process, "p2p_supplicant");
				sprintf(cmd, "p2p_supplicant -i%s -Dnl80211 -c %s -O %s %s"
					, primary_iface_p2p, SUPP_P2P_CONFIG_FILE, WPA_IFACE_PATH, debug_flag);
			}
		//1 wpa_supplicant
		} else {
			sprintf(process, "wpa_supplicant");
			sprintf(cmd, "wpa_supplicant -i%s -Dnl80211 -c %s -m %s -O %s %s"
					, primary_iface_sta, SUPP_CONFIG_FILE, SUPP_P2P_CONFIG_FILE, WPA_IFACE_PATH, debug_flag);
		}
	}

	if(start_process(process, cmd)){
		WMG_ERROR("start %s fail\n", cmd);
		return WMG_STATUS_FAIL;
	}

	if(mode_type == WPAD_MODE_STA) {
		wpad_object.wpad_bitmap |= WPAD_STA_BITMAP;
	} else if(mode_type == WPAD_MODE_P2P) {
		wpad_object.wpad_bitmap |= WPAD_P2P_BITMAP;
	} else if(mode_type == WPAD_MODE_AP) {
		wpad_object.wpad_bitmap |= WPAD_AP_BITMAP;
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t stop_wpad(int mode_type)
{
	if(mode_type == WPAD_MODE_AP) {
		if(stop_process("hostapd")) {
			WMG_ERROR("stop hostapd fail\n");
			return WMG_STATUS_FAIL;
		}
		system("ifconfig wlan0 down");
	} else {
		//2 wpa_supplicant
		if(wpad_object.wpad_indep_bitmap & 0x1) {
			if(mode_type == WPAD_MODE_STA) {
				if(stop_process("wpa_supplicant")) {
					WMG_ERROR("stop wpa_supplicant fail\n");
					return WMG_STATUS_FAIL;
				}
			} else if (mode_type == WPAD_MODE_P2P) {
				if(stop_process("p2p_supplicant")) {
					WMG_ERROR("stop p2p_supplicant fail\n");
					return WMG_STATUS_FAIL;
				}
			}
		//1 wpa_supplicant
		} else {
			uint8_t close_supplicant_flag = 0;
			if(mode_type == WPAD_MODE_STA) {
				close_supplicant_flag = wpad_object.wpad_bitmap & (~WPAD_STA_BITMAP);
			} else if(mode_type == WPAD_MODE_P2P) {
				close_supplicant_flag = wpad_object.wpad_bitmap & (~WPAD_P2P_BITMAP);
			}
			if(close_supplicant_flag & 0x3) {
				WMG_DEBUG("other mode use supplicant now, need not to stop supplicant\n");
				return WMG_STATUS_SUCCESS;
			} else {
				if(stop_process("wpa_supplicant")) {
					WMG_ERROR("stop wpa_supplicant fail\n");
					return WMG_STATUS_FAIL;
				}
			}
		}
	}

	if(mode_type == WPAD_MODE_STA) {
		wpad_object.wpad_bitmap &= (~WPAD_STA_BITMAP);
	} else if(mode_type == WPAD_MODE_P2P) {
		wpad_object.wpad_bitmap &= (~WPAD_P2P_BITMAP);
	} else if(mode_type == WPAD_MODE_AP) {
		wpad_object.wpad_bitmap &= (~WPAD_AP_BITMAP);
	}

	return WMG_STATUS_SUCCESS;
}

static int wifi_ctrl_recv(char *reply, size_t *reply_len, struct wpa_ctrl *monitor_conn)
{
	int res;
	int ctrlfd = wpa_ctrl_get_fd(monitor_conn);
	struct pollfd rfds;

	memset(&rfds, 0, sizeof(struct pollfd));
	rfds.fd = ctrlfd;
	rfds.events |= POLLIN;
	pthread_testcancel();
	res = TEMP_FAILURE_RETRY(poll(&rfds, 1, -1));
	pthread_testcancel();

	WMG_DEBUG("poll = %d, ctrlfd = %d, id = %lu\n", res, ctrlfd, pthread_self());

	if (res < 0) {
		WMG_ERROR("Error poll = %d\n", res);
		return res;
	}
	if (rfds.revents & POLLIN) {
		res = wpa_ctrl_pending(monitor_conn);
		if(res == 1){
			return wpa_ctrl_recv(monitor_conn, reply, reply_len);
		}
	}

	return res;
}

static int wifi_wait_on_socket(char *buf, size_t buflen, struct wpa_ctrl *monitor_conn)
{
	size_t nread = buflen - 1;
	int result;
	char *match, *match2;

	if (monitor_conn == NULL) {
		return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - connection closed");
	}

	result = wifi_ctrl_recv(buf, &nread, monitor_conn);

	if (result < 0) {
		WMG_ERROR("wifi_ctrl_recv failed(result:%d): %s\n", result, strerror(errno));
		return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - recv error");
	}
	WMG_EXCESSIVE("wifi_ctrl_recv: %s\n", buf);
	buf[nread] = '\0';
	/* Check for EOF on the socket */
	if (result == 0 && nread == 0) {
		/* Fabricate an event to pass up */
		WMG_EXCESSIVE("Received EOF on supplicant socket\n");
		return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - signal 0 received");
	}
	/*
	 * Events strings are in the format
	 *
	 *     IFNAME=iface <N>CTRL-EVENT-XXX
	 *        or
	 *     <N>CTRL-EVENT-XXX
	 *
	 * where N is the message level in numerical form (0=VERBOSE, 1=Excessive,
	 * etc.) and XXX is the event nae. The level information is not useful
	 * to us, so strip it off.
	 */
	if (strncmp(buf, IFNAME, IFNAMELEN) == 0) {
		match = strchr(buf, ' ');
		if (match != NULL) {
			if (match[1] == '<') {
				match2 = strchr(match + 2, '>');
				if (match2 != NULL) {
					nread -= (match2 - match);
					memmove(match + 1, match2 + 1, nread - (match - buf) + 1);
				}
			}
		} else {
			return snprintf(buf, buflen, "%s", WPA_EVENT_IGNORE);
		}
	} else if (buf[0] == '<') {
		match = strchr(buf, '>');
		if (match != NULL) {
			nread -= (match + 1 - buf);
			memmove(buf, match + 1, nread + 1);
			//printf("supplicant generated event without interface - %s\n", buf);
		}
	} else {
		/* let the event go as is! */
		//printf("supplicant generated event without interface and without message level - %s\n", buf);
	}

	return nread;
}


static void *wpas_event_thread(void *args)
{
	char buf[EVENT_BUF_SIZE] = {0};
	int size, ret, mode_type;
	mode_type = *(int *)args;

	WMG_DEBUG("create wpas event thread mode type %d\n",mode_type);

	for (;;) {
		size = wifi_wait_on_socket(buf, sizeof(buf), wpad_object.monitor_conn[mode_type]);
		if (size > 0) {
			ret = wpad_object.wpad_dispatch_event[mode_type](buf, size);
			if (ret) {
				WMG_DUMP("wpa_supplicant terminated\n");
				break;
			}
		} else {
			continue;
		}
	}

	WMG_INFO("event thread(mode_type:%d) exit now\n",mode_type);
	pthread_exit(NULL);
}

#define SUPPLICANT_TIMEOUT      3000000  // microseconds
#define SUPPLICANT_TIMEOUT_STEP  100000  // microseconds
static wmg_status_t wifi_connect_on_socket(int mode_type)
{
	static char path[PATH_MAX];
	int  supplicant_timeout = SUPPLICANT_TIMEOUT;
	int ret_tmp;
	wmg_status_t ret;

	//strncpy(primary_iface, "wlan0", IFACE_VALUE_MAX);
	if (mode_type == WPAD_MODE_STA) {
		snprintf(path, sizeof(path), "%s/%s", WPA_IFACE_PATH, primary_iface_sta);
	} else if (mode_type == WPAD_MODE_P2P) {
		snprintf(path, sizeof(path), "%s/%s", WPA_IFACE_PATH, primary_iface_p2p);
	} else if (mode_type == WPAD_MODE_AP) {
		snprintf(path, sizeof(path), "%s/%s", HAPD_IFACE_PATH, primary_iface_ap);
	}

	wpad_object.ctrl_conn[mode_type] = wpa_ctrl_open(path);
	while (wpad_object.ctrl_conn[mode_type] == NULL && supplicant_timeout > 0){
		usleep(SUPPLICANT_TIMEOUT_STEP);
		supplicant_timeout -= SUPPLICANT_TIMEOUT_STEP;
		wpad_object.ctrl_conn[mode_type] = wpa_ctrl_open(path);
	}
	if (wpad_object.ctrl_conn[mode_type] == NULL) {
		WMG_ERROR("Unable to open connection to wpad on \"%s\": %s\n",
			path, strerror(errno));
		return WMG_STATUS_FAIL;
	} else {
		WMG_DEBUG("open connection to wpad on \"%s\"\n", path);
	}

	wpad_object.monitor_conn[mode_type] = wpa_ctrl_open(path);
	if (wpad_object.monitor_conn[mode_type] == NULL) {
		WMG_ERROR("monitor_conn is NULL!\n");
		wpa_ctrl_close(wpad_object.ctrl_conn[mode_type]);
		wpad_object.ctrl_conn[mode_type] = NULL;
		return WMG_STATUS_FAIL;
	} else {
		WMG_DEBUG("open connection to wpad on \"%s\"\n", path);
	}

	if (wpa_ctrl_attach(wpad_object.monitor_conn[mode_type]) != 0) {
		WMG_ERROR("attach monitor_conn on \"%s\": %s\n",
			path, strerror(errno));
		WMG_ERROR("attach monitor_conn error!\n");
		wpa_ctrl_close(wpad_object.monitor_conn[mode_type]);
		wpa_ctrl_close(wpad_object.ctrl_conn[mode_type]);
		wpad_object.ctrl_conn[mode_type] = wpad_object.monitor_conn[mode_type] = NULL;
		return WMG_STATUS_FAIL;
	} else {
		WMG_DEBUG("attach monitor_conn on \"%s\"\n",path);
	}

	if(wpad_object.thread_args[mode_type] = malloc(sizeof(int))) {
		*(int *)wpad_object.thread_args[mode_type] = mode_type;
	} else {
		goto socket_err;
	}

	ret_tmp = os_net_thread_create(&wpad_object.pid[mode_type], NULL, wpas_event_thread,
			wpad_object.thread_args[mode_type], 0, 4096);
	if (ret_tmp) {
		WMG_ERROR("failed to create linux event(mode_type:%d) handle thread\n",mode_type);
		free(wpad_object.thread_args[mode_type]);
		wpad_object.thread_args[mode_type] = NULL;
		ret = WMG_STATUS_FAIL;
		goto socket_err;
	}
	WMG_DEBUG("create linux event(mode_type%d) handle thread success\n", mode_type);

	WMG_DEBUG("connect to wpa_supplicant ok!\n");
	return WMG_STATUS_SUCCESS;

socket_err:
		wpa_ctrl_close(wpad_object.monitor_conn[mode_type]);
		wpa_ctrl_close(wpad_object.ctrl_conn[mode_type]);
		wpad_object.ctrl_conn[mode_type] = wpad_object.monitor_conn[mode_type] = NULL;

	return ret;
}

static void wifi_close_sockets(int mode_type)
{
	char reply[4096] = {0};
	int ret = 0;

	if(wpad_object.pid[mode_type] != -1) {
		os_net_thread_delete(&wpad_object.pid[mode_type]);
		wpad_object.pid[mode_type] = -1;
		WMG_DEBUG("thread delete mode_type(%d)\n", mode_type);
	}

	if(wpad_object.thread_args[mode_type]) {
		free(wpad_object.thread_args[mode_type]);
		wpad_object.thread_args[mode_type] = NULL;
	}

	if (wpad_object.monitor_conn[mode_type] != NULL) {
		wpa_ctrl_detach(wpad_object.monitor_conn[mode_type]);
		wpa_ctrl_close(wpad_object.monitor_conn[mode_type]);
		wpad_object.monitor_conn[mode_type] = NULL;
	}

	if (wpad_object.ctrl_conn[mode_type] != NULL) {
		wpa_ctrl_close(wpad_object.ctrl_conn[mode_type]);
		wpad_object.ctrl_conn[mode_type] = NULL;
	}
}

wmg_status_t init_wpad(init_wpad_para_t wpad_para)
{
	if(start_wpad(wpad_para.mode_type)){
		return WMG_STATUS_FAIL;
	}

	if(wifi_connect_on_socket(wpad_para.mode_type))
	{
		stop_wpad(wpad_para.mode_type);
		return WMG_STATUS_FAIL;
	}

	wpad_object.wpad_dispatch_event[wpad_para.mode_type] = wpad_para.dispatch_event;
	wpad_object.private_data[wpad_para.mode_type] = wpad_para.linux_mode_private_data;

	return WMG_STATUS_SUCCESS;
}

wmg_status_t deinit_wpad(int mode_type)
{
	wifi_close_sockets(mode_type);
	stop_wpad(mode_type);
	wpad_object.wpad_dispatch_event[mode_type] = NULL;
	wpad_object.private_data[mode_type] = NULL;
	return WMG_STATUS_SUCCESS;
}

static int wifi_send_command(const char *cmd, char *reply, size_t *reply_len, int mode_type)
{
	int ret;
	if (wpad_object.ctrl_conn[mode_type] == NULL) {
		WMG_ERROR("Not connected to wpa_supplicant - \"%s\" command dropped.\n", cmd);
		return -1;
	}

	ret = wpa_ctrl_request(wpad_object.ctrl_conn[mode_type], cmd, strlen(cmd), reply, reply_len, NULL);
	if (ret == -2) {
		WMG_ERROR("'%s' command timed out.\n", cmd);
		return -2;
	} else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
		return -1;
	}
	if (strncmp(cmd, "PING", 4) == 0) {
		reply[*reply_len] = '\0';
	}
	return 0;
}

wmg_status_t command_to_wpad(char const *cmd, char *reply, size_t reply_len, int mode_type)
{
	pthread_mutex_lock(&wpad_object.wpa_supplicant_mutex);
	if(!cmd || !cmd[0]){
		pthread_mutex_unlock(&wpad_object.wpa_supplicant_mutex);
		return WMG_STATUS_FAIL;
	}

	WMG_EXCESSIVE("do cmd(mode_type %d) %s\n", mode_type, cmd);

	--reply_len; // Ensure we have room to add NUL termination.
	if (wifi_send_command(cmd, reply, &reply_len, mode_type) != 0) {
		pthread_mutex_unlock(&wpad_object.wpa_supplicant_mutex);
		return WMG_STATUS_FAIL;
	}

	WMG_EXCESSIVE("do cmd(mode_type %d) %s, reply: %s\n", mode_type, cmd, reply);
	// Strip off trailing newline.
	if (reply_len > 0 && reply[reply_len-1] == '\n') {
		reply[reply_len-1] = '\0';
	} else {
		reply[reply_len] = '\0';
	}
	pthread_mutex_unlock(&wpad_object.wpa_supplicant_mutex);
	return WMG_STATUS_SUCCESS;
}

static wpad_object_t wpad_object = {
	.wpa_supplicant_mutex = PTHREAD_MUTEX_INITIALIZER,
	.ctrl_conn = {NULL,NULL},
	.monitor_conn = {NULL,NULL},
	.pid = {-1,-1},
	.thread_args = {NULL,NULL},
	.wpad_dispatch_event = {NULL,NULL},
	.private_data = {NULL,NULL},
	.wpad_bitmap = 0x0,
	.wpad_indep_bitmap = WPAD_INDEP_BITMAP_DEF,
};
