/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LINUX_WPA_H
#define _LINUX_WPA_H

#if __cplusplus
extern "C" {
#endif

#include <wmg_sta.h>

wmg_sta_inf_object_t* sta_linux_inf_object_register(void);

#define CMD_LEN        255
#define REPLY_BUF_SIZE 4096 // wpa_supplicant's maximum size.

typedef struct {
	char old_ssid[SSID_MAX_LEN + 1];
	char new_ssid[SSID_MAX_LEN + 1];
	int last_time;
} linux_sta_private_data_t;

typedef struct {
	const char *ssid;
	wifi_secure_t key_mgmt;
	char *net_id;
	int *len;
} conf_is_ap_exist_para_t;

typedef struct {
	const char *ssid;
	wifi_secure_t key_mgmt;
	char *net_id;
	int *len;
} conf_ssid2netid_para_t;

typedef struct {
	char *ssid;
	int *len;
} conf_is_ap_connected_para_t;

typedef struct {
	char *net_id;
	int *len;
} conf_get_netid_connected_para_t;

typedef struct {
	char *net_id;
	int *len;
} conf_get_ap_connected_para_t;

typedef struct {
	char *ssid;
	wifi_secure_t key_mgmt;
} conf_remove_network_para_t;


#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
   (__extension__                                                              \
     ({ long int __result;                                                     \
        do __result = (long int) (expression);                                 \
        while (__result == -1L && errno == EINTR);                             \
        __result; }))
#endif

#if __cplusplus
};  // extern "C"
#endif

#endif  // _LINUX_WPA_H
