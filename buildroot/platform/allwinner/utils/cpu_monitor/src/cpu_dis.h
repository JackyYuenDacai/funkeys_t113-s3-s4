#ifndef __CPU_DIS_INCLUDE_H_
#define __CPU_DIS_INCLUDE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cpu_lib.h"


u32 dis_cpu_simple_head(char *buf, struct config *config);
u32 dis_cpu_simple_content(char *buf, struct config *config, u32 blen, u32 monitor_cnt);
u32 dis_cpu_complex_head(char *buf, struct config *config);
u32 dis_cpu_complex_content(char *buf, struct config *config, u32 blen);

#endif
