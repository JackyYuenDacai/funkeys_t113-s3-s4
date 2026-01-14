#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <wpa_ctrl.h>
#include <linux_sta.h>
#include <udhcpc.h>
#include <utils.h>
#include <wifi_log.h>
#include <unistd.h>
#include <wmg_sta.h>
#include <linux/event.h>
#include <linux/udhcpc.h>
#include <linux/scan.h>
#include <linux_common.h>
#include <sys/times.h>

#define DHCP_UPDATE_SECONDS 600

static wmg_sta_inf_object_t sta_inf_object;

#define LIST_ENTRY_NUME_MAX 64

typedef struct {
	char old_ssid[SSID_MAX_LEN + 1];
	char new_ssid[SSID_MAX_LEN + 1];
	int last_time;
} linux_private_data_t;

const char *wmg_sta_event_to_str(wifi_sta_event_t event)
{
	switch (event) {
	case WIFI_DISCONNECTED:
		return "DISCONNECTED";
	case WIFI_SCAN_STARTED:
		return "SCAN_STARTED";
	case WIFI_SCAN_FAILED:
		return "SCAN_FAILED";
	case WIFI_SCAN_RESULTS:
		return "SCAN_RESULTS";
	case WIFI_NETWORK_NOT_FOUND:
		return "NETWORK_NOT_FOUND";
	case WIFI_PASSWORD_INCORRECT:
		return "PASSWORD_INCORRECT";
	case WIFI_ASSOC_REJECT:
		return "ASSOC_REJECT";
	case WIFI_CONNECTED:
		return "CONNECTED";
	case WIFI_TERMINATING:
		return "TERMINATING";
	default:
		return "UNKNOWN";
	}
}

static wmg_status_t wpa_parse_status_info(char *status, wifi_sta_info_t *sta_info)
{
	int len;
	char *pos = NULL, *pre = NULL;
	char tmp[SSID_MAX_LEN + 1] = {0};

	char id[32] = {0};
	char freq[32] = {0};
	char bssid[18] = {0};
	char ssid[SSID_MAX_LEN + 1] = {0};
	char mac_addr[32] = {0};
	char ip_addr[32] = {0};
	char sec[32] = {0};
	int i;
	char *pch;

	if (status == NULL || sta_info == NULL) {
		WMG_ERROR("invalid parameters\n");
		return WMG_STATUS_INVALID;
	}

	pos = strstr(status, "wpa_state");
	if (pos != NULL) {
		pos += 10;
		if (strncmp(pos, "COMPLETED", 9) != 0) {
			WMG_WARNG("Warning: wpa state is inactive\n");
			return WMG_STATUS_FAIL;
		} else {
			WMG_DUMP("wpa state is completed\n");
			pch = strtok(status, "'\n'");
			while (pch != NULL) {
				if (strncmp(pch, "id=", 3) == 0) {
					strcpy(id, (pch + 3));
					WMG_DEBUG("%s\n", id);
				}
				if (strncmp(pch, "freq=", 5) == 0) {
					strcpy(freq, (pch + 5));
					WMG_DEBUG("%s\n", freq);
				}
				if (strncmp(pch, "bssid=", 6) == 0) {
					strcpy(bssid, (pch + 6));
					WMG_DEBUG("%s\n", bssid);
				}
				if (strncmp(pch, "ssid=", 5) == 0) {
					if((strlen(pch) - 5) > SSID_MAX_LEN) {
						WMG_DEBUG("ssid is too long:%d", (strlen(pch) - 5));
						return WMG_STATUS_INVALID;
					}
					strcpy(ssid, (pch + 5));
					WMG_DEBUG("%s\n", ssid);
				}
				if (strncmp(pch, "address=", 8) == 0) {
					strcpy(mac_addr, (pch + 8));
					WMG_DEBUG("%s\n", mac_addr);
				}
				if (strncmp(pch, "ip_address=", 11) == 0) {
					strcpy(ip_addr, (pch + 11));
					WMG_DEBUG("%s\n", ip_addr);
				}
				if (strncmp(pch, "key_mgmt=", 9) == 0) {
					strcpy(sec, (pch + 9));
					WMG_DEBUG("%s\n", sec);
				}
				pch = strtok(NULL, "'\n'");
			}
			sta_info->id = atoi(id);
			sta_info->freq = atoi(freq);
			i = 0;
			pch = strtok(bssid, ":");
			for(;(pch != NULL) && (i < 6); i++){
				sta_info->bssid[i] = char2uint8(pch);
				pch = strtok(NULL, ":");
			}
			strcpy(sta_info->ssid, ssid);
			i = 0;
			pch = strtok(mac_addr, ":");
			for(;(pch != NULL) && (i < 6); i++){
				sta_info->mac_addr[i] = char2uint8(pch);
				pch = strtok(NULL, ":");
			}
			i = 0;
			pch = strtok(ip_addr, ".");
			for(;(pch != NULL) && (i < 4); i++){
				sta_info->ip_addr[i] = atoi(pch);
				pch = strtok(NULL, ".");
			}
			if (strncmp(sec, "WPA2-PSK", 7) == 0) {
				sta_info->sec = WIFI_SEC_WPA2_PSK;
			} else if (strncmp(tmp, "WPA-PSK", 6) == 0) {
				sta_info->sec = WIFI_SEC_WPA_PSK;
			} else {
				sta_info->sec = WIFI_SEC_NONE;
			}
			return WMG_STATUS_SUCCESS;
		}
	} else {
		WMG_ERROR("status info is invalid\n");
		return WMG_STATUS_INVALID;
	}
}

