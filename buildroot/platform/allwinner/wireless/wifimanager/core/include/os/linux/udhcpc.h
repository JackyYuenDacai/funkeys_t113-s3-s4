/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __UDHCPC_H__
#define __UDHCPC_H__

#if __cplusplus
extern "C" {
#endif

void start_udhcpc(char *inf);
int is_ip_exist(char *inf);

#if __cplusplus
}
#endif

#endif /* __UDHCPC_H__ */
