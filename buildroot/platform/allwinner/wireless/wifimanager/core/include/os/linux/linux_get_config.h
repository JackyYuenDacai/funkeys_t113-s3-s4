#ifndef __LINUX_GET_CONFIG_H__
#define __LINUX_GET_CONFIG_H__

#include "wmg_common.h"

#if __cplusplus
extern "C" {
#endif

int get_config(char *config_file_name, char *config_name, char *config_buf);

#if __cplusplus
}
#endif

#endif /*  __LINUX_GET_CONFIG_H__ */