static wmg_status_t wpa_parse_signal_info(char *status, wifi_sta_info_t *sta_info)
{
	char rssi[32] = {0};
	char *pch;

	if (status == NULL || sta_info == NULL) {
		WMG_ERROR("invalid parameters\n");
		return WMG_STATUS_INVALID;
	}

	sta_info->rssi = 0;

	pch = strtok(status, "'\n'");
	while (pch != NULL) {
		if (strncmp(pch, "RSSI=", 5) == 0) {
			strcpy(rssi, (pch + 5));
			WMG_DEBUG("RSSI=%s\n", rssi);
			break;
		}
		pch = strtok(NULL, "'\n'");
	}
	sta_info->rssi = atoi(rssi);
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_command_to_supplicant(char const *cmd, char *reply, size_t reply_len)
{
	return command_to_wpad(cmd, reply, reply_len, WPAD_MODE_STA);
}

/**
 * get ap(ssid/key_mgmt) status in wpa_supplicant.conf
 * return
 * -1: not exist
 *  1: exist but not connected
 *  3: exist and connected; network id in buffer net_id
 */
static int wpa_conf_is_ap_exist(const char *ssid, wifi_secure_t key_mgmt, char *net_id, int *len)
{
	int ret = -1;
	char cmd[CMD_LEN + 1] = {0};
	char reply[REPLY_BUF_SIZE] = {0}, key_type[128], key_reply[128];
	char *pssid_start = NULL, *pssid_end = NULL, *ptr = NULL;
	int flag = 0;

	if (!ssid || !ssid[0]) {
		WMG_ERROR("ssid is NULL!\n");
		return -1;
	}

	/* parse key_type */
	if (key_mgmt == WIFI_SEC_WPA_PSK || key_mgmt == WIFI_SEC_WPA2_PSK) {
		strncpy(key_type, "WPA-PSK", 128);
	} else if (key_mgmt == WIFI_SEC_WPA3_PSK) {
		strncpy(key_type, "SAE", 128);
	} else {
		strncpy(key_type, "NONE", 128);
	}

	strncpy(cmd, "LIST_NETWORKS", CMD_LEN);
	cmd[CMD_LEN] = '\0';
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("do list networks error!\n");
		return -1;
	}

	ptr = reply;
	while ((pssid_start = strstr(ptr, ssid)) != NULL) {
		char *p_s = NULL, *p_e = NULL, *p = NULL;

		pssid_end = pssid_start + strlen(ssid);
		/* ssid is presuffix of searched network */
		if (*pssid_end != '\t') {
			p_e = strchr(pssid_start, '\n');
			if (p_e != NULL) {
				ptr = p_e;
				continue;
			} else {
				break;
			}
		}

		flag = 0;

		p_e = strchr(pssid_start, '\n');
		if (p_e) {
			*p_e = '\0';
		}
		p_s = strrchr(ptr, '\n');
		p_s++;

		if (strstr(p_s, "CURRENT")) {
			flag = 2;
		}

		p = strtok(p_s, "\t");
		if (p) {
			if (net_id != NULL && *len > 0) {
				strncpy(net_id, p, *len - 1);
				net_id[*len - 1] = '\0';
			}
		}

		/* get key_mgmt */
		sprintf(cmd, "GET_NETWORK %s key_mgmt", net_id);
		cmd[CMD_LEN] = '\0';
		if(linux_command_to_supplicant(cmd, key_reply, sizeof(key_reply))) {
			WMG_ERROR("do get network %s key_mgmt error!\n", net_id);
			return -1;
		}

		WMG_EXCESSIVE("GET_NETWORK %s key_mgmt reply %s\n", net_id, key_reply);
		WMG_EXCESSIVE("new key type %s\n", key_type);

		if (strcmp(key_reply, key_type) == 0) {
			flag += 1;
			*len = strlen(net_id);
			break;
		} else {
			WMG_DEBUG("remove old same name network(net_id:%s) config\n", net_id);
			/* remove the same name network */
			sprintf(cmd, "REMOVE_NETWORK %s", net_id);
			if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
				WMG_ERROR("do remove network %s error!\n", net_id);
			}
		}

		if (p_e == NULL) {
			break;
		} else {
			*p_e = '\n';
			ptr = p_e;
		}
	}

	return flag;
}

/*
 * ssid to netid
 */
static int wpa_conf_ssid2netid(const char *ssid, int *net_id)
{
	int ret = -1,i = 0;
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[REPLY_BUF_SIZE] = {0};
	char *pch = NULL;
	char *delim = "'\n''\t'";
	int id = -1;
	int flags = 0;

	if (ssid == NULL) {
		WMG_ERROR("invalid parameters\n");
		return ret;
	}

	sprintf(cmd, "%s", "LIST_NETWORKS");
	cmd[CMD_LEN] = '\0';
	ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret) {
		WMG_ERROR("failed to list networks, reply %s\n", reply);
		return ret;
	}

	if(strlen(reply) < 34){
		WMG_INFO("Here has no entry save\n");
		return -1;
	}

	pch = strtok((reply + 34), delim);
	while(pch != NULL){
		id = atoi(pch);
		pch = strtok(NULL, delim);
		if(strcmp(ssid, pch) == 0){
			*net_id = id;
			flags = 1;
			break;
		}
		pch = strtok(NULL, delim);
		pch = strtok(NULL, delim);
		pch = strtok(NULL, delim);
	};

	WMG_DEBUG("%s net_id is:%d\n", ssid, *net_id);
	if(flags == 1) {
		return 0;
	} else {
		return -1;
	}
}

