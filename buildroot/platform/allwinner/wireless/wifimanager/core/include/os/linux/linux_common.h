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

#ifndef _LINUX_COMMON_H
#define _LINUX_COMMON_H

#if __cplusplus
extern "C" {
#endif

#define WPAD_MODE_STA     0
#define WPAD_MODE_P2P     1
#define WPAD_MODE_AP      2

typedef int (* dispatch_event_t)(const char *event_str, int nread);

typedef struct {
	int mode_type;
	dispatch_event_t dispatch_event;
	void *linux_mode_private_data;
} init_wpad_para_t;

uint8_t char2uint8(char* trs);
wmg_status_t linux_common_set_mac(const char *ifname, uint8_t *);
wmg_status_t linux_common_get_mac(const char *ifname, uint8_t *);

wmg_status_t init_wpad(init_wpad_para_t wpad_para);
wmg_status_t deinit_wpad(int mode_type);
wmg_status_t command_to_wpad(char const *cmd, char *reply, size_t reply_len, int mode_type);

#if __cplusplus
};  // extern "C"
#endif

#endif  // _LINUX_COMMON_H