static int check_wpa_password(const char *passwd)
{
	int ret = -1, i;

	if (!passwd || *passwd =='\0') {
		WMG_ERROR("password is NULL\n");
		return ret;
	}

	for (i = 0; passwd[i] !='\0'; i++) {
		/* non printable char */
		if ((passwd[i] < 32) || (passwd[i] > 126)) {
			ret = -1;
			break;
		}
	}

	if (passwd[i] == '\0') {
		ret = 0;
	}

	return ret;
}

static wmg_status_t linux_sta_cmd_disconnect()
{
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[EVENT_BUF_SIZE] = {0};

	sprintf(cmd, "%s", "DISCONNECT");
	cmd[CMD_MAX_LEN] = '\0';
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("failed to disconnect ap, reply %s\n", reply);
		return WMG_STATUS_FAIL;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_sta_cmd_reconnect()
{
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[EVENT_BUF_SIZE] = {0};

	sprintf(cmd, "%s", "RECONNECT");
	cmd[CMD_MAX_LEN] = '\0';
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("failed to reconnect ap, reply %s\n", reply);
		return WMG_STATUS_FAIL;
	}
	return WMG_STATUS_SUCCESS;
}

static int linux_sta_cmd_status()
{
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[EVENT_BUF_SIZE] = {0};

	WMG_DUMP("wpa status\n");
	sprintf(cmd, "%s", "STATUS");
	cmd[CMD_MAX_LEN] = '\0';
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("failed to get sta status\n");
		return -1;
	}
	if (strstr(reply, "wpa_state=COMPLETED") != NULL) {
		WMG_DUMP("sta status is: connection\n");
		return 0;
	} else if (strstr(reply, "wpa_state=DISCONNECTED") != NULL) {
		WMG_DUMP("sta status is: disconnect\n");
		return 1;
	} else if (strstr(reply, "wpa_state=SCANNING") != NULL) {
		WMG_DUMP("sta status is: scanning\n");
		return 2;
	} else {
		WMG_DUMP("ap status is unknowd\n");
		return -1;
	}
}

static wmg_status_t wpas_select_network(wifi_sta_cn_para_t *cn_para, char *netid, int netid_len)
{
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[EVENT_BUF_SIZE] = {0};
	int passwd_len = 0;

	sprintf(cmd, "%s", "ADD_NETWORK");
	cmd[CMD_MAX_LEN] = '\0';
	if(linux_command_to_supplicant(cmd, netid, netid_len)) {
		WMG_ERROR("failed to add network, reply %s\n", netid);
		return WMG_STATUS_FAIL;
	}

	WMG_DEBUG("add network id=%s\n", netid);
	sprintf(cmd, "SET_NETWORK %s ssid \"%s\"", netid, cn_para->ssid);
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("failed to set network ssid '%s', reply %s\n", cn_para->ssid, reply);
		goto err_remvo_netid;
	}

	switch (cn_para->sec) {
	case WIFI_SEC_NONE:
		sprintf(cmd, "SET_NETWORK %s key_mgmt NONE", netid);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network key_mgmt(NONE), reply %s\n", reply);
			goto err_remvo_netid;
		}
		break;
	case WIFI_SEC_WPA_PSK:
	case WIFI_SEC_WPA2_PSK:
		sprintf(cmd, "SET_NETWORK %s key_mgmt WPA-PSK", netid);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network key_mgmt(WPA-PSK), reply %s\n", reply);
			goto err_remvo_netid;
		}

		/* check password */
		if(check_wpa_password(cn_para->password)) {
			WMG_ERROR("password is invalid\n");
			goto err_remvo_netid;
		}
		sprintf(cmd, "SET_NETWORK %s psk \"%s\"", netid, cn_para->password);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network psk, reply %s\n", reply);
			goto err_remvo_netid;
		}
		break;
	case WIFI_SEC_WPA3_PSK:
		sprintf(cmd, "SET_NETWORK %s key_mgmt SAE", netid);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network key_mgmt(SAE), reply %s\n", reply);
			goto err_remvo_netid;
		}

		/* check password */
		if(check_wpa_password(cn_para->password)) {
			WMG_ERROR("password is invalid\n");
			goto err_remvo_netid;
		}
		sprintf(cmd, "SET_NETWORK %s psk \"%s\"", netid, cn_para->password);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network psk, reply %s\n", reply);
			goto err_remvo_netid;
		}

		/* set ieee80211w */
		sprintf(cmd, "SET_NETWORK %s ieee80211w 1", netid);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set ieee80211w 1, reply %s\n", reply);
			goto err_remvo_netid;
		}
		break;
	case WIFI_SEC_WEP:
		sprintf(cmd, "SET_NETWORK %s key_mgmt NONE", netid);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network key_mgmt, reply %s\n", reply);
			goto err_remvo_netid;
		}

		passwd_len = strlen(cn_para->password);
		if((passwd_len == 10) || (passwd_len == 26)) {
			sprintf(cmd, "SET_NETWORK %s wep_key0 %s", netid, cn_para->password);
			WMG_DEBUG("The passwd is HEX format!\n");
		} else if((passwd_len == 5) || (passwd_len == 13)) {
			sprintf(cmd, "SET_NETWORK %s wep_key0 \"%s\"", netid, cn_para->password);
			WMG_DEBUG("The passwd is ASCII format!\n");
		} else {
			WMG_ERROR("The password does not conform to the specification!\n");
			goto err_remvo_netid;
		}
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network psk, reply %s\n", reply);
			goto err_remvo_netid;
		}
		sprintf(cmd, "SET_NETWORK %s auth_alg OPEN SHARED", netid);
		if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
			WMG_ERROR("failed to set network auth_alg OPEN SHARED, reply %s\n", reply);
			goto err_remvo_netid;
		}
		break;
	default:
		WMG_ERROR("unknown key mgmt\n");
		goto err_remvo_netid;
	}

	sprintf(cmd, "SET_NETWORK %s scan_ssid 1", netid);
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("failed to set network scan_ssid(1), reply %s\n", reply);
		goto err_remvo_netid;
	}

	sprintf(cmd, "SELECT_NETWORK %s", netid);
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("failed to select network %s, reply %s\n", netid, reply);
		goto err_remvo_netid;
	}

	return WMG_STATUS_SUCCESS;

err_remvo_netid:
	WMG_ERROR("select network faile, remove network now\n");
	sprintf(cmd, "REMOVE_NETWORK %s", netid);
	if(linux_command_to_supplicant(cmd, reply, sizeof(reply))) {
		WMG_ERROR("select network faile, do remove network %s error!\n", netid);
	}
	return WMG_STATUS_FAIL;
}

static int wpa_set_auto_reconn(wmg_bool_t enable)
{
	int ret;
	WMG_DEBUG("get reconnn flag:%d\n",enable);
	sta_inf_object.sta_auto_reconn = enable;
	if(enable == false){
		WMG_DEBUG("send disconnet cmd\n");
		if(linux_sta_cmd_status() == 2){
			ret = linux_sta_cmd_disconnect();
			if (ret) {
				WMG_ERROR("failed to send disconnect cmd\n");
				return ret;
			}
		}
	} else {
		linux_sta_cmd_reconnect();
	}
}

static int wpa_get_scan_results(get_scan_results_para_t *sta_scan_results_para)
{
	wmg_status_t ret;
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[SCAN_BUF_LEN] = {0};
	int event = -1, ret_tmp;
	int try_cnt = 0;

scan_once:
	evt_socket_clear(sta_inf_object.sta_event_handle);
	sprintf(cmd, "%s", "SCAN");
	cmd[CMD_MAX_LEN] = '\0';
	ret_tmp = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret_tmp) {
		if (strncmp(reply, "FAIL-BUSY", 9) == 0) {
			WMG_DUMP("wpa_supplicant is scanning internally\n");
			sleep(2);
			goto scan_results;
		}
		ret = WMG_STATUS_FAIL;
		goto err;
	}

	while (try_cnt < SCAN_TRY_MAX) {
		ret_tmp = evt_read(sta_inf_object.sta_event_handle, (int *)&event);
		if (ret_tmp > 0) {
			WMG_DUMP("receive wpas event '%s'\n", wmg_sta_event_to_str(event));
			if (event == WIFI_SCAN_RESULTS) {
				break;
			} else if (event == WIFI_SCAN_FAILED) {
				if (try_cnt >= 2) {
					ret = WMG_STATUS_FAIL;
					goto err;
				}
				try_cnt++;
				goto scan_once;
			} else {
				try_cnt++;
			}
		} else {
			try_cnt++;
		}
	}

	if (try_cnt == SCAN_TRY_MAX && event != WIFI_SCAN_RESULTS) {
		ret = WMG_STATUS_FAIL;
		goto err;
	}

scan_results:
	try_cnt = 0;
	sprintf(cmd, "%s", "SCAN_RESULTS");
	cmd[CMD_MAX_LEN] = '\0';
	ret_tmp = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret_tmp) {
		sleep(1);
		try_cnt++;
		if (try_cnt == 2) {
			ret = WMG_STATUS_FAIL;
			goto err;
		}
		goto scan_results;
	}

	remove_slash_from_scan_results(reply);
	ret_tmp = parse_scan_results(reply, sta_scan_results_para->scan_results, sta_scan_results_para->bss_num, sta_scan_results_para->arr_size);
	if (ret_tmp) {
		WMG_ERROR("failed to parse scan results\n");
		ret = WMG_STATUS_FAIL;
	} else {
		ret = WMG_STATUS_SUCCESS;
	}

err:
	return ret;
}

static void wpas_event_notify_to_sta_dev(wifi_sta_event_t event)
{
	if (sta_inf_object.sta_event_cb) {
		sta_inf_object.sta_event_cb(event);
	}
}

static void wpas_event_notify(wifi_sta_event_t event)
{
	evt_send(sta_inf_object.sta_event_handle, event);
	wpas_event_notify_to_sta_dev(event);
}

static void try_start_udhcpc(char *inf)
{
	linux_private_data_t *private_data = NULL;
	private_data = (linux_private_data_t *)sta_inf_object.sta_private_data;
	int now_time = time((time_t *)NULL);
	WMG_DEBUG("**************************wait 1 seconds********************************\n");
	sleep(1);
	WMG_DEBUG("*************************try start udhcpc*******************************\n");

	wpas_event_notify(WIFI_DHCP_START);
	if((is_ip_exist(inf)) != 4) {
		WMG_DEBUG("IPv4 is not exist, need to start_udhcpc\n");
		start_udhcpc(inf);
		if((is_ip_exist(inf)) == 4) {
			wpas_event_notify(WIFI_DHCP_SUCCESS);
			strcpy(private_data->old_ssid, private_data->new_ssid);
			private_data->last_time = now_time;
		} else {
			wpas_event_notify(WIFI_DHCP_TIMEOUT);
		}
	} else {
		if(strcmp(private_data->old_ssid, private_data->new_ssid)){
			WMG_DEBUG("Old ssid(%s) and new ssid(%s) different, need to start_udhcpc\n",
					private_data->old_ssid, private_data->new_ssid);
			start_udhcpc(inf);
			if((is_ip_exist(inf)) == 4) {
				wpas_event_notify(WIFI_DHCP_SUCCESS);
				private_data->last_time = now_time;
				strcpy(private_data->old_ssid, private_data->new_ssid);
			} else {
				wpas_event_notify(WIFI_DHCP_TIMEOUT);
			}
		} else {
			if(((now_time) - (private_data->last_time)) > DHCP_UPDATE_SECONDS) {
				WMG_DEBUG("It(%s) took more than %d seconds to get IP, need to start_udhcpc\n",
						private_data->old_ssid, DHCP_UPDATE_SECONDS);
				start_udhcpc(inf);
				if((is_ip_exist(inf) == 4)) {
					wpas_event_notify(WIFI_DHCP_SUCCESS);
					private_data->last_time = now_time;
				} else {
					wpas_event_notify(WIFI_DHCP_TIMEOUT);
				}
			} else {
				WMG_INFO("(%s)The time to get IP does not exceed %d(%d) seconds, need not to start_udhcpc\n",
						private_data->old_ssid,DHCP_UPDATE_SECONDS, ((now_time) - (private_data->last_time)));
				wpas_event_notify(WIFI_DHCP_SUCCESS);
			}
		}
	}
	WMG_DEBUG("**************************try udhcpc end********************************\n");
}

static char sta_inf[] = "wlan0";
static int wpas_dispatch_event(const char *event_str, int nread)
{
	int ret, i = 0, event = 0;
	char event_nae[CMD_MAX_LEN];
	char cmd[CMD_MAX_LEN + 1] = {0}, reply[EVENT_BUF_SIZE] = {0};
	char *nae_start = NULL, *nae_end = NULL;

	if (!event_str || !event_str[0]) {
		WMG_WARNG("wpa_supplicant event is NULL!\n");
		return 0;
	}

	WMG_DUMP("receive wpa event: %s\n", event_str);
	if (strncmp(event_str, "CTRL-EVENT-", 11) != 0) {
		if (!strncmp(event_str, "WPA:", 4)) {
			if (strstr(event_str, "pre-shared key may be incorrect")){
				wpas_event_notify(WIFI_PASSWORD_INCORRECT);
				return 0;
			}
		}
		return 0;
	}

	nae_start = (char *)((unsigned long)event_str + 11);
	nae_end = strchr(nae_start, ' ');
	if (nae_end) {
		while ((nae_start < nae_end) && (i < 18)) {
			event_nae[i] = *nae_start++;
			i++;
		}
		event_nae[i] = '\0';
	} else {
		WMG_DUMP("received wpa_supplicant event with empty event nae!\n");
		return 0;
	}

	WMG_DUMP("event name:%s\n", event_nae);
	if (!strcmp(event_nae, "DISCONNECTED")) {
		wpas_event_notify(WIFI_DISCONNECTED);
		if(sta_inf_object.sta_auto_reconn == WMG_TRUE) {
			linux_sta_cmd_reconnect();
		}
	} else if (!strcmp(event_nae, "SCAN-STARTED")) {
		wpas_event_notify(WIFI_SCAN_STARTED);
	} else if (!strcmp(event_nae, "SCAN-RESULTS")) {
		wpas_event_notify(WIFI_SCAN_RESULTS);
	} else if (!strcmp(event_nae, "SCAN-FAILED")) {
		wpas_event_notify(WIFI_SCAN_FAILED);
	} else if (!strcmp(event_nae, "NETWORK-NOT-FOUND")) {
		wpas_event_notify(WIFI_NETWORK_NOT_FOUND);
	} else if (!strcmp(event_nae, "AUTH-REJECT")) {
		wpas_event_notify(WIFI_AUTH_REJECT);
	} else if(!strcmp(event_nae, "ASSOC-REJECT")) {
		wpas_event_notify(WIFI_ASSOC_REJECT);
		sprintf(cmd, "%s", "DISCONNECT");
		cmd[CMD_MAX_LEN] = '\0';
		ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
		if (ret) {
			WMG_ERROR("failed to disconnect from ap, reply %s\n", reply);
		}
	} else if (!strcmp(event_nae, "CONNECTED")) {
		wpas_event_notify(WIFI_CONNECTED);
		try_start_udhcpc(sta_inf);
	} else if (!strcmp(event_nae, "TERMINATING")) {
		wpas_event_notify(WIFI_TERMINATING);
		return 1;
	} else {
		event = WIFI_UNKNOWN;
	}

	return 0;
}

static wmg_status_t linux_sta_inf_init(sta_event_cb_t sta_event_cb,void *p)
{
	if(sta_inf_object.sta_init_flag == WMG_FALSE) {
		WMG_INFO("linux sta supplicant init now\n");

		sta_inf_object.sta_event_handle = (event_handle_t *)malloc(sizeof(event_handle_t));
		if(sta_inf_object.sta_event_handle != NULL) {
			memset(sta_inf_object.sta_event_handle, 0, sizeof(event_handle_t));
			sta_inf_object.sta_event_handle->evt_socket[0] = -1;
			sta_inf_object.sta_event_handle->evt_socket[1] = -1;
			sta_inf_object.sta_event_handle->evt_socket_enable = WMG_FALSE;
		} else {
			WMG_ERROR("failed to allocate memory for linux wpa event_handle\n");
			return WMG_STATUS_FAIL;
		}
		if(sta_event_cb != NULL){
			sta_inf_object.sta_event_cb = sta_event_cb;
		}
		memset(((linux_private_data_t *)(sta_inf_object.sta_private_data))->old_ssid, 0,
				SSID_MAX_LEN);
		memset(((linux_private_data_t *)(sta_inf_object.sta_private_data))->new_ssid, 0,
				SSID_MAX_LEN);
		sta_inf_object.sta_init_flag = WMG_TRUE;
	} else {
		WMG_INFO("linux supplicant already init\n");
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_sta_inf_deinit(void *p)
{
	if(sta_inf_object.sta_init_flag == WMG_TRUE) {
		system("ifconfig wlan0 down");
		memset(((linux_private_data_t *)(sta_inf_object.sta_private_data))->old_ssid, 0,
				SSID_MAX_LEN);
		memset(((linux_private_data_t *)(sta_inf_object.sta_private_data))->new_ssid, 0,
				SSID_MAX_LEN);
		if(sta_inf_object.sta_event_handle != NULL){
			free(sta_inf_object.sta_event_handle);
			sta_inf_object.sta_event_handle = NULL;
		}
		sta_inf_object.sta_init_flag = WMG_FALSE;
		sta_inf_object.sta_auto_reconn = WMG_FALSE;
		sta_inf_object.sta_event_cb = NULL;
	} else {
		WMG_DEBUG("linux supplicant already deinit\n");
	}
	return WMG_STATUS_SUCCESS;
}

/* Establishes the control and monitor socket connections on the interface */
static wmg_status_t linux_sta_inf_enable()
{
	init_wpad_para_t wpa_supplicant_para = {
		.mode_type = WPAD_MODE_STA,
		.dispatch_event = wpas_dispatch_event,
		.linux_mode_private_data = sta_inf_object.sta_private_data,
	};

	if(init_wpad(wpa_supplicant_para)) {
		WMG_ERROR("init wpa_supplicant failed\n");
		return WMG_STATUS_FAIL;
	} else {
		if(evt_socket_init(sta_inf_object.sta_event_handle)) {
			WMG_ERROR("failed to initialize linux sta event socket\n");
			deinit_wpad(WPAD_MODE_STA);;
		}
		WMG_DEBUG("init wpa_supplicant success\n");
		sta_inf_object.sta_enable_flag = WMG_TRUE;
		return WMG_STATUS_SUCCESS;
	}
}

static wmg_status_t linux_sta_inf_disable()
{
	WMG_INFO("linux supplicant stop now\n");
	if (sta_inf_object.sta_enable_flag == WMG_TRUE) {
		if (deinit_wpad(WPAD_MODE_STA) == WMG_STATUS_SUCCESS) {
			evt_socket_exit(sta_inf_object.sta_event_handle);
			sta_inf_object.sta_enable_flag = WMG_FALSE;
			return WMG_STATUS_SUCCESS;
		}
	} else {
		WMG_INFO("linux supplicant already has stop\n");
		return WMG_STATUS_SUCCESS;
	}
	return WMG_STATUS_FAIL;
}

static wmg_status_t linux_supplicant_connect_to_ap(wifi_sta_cn_para_t *cn_para)
{
	wmg_status_t ret;
	int try_cnt = 0, len, ret_tmp, netid_len_buf;
	int event = -1;
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[EVENT_BUF_SIZE] = {0};
	char netid[CMD_MAX_LEN + 1] = {0};
	linux_sta_private_data_t *private_data = NULL;
	private_data = (linux_sta_private_data_t *)sta_inf_object.sta_private_data;

	len = CMD_MAX_LEN + 1;
	ret_tmp = wpa_conf_is_ap_exist(cn_para->ssid, cn_para->sec, netid, &len);
	if (ret_tmp > 0) {
		WMG_DUMP("current bss is exist\n");
		if (ret_tmp == 3) {
			WMG_DUMP("wifi is already connected to %s\n", cn_para->ssid);
			wpas_event_notify_to_sta_dev(WIFI_CONNECTED);
			return WMG_STATUS_SUCCESS;
		} else {
			sprintf(cmd, "REMOVE_NETWORK %s", netid);
			cmd[CMD_MAX_LEN] = '\0';
			ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
			if (ret) {
				WMG_ERROR("failed to remove network %s, reply %s\n", netid, reply);
				wpas_event_notify_to_sta_dev(WIFI_DISCONNECTED);
				return WMG_STATUS_FAIL;
			}
		}
	} else {
		WMG_WARNG("current bss is not exist, need to add it\n");
	}

	/* check ssid contains chinese or not */
	ret_tmp = evt_socket_clear(sta_inf_object.sta_event_handle);
	if (ret_tmp)
		WMG_WARNG("failed to clear event socket\n");

	strcpy(private_data->new_ssid, cn_para->ssid);

	netid_len_buf = len + 1;
	ret_tmp = wpas_select_network(cn_para, netid, netid_len_buf);
	if (ret_tmp) {
		WMG_ERROR("failed to config network\n");
		wpas_event_notify_to_sta_dev(WIFI_DISCONNECTED);
		return WMG_STATUS_FAIL;
	}

read_event:
	ret = WMG_STATUS_FAIL;
	/* wait connect event */
	while (try_cnt < EVENT_TRY_MAX) {
		ret_tmp = evt_read(sta_inf_object.sta_event_handle, &event);
		if (ret_tmp > 0) {
			WMG_DUMP("receive wpas event '%s'\n", wmg_sta_event_to_str(event));
			if (event == WIFI_CONNECTED) {
				ret = WMG_STATUS_NOT_READY;
			} else if (event == WIFI_DHCP_START) {
				ret = WMG_STATUS_NOT_READY;
			} else if (event == WIFI_DHCP_TIMEOUT) {
				ret = WMG_STATUS_FAIL;
				break;
			} else if (event == WIFI_DHCP_SUCCESS) {
				ret = WMG_STATUS_SUCCESS;
				break;
			} else if (event == WIFI_PASSWORD_INCORRECT) {
				break;
			} else if (event == WIFI_NETWORK_NOT_FOUND) {
			} else if (event == WIFI_ASSOC_REJECT) {
				break;
			} else {
				/* other event */
				try_cnt++;
			}
		} else {
			try_cnt++;
		}
	}

	if (ret == WMG_STATUS_SUCCESS) {
		sprintf(cmd, "%s", "SAVE_CONFIG");
		cmd[CMD_MAX_LEN] = '\0';
		ret_tmp = linux_command_to_supplicant(cmd, reply, sizeof(reply));
		if (!ret_tmp)
			WMG_DUMP("save config to wpa_supplicant.conf success\n");
		else
			WMG_WARNG("failed to save config to wpa_supplicant.conf\n");
	}

	if (ret != WMG_STATUS_SUCCESS) {
		sprintf(cmd, "%s %s", "REMOVE_NETWORK", netid);
		cmd[CMD_MAX_LEN] = '\0';
		ret_tmp = linux_command_to_supplicant(cmd, reply, sizeof(reply));
		if (!ret_tmp)
			WMG_DUMP("remove network %s because of connection failure\n", netid);
		else
			WMG_WARNG("failed to remove network %s\n", netid);
	}

	return ret;
}

static wmg_status_t linux_supplicant_disconnect_to_ap(void)
{
	wmg_status_t ret;
	if(linux_sta_cmd_status() != 0){
		WMG_DEBUG("sta is disconnected, need not to disconnect to ap\n");
		return WMG_STATUS_SUCCESS;
	}
	ret = linux_sta_cmd_disconnect();
	if (ret) {
		WMG_ERROR("failed to disconnect ap\n");
		return ret;
	}
	system("ifconfig wlan0 0.0.0.0");
	sta_inf_object.sta_auto_reconn = WMG_FALSE;
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_supplicant_get_info(wifi_sta_info_t *sta_info)
{
	wmg_status_t ret;
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[EVENT_BUF_SIZE] = {0};

	sprintf(cmd, "%s", "STATUS");
	cmd[CMD_MAX_LEN] = '\0';
	ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret) {
		WMG_ERROR("failed to get status of wifi station, reply %s\n", reply);
		return WMG_STATUS_FAIL;
	}
	ret = wpa_parse_status_info(reply, sta_info);
	if(ret) {
		WMG_ERROR("failed to parse station info\n");
		return WMG_STATUS_FAIL;
	}

	sprintf(cmd, "%s", "SIGNAL_POLL");
	ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret) {
		WMG_ERROR("failed to get signal of wifi station, reply %s\n", reply);
		return WMG_STATUS_FAIL;
	}
	ret = wpa_parse_signal_info(reply, sta_info);
	if(ret) {
		WMG_ERROR("failed to parse signal info\n");
		return WMG_STATUS_FAIL;
	}
	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_supplicant_list_networks(wifi_sta_list_t *sta_list)
{
	wmg_status_t ret;
	int i = 0, list_entry_num = 0;
	char cmd[CMD_MAX_LEN + 1] = {0};
	char reply[REPLY_BUF_SIZE] = {0};
	char *pch_entry_p[LIST_ENTRY_NUME_MAX] = {0};
	char *pch_entry = NULL;
	char *pch_item = NULL;
	char *delim_entry = "'\n'";
	char *delim_item = "'\t'";

	if (sta_list == NULL) {
		WMG_ERROR("invalid parameters\n");
		return WMG_STATUS_INVALID;
	}

	if(sta_list->list_num > LIST_ENTRY_NUME_MAX) {
		WMG_WARNG("Sys only support list %d entry\n", LIST_ENTRY_NUME_MAX);
		list_entry_num = LIST_ENTRY_NUME_MAX;
	} else {
		list_entry_num = sta_list->list_num;
	}

	sprintf(cmd, "%s", "LIST_NETWORKS");
	cmd[CMD_LEN] = '\0';
	ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret) {
		WMG_ERROR("failed to list networks, reply %s\n", reply);
		return ret;
	}

	if(strlen(reply) < 34){
		WMG_INFO("Here has no entry save\n");
		return WMG_STATUS_FAIL;
	}

	pch_entry = strtok((reply + 34), delim_entry);
	while((pch_entry != NULL) && (i < list_entry_num)) {
		pch_entry_p[i] = pch_entry;
		i++;
		pch_entry = strtok(NULL, delim_entry);
	}

	sta_list->sys_list_num = i;
	WMG_DEBUG("sys save num is:%d, list buff num is:%d\n", i, sta_list->list_num);

	for(i = 0; pch_entry_p[i] != NULL; i++) {
		pch_item = strtok(pch_entry_p[i], delim_item);
		if(pch_item != NULL) {
			sta_list->list_nod[i].id = atoi(pch_item);
		}
		pch_item = strtok(NULL, delim_item);
		if(pch_item != NULL) {
			strcpy((sta_list->list_nod[i].ssid), pch_item);
		}
		pch_item = strtok(NULL, delim_item);
		if(pch_item != NULL) {
			strcpy((sta_list->list_nod[i].bssid), pch_item);
		}
		pch_item = strtok(NULL, delim_item);
		if(pch_item != NULL) {
			strcpy((sta_list->list_nod[i].flags), pch_item);
		} else {
			strcpy((sta_list->list_nod[i].flags), "NULL");
		}
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_supplicant_remove_networks(char *ssid)
{
	wmg_status_t ret = WMG_STATUS_FAIL;
	char cmd[CMD_LEN +1 ] = {0};
	char reply[REPLY_BUF_SIZE] = {0};
	int net_id;
	char *pch = NULL;
	char *delim = "'\n''\t'";

	if(ssid != NULL) {
		WMG_DEBUG("remove network(%s) ...\n", ssid);
		/* check AP is exist in wpa_supplicant.conf */
		ret = wpa_conf_ssid2netid(ssid, &net_id);
		if (ret != 0) {
			WMG_WARNG("%s is not in wpa_supplicant.conf!\n", ssid);
			return WMG_STATUS_INVALID;
		}

		/* cancel saved in wpa_supplicant.conf */
		cmd[CMD_MAX_LEN] = '\0';
		sprintf(cmd, "REMOVE_NETWORK %d", net_id);
		ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
		if (ret) {
			WMG_ERROR("do remove network %s error!\n", net_id);
			return WMG_STATUS_FAIL;
		}

	} else {
		WMG_DEBUG("remove all network ...\n", ssid);
		sprintf(cmd, "%s", "LIST_NETWORKS");
		cmd[CMD_LEN] = '\0';
		ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
		if (ret) {
			WMG_ERROR("failed to list networks, reply %s\n", reply);
			return ret;
		}

		if(strlen(reply) < 34){
			WMG_INFO("Here has no entry save\n");
			return WMG_STATUS_INVALID;
		}

		pch = strtok((reply + 34), delim);
		while(pch != NULL){
			cmd[CMD_MAX_LEN] = '\0';
			sprintf(cmd, "REMOVE_NETWORK %s", pch);
			ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
			if (ret) {
				WMG_ERROR("do remove network %s error!\n", pch);
				return WMG_STATUS_FAIL;
			}
			pch = strtok(NULL, delim);
			pch = strtok(NULL, delim);
			pch = strtok(NULL, delim);
			pch = strtok(NULL, delim);
		};
	}

	/* save config */
	sprintf(cmd, "%s", "SAVE_CONFIG");
	ret = linux_command_to_supplicant(cmd, reply, sizeof(reply));
	if (ret) {
		WMG_ERROR("do save config error!\n");
		return WMG_STATUS_FAIL;
	}

	return ret;
}

static wmg_status_t linux_set_mac(const char *ifname, uint8_t *mac_addr)
{
	wmg_status_t ret;

	ret = linux_common_set_mac(ifname, mac_addr);
	if (ret) {
		WMG_ERROR("linux failed to set mac\n");
		return WMG_STATUS_FAIL;
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_get_mac(const char *ifname, uint8_t *mac_addr)
{
	wmg_status_t ret;

	ret = linux_common_get_mac(ifname, mac_addr);
	if (ret) {
		WMG_ERROR("linux failed to get mac\n");
		return WMG_STATUS_FAIL;
	}

	return WMG_STATUS_SUCCESS;
}

static wmg_status_t linux_platform_extension(int cmd, void* cmd_para,int *erro_code)
{
	switch (cmd) {
		case STA_CMD_CONNECT:
			return linux_supplicant_connect_to_ap((wifi_sta_cn_para_t *)cmd_para);
		case STA_CMD_DISCONNECT:
			return linux_supplicant_disconnect_to_ap();
		case STA_CMD_GET_INFO:
			return linux_supplicant_get_info((wifi_sta_info_t *)cmd_para);
		case STA_CMD_LIST_NETWORKS:
			return linux_supplicant_list_networks((wifi_sta_list_t *)cmd_para);
		case STA_CMD_REMOVE_NETWORKS:
			return linux_supplicant_remove_networks((char *)cmd_para);
		case STA_CMD_SET_AUTO_RECONN:
			return wpa_set_auto_reconn(*((wmg_bool_t *)cmd_para));
		case STA_CMD_GET_SCAN_RESULTS:
			return wpa_get_scan_results((get_scan_results_para_t *)cmd_para);
		case STA_CMD_SET_MAC:
			return linux_set_mac(((common_mac_para_t *)cmd_para)->ifname,
					((common_mac_para_t *)cmd_para)->mac_addr);
		case STA_CMD_GET_MAC:
			return linux_get_mac(((common_mac_para_t *)cmd_para)->ifname,
					((common_mac_para_t *)cmd_para)->mac_addr);
		default:
			return WMG_FALSE;
	}
}

static linux_sta_private_data_t linux_private_data = {
	.old_ssid = {0},
	.new_ssid = {0},
	.last_time = 0,
};

static wmg_sta_inf_object_t sta_inf_object = {
	.sta_init_flag = WMG_FALSE,
	.sta_enable_flag = WMG_FALSE,
	.sta_auto_reconn = WMG_FALSE,
	.sta_event_cb = NULL,
	.sta_event_handle = NULL,
	.sta_private_data = &linux_private_data,

	.sta_inf_init = linux_sta_inf_init,
	.sta_inf_deinit = linux_sta_inf_deinit,
	.sta_inf_enable = linux_sta_inf_enable,
	.sta_inf_disable = linux_sta_inf_disable,
	.sta_platform_extension = linux_platform_extension,
};

wmg_sta_inf_object_t* sta_linux_inf_object_register(void)
{
	return &sta_inf_object;
}
