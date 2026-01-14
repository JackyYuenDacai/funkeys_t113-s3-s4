/*  cpufreq-test Tool
 *
 *  Copyright (C) 2014 sujiajia <sujiajia@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#include "cpu_lib.h"
#include "cpu_dis.h"

static u32 monitor_magic_switch = 0;
static u32 monitor_start = 0;
static u32 monitor_cnt = 0;

static struct option monitor_long_options[] = {
	{"output",        1,    0,    'o'},
	{"complex",         1,    0,    'c'},
	{"tempreture",    1,    0,    't'},
	{"simple",      1,  0,  's'},
	{"file",        1,    0,    'f'},
	{"memory",      1,    0,    'm'},
	{"kswap",      1,    0,    'k'},
	{"help",        0,    0,    'h'},
	{"vresion",     0,  0,  'v'},
	{0,             0,     0,      0}
};

const char* const monitor_short_options = "h:o:c:t:f:v:s:k:";

/*******************************************************************
  monitor_help_usage_show
 *******************************************************************/

void monitor_help_usage_show()
{
	printf(" show how to use: cpu_monitor\n"
			" Options:\n"
			" 1.{ Show All Cpus with (Freq + Usage + Iowait) + (Temperature) } pre N millisecond \n"
			"     cpu_monitor -c [N]\t\t\t An example: cpu_monitor -c 500 \n\n"
			" 2.{ Show All Cpus with (Freq + Usage ) + (Temperature) } pre N millisecond \n"
			"     cpu_monitor -s [N]\t\t\t An example: cpu_monitor -s 500 \n\n"
			" 3.{ Show All Cpus with (Freq + Usage) + (Temperature) } pre N millisecond \n"
			"     Store log in dir/cpu-monitor_HOST_KERNEL-VERSION_TIMESTAMP.log \n"
			"     cpu_monitor -o [dir] -f [N]\t An example: cpu_monitor -o /data -f 500\n\n"
			" 4.{ Show meminfo per N millisecond ,unit default '0' as MB, set '1' as KB\n"
			"     cpu_monitor -u [unit] -m [N]\t An example: cpu_monitor -u 1 -m 500\n\n"
			" 5.{ Trace Kernel Memory vm_event } per N millisecond\n"
			"     cpu_monitor -k \t An example: cpu_monitor -k 500\n\n"
			" 6. -h, --help\t\t\t\t show this help screen\n\n"
			" 7. -v, --cpu monitor version\n\n"
			" 8. if you find any bug, pls email to author sujiajia@allwinnertech.com\n");
	exit(1);
}

FILE *monitor_prepare_output_file(const char *dirname)
{
	FILE *output = NULL;
	DIR *dir;
	s32 len;
	char *filename = NULL;
	char tmbuf[64];
	struct utsname sysdata;
	struct timeval tv;
	struct tm *nowtm;
	time_t nowtime;

	dir = opendir(dirname);
	if (dir == NULL) {
		if (mkdir(dirname, 0755)) {
			perror("mkdir");
			fprintf(stderr, "error: Cannot create dir %s\n",
					dirname);
			return NULL;
		}
	}

	len = strlen(dirname) + 125;
	filename = malloc(sizeof(char) * len);
	if (filename == NULL)
		return NULL;

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof(tmbuf), "%F-%H-%M-%S", nowtm);

	if (uname(&sysdata) == 0) {
		len += strlen(sysdata.nodename) + strlen(sysdata.release);
		snprintf(filename, len - 1, "%s/cpu-monitor_%s_%s_%s.log",
				dirname, sysdata.nodename, sysdata.release, tmbuf);
	} else {
		snprintf(filename, len - 1, "%s/cpu-monitor_%s.log",
				dirname, tmbuf);
	}

	output = fopen(filename, "w+");
	if (output == NULL) {
		perror("fopen");
		fprintf(stderr, "error: unable to open logfile\n");
	}

	fprintf(stdout, "Logfile: %s\n", filename);

	free(filename);
	return output;
}

s32 monitor_system_platform(struct config *config)
{
	s32 ret;
	s32 i;
	char buf[32];
	char *plat_str[9] = { "sun50iw6p1", "Sun50iw1p1", "sun50iw2p1", "Sun8iw6p1", "Sun8iw7p1", "sun8iw7", "Sun9iw1p1", "sun8iw15",NULL };
	char *plat_info[9] = { "H6", "H64","H5", "H8", "H3", "H3", "A80","A50", NULL };
	char *plat_arch[3] = { "ARMv7", "AArch64", NULL};	
	enum ARCH_PLAT {
		H6_ARCH_64=0,
		H64_ARCH_64,
		H5_ARCH_64,
		H8_ARCH_32,
		H3_ARCH_32  = H8_ARCH_32,
		A80_ARCH_32 = H8_ARCH_32,
		A50_ARCH_32 = H8_ARCH_32,
	};
	if (!config)
		return -1;
	ret = read_sys_info_stat(buf);
	if (ret < 0)
		return -1;

	for(i=0; plat_str[i]!=NULL; i++)
	{
		if (!strncmp(buf, plat_str[i], strlen(plat_str[i]))) {
			strncpy(config->platform_info, plat_info[i], strlen(plat_info[i]));
			/******
			H64 arch64  <=4GB  	 mem_type '0': mean platform just has ZONE DMA
			H3/H8 arch32 512MB   mem_type '1': mean platform just has ZONE NORMAL
			H3/H8 arch32 >=1GMB  mem_type '2': mean platform just has ZONE NORMAL and ZONE HIGH
			*******/
			if ((i == H64_ARCH_64) || (i == H5_ARCH_64) || (i == H6_ARCH_64))
				config->mem_type = 0;
			else
				config->mem_type = 1;
			break;
		}
	}

	memset(buf, 0, sizeof(buf));
	ret = read_cpu_info_stat(buf);
	if (ret < 0)
		return -1;

	for(i=0; plat_arch[i]!=NULL; i++)
	{
		if (!strncmp(buf, plat_arch[i], strlen(plat_arch[i]))) {
			strncpy(config->platform_arch, plat_arch[i], strlen(plat_arch[i]));
			break;
		}
	}
	return 0;
}


s32 monitor_current_max_temperature(struct config *cfg)
{
	s32 themp_value = 0;
	if (!cfg)
		return -1;
	themp_value    = read_proc_thermal_stat(0);
	if (themp_value < 0)
		return -1;
	cfg->themp = themp_value;
	return 0;
}


s32 monitor_current_dram_dvfs(struct config *cfg)
{
	s32 dram_dvfs = 0;
	if (!cfg)
		return -1;
	dram_dvfs = read_proc_dram_dvfs();
	if (dram_dvfs < 0)
		dram_dvfs = 0;
	else
		cfg->dram_dvfs = dram_dvfs/1000; //Mhz
	return 0;
}

s32 monitor_current_ion_vmstat(struct config *cfg)
{
	s32 ret = 0;
	if (!cfg)
		return -1;

	ret = read_sys_ion_vmstat(&cfg->ions);
	if (ret <0)
		return -1;
	return 0;
}


s32 monitor_current_gpu_dvfs(struct config *cfg)
{
	s32 gpu_dvfs = 0;
	if (!cfg)
		return -1;
	gpu_dvfs = read_proc_gpu_dvfs();
	if (gpu_dvfs < 0)
		gpu_dvfs = 0;
	else
		cfg->gpu_dvfs = gpu_dvfs/1000000;//Mhz
	return 0;
}

s32 monitor_current_gpu_temperature(struct config *cfg)
{
	s32 themp_value = 0;
	if (!cfg)
		return -1;
	themp_value    = read_proc_thermal_stat(1);
	if (themp_value < 0)
		cfg->gpu_temp = 0;
	cfg->gpu_temp = themp_value;
	return 0;
}

s32 monitor_current_gpu_vmstat(struct config *cfg)
{
	int i = 0;
	int ret = 0;
	int gpu_vmstat_extern_flag[4] = {0,1,1,0};
	char *plat_info[4] = { "H3", "H5", "H6", NULL};
	char *gpu_vmstat[4] = {
		                   "/sys/kernel/debug/mali/memory_usage",
                           "/sys/kernel/debug/mali/gpu_memory",
						   "/sys/kernel/debug/mali0/gpu_memory",
						   NULL
						   };
    						   
	char *gpu_vmstat_extern_format[4] =  {
							NULL,
						    "Mali mem usage", 
						    "mali0",
							NULL,
						    };
							
	char *gpu_vmstat_scan_format[4] =  {
							NULL,
						    "%*s %*s %*s %d", 
						    "%*s %d",
							NULL,
						    };										
	int gpu_vmstat_unit[4] =  {
							1,
						    1, 
						    4*1024,
							0,
						    };	
	for(i=0; plat_info[i]!=NULL; i++) {
		if (!strncmp(cfg->platform_info, plat_info[i], strlen(plat_info[i]))) {
			ret = read_gpu_vmstat(gpu_vmstat[i], gpu_vmstat_extern_flag[i], 
					gpu_vmstat_extern_format[i], gpu_vmstat_scan_format[i]);
			if (ret > 0)
				cfg->gpu_memusg = ret*gpu_vmstat_unit[i];
			else
				cfg->gpu_memusg = 0;
			break;
		}
	}

	return cfg->gpu_memusg;
}


s32 monitor_percpu_freq_calculate(struct config *cfg)
{
	u32 i = 0;
	u32 cpu_max = CPU_NRS_MAX;
	s32 is_online;
	u32 pcpu_freq;

	if(!cfg)
		return -1;

	for (i=0; i<CPU_NRS_MAX; i++)
	{
		/*
		 * cpu0 have not online for linux4.4
		 */
		if (i)
			is_online = find_cpu_online(i);
		else
			is_online = CPU_IS_ONLINE;
		switch(is_online) {
			case CPU_IS_ONLINE:
				pcpu_freq = 0;
				pcpu_freq = get_cpufreq_by_cpunr(i);
				if (pcpu_freq != 0) {
					cfg->cpu_freq.cpu_online[i] = 1;
					cfg->cpu_freq.cpufreq_list[i] = pcpu_freq/1000;
				}
				break;
			case CPU_IS_OFFLINE:
				cfg->cpu_freq.cpu_online[i] = 0;
				cfg->cpu_freq.cpufreq_list[i] = 0;
				break;
			case CPU_IS_INVALID:
				cpu_max--;
			case CPU_IS_FAULT:
				cfg->cpu_freq.cpu_online[i] = 0;
				cfg->cpu_freq.cpufreq_list[i] = 0;
				break;
		}
	}
	cfg->max_cpu = cpu_max;
	return 0;
}

s32 monitor_percpu_usage_calculate(struct config *cfg)
{
	struct cpu_usage cur;
	s32 ret =0;
	s32 i = 0;
	float usage = 0;
	float total, busy, idle, iowait = 0;

	if (!cfg)
		return -1;
	memset(&cur, 0, sizeof(cur));
	ret = read_proc_precpu_stat(&cur);
	if (ret <0)
		return -1;

	if (cfg->pre_usages.active == 0) {
		memcpy(&cfg->pre_usages, &cur, sizeof(cur));
		memset(&cur, 0, sizeof(cur));
		usleep(300000);
		if (read_proc_precpu_stat(&cur)<0)
			return -1;
	}

	for(i=0; i<(CPU_NRS_MAX+1); i++)
	{
		if (cur.active&(0x01<<i)) {
			if (cfg->pre_usages.active&(0x01<<i)) {
				total = cur.stats[i].totalcpuTime-
					cfg->pre_usages.stats[i].totalcpuTime;
				busy = cur.stats[i].totalbusyTime -
					cfg->pre_usages.stats[i].totalbusyTime;
				idle = cur.stats[i].itime -
					cfg->pre_usages.stats[i].itime;
				iowait =  cur.stats[i].iowtime -
					cfg->pre_usages.stats[i].iowtime;

				if ((idle > total) || (total <= 0)){
					cfg->cpu_load[i].iowait = iowait;
					cfg->cpu_load[i].idle = idle;
					cfg->cpu_load[i].busy = busy;
				} else {
					usage = (float)busy/total*100;
					cfg->cpu_load[i].busy = (u32)usage;
					usage = (float)idle/total*100;
					cfg->cpu_load[i].idle = (u32)usage;
					usage = (float)iowait/total*100;
					cfg->cpu_load[i].iowait = (u32)usage;
				}
			}
		} else {
			cfg->cpu_load[i].iowait = 0;
			cfg->cpu_load[i].idle = 0;
			cfg->cpu_load[i].busy = 0;
		}
	}

	memcpy(&cfg->pre_usages, &cur, sizeof(cur));
	return 0;
}


s32 monitor_precpu_simple_show(struct config *config)
{
	char buf[512] = {0};
	u32 blen = 0;
	s32 ret = 0;

	if (!config)
		return -1;

	ret = monitor_percpu_usage_calculate(config);
	if (ret < 0) {
		printf("err: monitor_percpu_usage_calculate exit\n");
		//return -1;
	}

	ret = monitor_current_max_temperature(config);
	if (ret < 0) {
		printf("err: monitor_current_max_temperature exit\n");
		//return -1;
	}

	ret = monitor_percpu_freq_calculate(config);
	if (ret < 0) {
		printf("err: monitor_percpu_freq_calculate exit\n");
		//return -1;
	}

	ret = monitor_current_dram_dvfs(config);
	if (ret < 0) {
		printf("err: monitor_current_dram_dvfs exit\n");
		//return -1;
	}

	ret = monitor_current_gpu_dvfs(config);
	if (ret < 0) {
		printf("err: monitor_current_gpu_dvfs exit\n");
		//return -1;
	}
	ret = monitor_current_gpu_temperature(config);
	if (ret < 0) {
		printf("err: monitor_current_gpu_temp exit\n");
		//return -1;
	}

	if ((monitor_magic_switch&0xf) == 0) {
		blen = dis_cpu_simple_head(buf, config);
		if (blen < 0)
			return blen;
		monitor_magic_switch = 0;
	}
	blen = dis_cpu_simple_content(buf, config, blen, monitor_cnt);
	if (blen < 0)
		return blen;

	printf("%s",buf);
	monitor_cnt++;
	if (monitor_cnt > 0xffffff00)
		monitor_cnt = 0;
	monitor_magic_switch ++;

	return 0;
}


s32 monitor_precpu_complex_show(struct config *config)
{
	char buf[1024] = {0};
	u32 blen = 0;
	s32 ret = 0;

	if (!config)
		return -1;
	ret = monitor_percpu_usage_calculate(config);
	if (ret < 0)
		return -1;

	ret = monitor_current_max_temperature(config);
	if (ret < 0)
		return -1;

	ret = monitor_percpu_freq_calculate(config);
	if (ret < 0)
		return -1;

	ret = monitor_current_dram_dvfs(config);
	if (ret < 0)
		return -1;

	ret = monitor_current_gpu_dvfs(config);
	if (ret < 0)
		return -1;

	if ((monitor_magic_switch&0x7) == 0) {
		blen = dis_cpu_complex_head(buf, config);
		if (blen < 0)
			return blen;
		monitor_magic_switch = 0;
	}
	blen = dis_cpu_complex_content(buf, config, blen);
	if (blen < 0)
		return blen;

	printf("%s",buf);
	monitor_magic_switch ++;

	return 0;
}

s32 monitor_precpu_complex_store(struct config *config)
{
	char buf[1024] = {0};
	u32 blen = 0;
	s32 ret = 0;

	if (!config)
		return -1;

	ret = monitor_percpu_usage_calculate(config);
	if (ret < 0)
		return -1;

	ret = monitor_current_max_temperature(config);
	if (ret < 0)
		return -1;

	ret = monitor_percpu_freq_calculate(config);
	if (ret < 0)
		return -1;

	ret = monitor_current_dram_dvfs(config);
	if (ret < 0)
		return -1;

	ret = monitor_current_gpu_dvfs(config);
	if (ret < 0)
		return -1;

	if (monitor_magic_switch == 0) {
		blen = dis_cpu_complex_head(buf, config);
		if (blen < 0)
			return blen;
		monitor_magic_switch++;
	}
	blen = dis_cpu_complex_content(buf, config, blen);
	if (blen < 0)
		return blen;

	printf("%s", buf);
	if (config->output != NULL) {
		fseek(config->output, 0L, SEEK_END);
		fwrite(buf, 1, blen, config->output);
	}

	return 0;
}


s32 monitor_percpu_freq_show(struct cpufreq_info *cfs)
{
	struct timeval now;
	gettimeofday(&now,NULL);
	if (monitor_magic_switch == 0) {
		cfs->blen += snprintf(cfs->buf, 256,
				"%11s %7s %8s %14s %16s %20s\n",
				"Time","CPU","Cur-F","<Min-Max-F>",
				"Scaling-Cur-F","<Scaling-Min-Max-F>");
		monitor_magic_switch = 1;
	}
	cfs->blen += snprintf((cfs->buf + cfs->blen), 256,
			"%7.3f %3d  %8d %7d-%4d %12d %15d-%4d \n",
			now.tv_sec+now.tv_usec*1e-6,
			cfs->online_cpu,
			cfs->cur_freq/1000,
			cfs->min_freq/1000,
			cfs->max_freq/1000,
			cfs->scaling_cur_freq/1000,
			cfs->scaling_min_freq/1000,
			cfs->scaling_max_freq/1000);
	printf("%s",cfs->buf);

	return 0;
}


s32 monitor_presensor_tempe(struct config *config)
{
	s32 ret = 0;
	char strout[512] = {0};
	u32 strlen = 0;

	if (!config)
		return -1;

	ret = read_sysfs_tempe_state(config->thempre);
	if (ret <0)
		return -1;
	if ((monitor_magic_switch&0x1f) == 0) {
		strlen += snprintf(strout, 512,
				//" %13s"
				" %12s"
				" %12s"
				" %12s"
				" %12s"
				" %12s"
				" %12s"
				"\n",
				//"Time",
				"A15 Temp", "A7 Temp",
				"Dram Temp", "GPU Temp",
				"avg Temp", "max Temp");
		monitor_magic_switch = 0;
	}

	//gettimeofday(&now,NULL);
	strlen += snprintf((strout + strlen), 512,
			//" %15.2f"
			" %7d"
			" %12d"
			" %9d"
			" %13d"
			" %13d"
			" %12d\n",
			//now.tv_sec+now.tv_usec*1e-6,
			config->thempre[0],
			config->thempre[3],
			config->thempre[1],
			config->thempre[2],
			config->thempre[5],
			config->thempre[4]);

	printf("%s",strout);
	monitor_magic_switch ++;

	return 0;
}

s32 monitor_system_meminfo(struct config *config)
{
	static s32 pre_vm[VMSTATE_MAX];
	struct vmstat_value vmstat;
	char strout[1024] = {0};
	u32 strlen = 0;
	s32 ret = 0;
	struct timeval tv;

	if (!config)
		return -1;

	gettimeofday(&tv, NULL);
	monitor_system_platform(config);
	memset(&vmstat, 0 , sizeof(vmstat));

	monitor_current_gpu_vmstat(config);

	monitor_current_ion_vmstat(config);
	ret = read_proc_vmstat(&vmstat);
	if (ret < 0) {
		ERROR("aceess vmstat fail!\n");
	}

	ret = read_proc_meminfo(&vmstat);
	if (ret < 0) {
		ERROR("aceess meminfo fail!\n");
	}
	if (monitor_start == 0) {
		memcpy((void *)pre_vm, (void *)vmstat.vm_value, sizeof(pre_vm));
		monitor_start = 1;
		return 0;
	}

	if ((monitor_magic_switch&0xf) == 0) {
		s32 total_used = vmstat.mm_value[SWAPTOTAL] - vmstat.mm_value[SWAPFREE] +
						 vmstat.mm_value[BUFFER] + vmstat.vm_value[NR_KERNEL_STACK]*4 +
						 vmstat.vm_value[NR_SHMEM]*4 + (config->gpu_memusg>>10) +
						 (config->ions.cma>>10) + (config->ions.sytem>>10) +
						 vmstat.vm_value[NR_ANON_PAGES]*4 + vmstat.vm_value[NR_FILE_PAGES]*4 +
						 (vmstat.vm_value[NR_SLAB_RECLAIMABLE] + vmstat.vm_value[NR_SLAB_UNRECLAIMABLE])*4;
		s32 total_freed = vmstat.vm_value[NR_FREE_PAGES]*4;
		s32 total_lost  = vmstat.mm_value[Total] - total_used - total_freed;
							
		strlen += snprintf(strout, 1024,
				" ----------------------------------------%s %s--Mem State {unit:%s}-----------------------------------------------\n"
				" %7s %6d  "
				" %7s %6d  "
				" %7s %6d  "
				" %7s %6d  "
				" %7s %6d  "
				" \n"
				" %7s %4d  "
				" %7s %4d  "
				" %7s %4d  "
				" %7s %4d  "
				" \n"
				" %7s %2d  "
				" %7s %4d  "
				" %7s %4d  "
				" %7s %4d  "
				" \n"
				" %7s%4d  "
				" %7s%4d  "
				" %7s%4d  "
				" \n",
				config->platform_info,
				config->platform_arch,
				(config->unit==0)?"MB":"KB",
				"Memory-Total:",
				(config->unit==1)?vmstat.mm_value[Total]:vmstat.mm_value[Total]/1024,
				"Swap-Total:",
				(config->unit==1)?vmstat.mm_value[SWAPTOTAL]:vmstat.mm_value[SWAPTOTAL]/1024,
				"Swap-Free:",
				(config->unit==1)?vmstat.mm_value[SWAPFREE]:(vmstat.mm_value[SWAPFREE])/1024,
				"Vma-Total:",
				(config->unit==1)?vmstat.mm_value[VMTOTAL]:vmstat.mm_value[VMTOTAL]/1024,
				"Vma-Free:",
				(config->unit==1)?(vmstat.mm_value[VMTOTAL] - vmstat.mm_value[VMUESD]):
					((vmstat.mm_value[VMTOTAL] - vmstat.mm_value[VMUESD])/1024),
				"Buffer:",
				(config->unit==1)?vmstat.mm_value[BUFFER]:vmstat.mm_value[BUFFER]/1024,
				"KernStack:",
				(config->unit==1)?vmstat.vm_value[NR_KERNEL_STACK]*4:(vmstat.vm_value[NR_KERNEL_STACK])*4/1024,
			    "Shmem:",
				(config->unit==1)?vmstat.vm_value[NR_SHMEM]*4:(vmstat.vm_value[NR_SHMEM])*4/1024,
				"Gpu:",
				(config->unit==1)?config->gpu_memusg>>10:config->gpu_memusg>>20,
				"Ion cma: ", 
				(config->unit==1)?config->ions.cma>>10:config->ions.cma>>20,
				"caverout: ",
				(config->unit==1)?config->ions.carveout>>10:config->ions.carveout>>20,
				"sytem: ",
				(config->unit==1)?config->ions.sytem>>10:config->ions.sytem>>20,
				"contig: ",
				(config->unit==1)?config->ions.contig>>10:config->ions.contig>>20,
				"Total-used: ",
				(config->unit==1)?total_used:total_used>>10,
				"Total-freed: ",
				(config->unit==1)?total_freed:total_freed>>10,
				"Total-lost: ",
				(config->unit==1)?total_lost:total_lost>>10);

		strlen += snprintf((strout + strlen), 1024,
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"%7s| "
				"%10s"
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"  %s"
				"\n",
				"Anon",
				"Slab",
				"Cache",
				"Sysfre",
				"Cmafre",
				(config->mem_type==0)?"pgal{dm}":"pgal{nor}",
				"pgfree",
				"pgfalt",
				"fimap",
				"kswap",
				"dirty",
				"wback",
				"rbio",
				"wbio",
				"timestamp");
		monitor_magic_switch = 0;
	}
	if (config->unit == 1) {
		strlen += snprintf((strout + strlen), 1024,
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d| "
				"%10d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"  %d"
				"\n",
				vmstat.vm_value[NR_ANON_PAGES]*4,
				(vmstat.vm_value[NR_SLAB_RECLAIMABLE] + vmstat.vm_value[NR_SLAB_UNRECLAIMABLE])*4,
				vmstat.vm_value[NR_FILE_PAGES]*4,
				(vmstat.vm_value[NR_FREE_PAGES] - vmstat.vm_value[NR_FREE_CMA])*4,
				vmstat.vm_value[NR_FREE_CMA]*4,
				((config->mem_type==0)?(vmstat.vm_value[PGALLOC_DMA]-pre_vm[PGALLOC_DMA]):
				(vmstat.vm_value[PGALLOC_NORMAL]-pre_vm[PGALLOC_NORMAL])),
				vmstat.vm_value[PGFREE]-pre_vm[PGFREE],
				vmstat.vm_value[PGFAULT]-pre_vm[PGFAULT],
				vmstat.vm_value[PGMAJFAULT]-pre_vm[PGMAJFAULT],
				vmstat.vm_value[PAGEOUTRUN]-pre_vm[PAGEOUTRUN],
				vmstat.vm_value[NR_DIRTIED] -pre_vm[NR_DIRTIED],
				vmstat.vm_value[NR_WRITTEN] -pre_vm[NR_WRITTEN],
				vmstat.vm_value[PGPGIN]-pre_vm[PGPGIN],
				vmstat.vm_value[PGPGOUT]-pre_vm[PGPGOUT],
				tv.tv_sec);
	} else {
		strlen += snprintf((strout + strlen), 1024,
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d| "
				"%10d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"  %d"
				"\n",
				(vmstat.vm_value[NR_ANON_PAGES])*4/1024,
				((vmstat.vm_value[NR_SLAB_RECLAIMABLE] + vmstat.vm_value[NR_SLAB_UNRECLAIMABLE]))*4/1024,
				(vmstat.vm_value[NR_FILE_PAGES])*4/1024,
				((vmstat.vm_value[NR_FREE_PAGES] - vmstat.vm_value[NR_FREE_CMA]))*4/1024,
				(vmstat.vm_value[NR_FREE_CMA])*4/1024,
				((config->mem_type==0)?(vmstat.vm_value[PGALLOC_DMA]-pre_vm[PGALLOC_DMA]):
				(vmstat.vm_value[PGALLOC_NORMAL]-pre_vm[PGALLOC_NORMAL])),
				vmstat.vm_value[PGFREE]-pre_vm[PGFREE],
				vmstat.vm_value[PGFAULT]-pre_vm[PGFAULT],
				vmstat.vm_value[PGMAJFAULT]-pre_vm[PGMAJFAULT],
				vmstat.vm_value[PAGEOUTRUN]-pre_vm[PAGEOUTRUN],
				vmstat.vm_value[NR_DIRTIED] -pre_vm[NR_DIRTIED],
				vmstat.vm_value[NR_WRITTEN] -pre_vm[NR_WRITTEN],
				vmstat.vm_value[PGPGIN]-pre_vm[PGPGIN],
				vmstat.vm_value[PGPGOUT]-pre_vm[PGPGOUT],
				tv.tv_sec);
	}
	monitor_magic_switch++;

	printf("%s",strout);
	memcpy((void *)pre_vm, (void *)vmstat.vm_value, sizeof(pre_vm));	
	return 0;
fail:
	return -1;
}

s32 monitor_system_kswap(struct config *config)
{
	static s32 pre_vm_value[VMSTATE_MAX];
	struct vmstat_value vmstat;
	char strout[1024] = {0};
	u32 strlen = 0;
	s32 ret = 0;
	struct timeval tv;

	if (!config)
		return -1;

	gettimeofday(&tv, NULL);
	monitor_system_platform(config);
	memset(&vmstat, 0 , sizeof(vmstat));
	ret = read_proc_vmstat(&vmstat);
	if (ret < 0) {
		ERROR("aceess vmstat fail!\n");
	}

	ret = read_proc_meminfo(&vmstat);
	if (ret < 0) {
		ERROR("aceess meminfo fail!\n");
	}
	if (monitor_start == 0) {
		memcpy((void *)pre_vm_value, (void *)vmstat.vm_value, sizeof(pre_vm_value));
		monitor_start = 1;
		return 0;
	}

	if ((monitor_magic_switch&0xf) == 0) {
		strlen += snprintf(strout, 512,
				" -----------------------------------------------------%s %s Zone %s memory event-------------------------------------------------------\n"
				" ----Global-Pg----|"
				" ----------Global-io----------|"
				" ------Kswap------|"
				" --Direct Reclaim--|"
				" -----Other scan------|"
				" -----Cma----|"
				" ----swap----|"
				"-----\n",
				config->platform_info,
				config->platform_arch,
				(config->mem_type==0)?"Dma":"Normal");

		strlen += snprintf((strout + strlen), 512,
				"%6s"
				"%6s"
				"%6s"//
				"%7s"
				"%6s"
				"%6s"
				"%6s"
				"%6s"//		
				"%6s"
				"%6s"
				"%7s"//	
				"%6s"
				"%6s"
				"%7s"//
				"%10s"
				"%7s"
				"%7s"//
				"%7s"
				"%7s"
				"%7s"
				"%7s"
				"%6s"
				"%s\n",
				"alloc",
				"free",
				"fault",//
				"fimap",
				"rbio",
				"wbio",
				"dirty",
				"wback",//
				"run",
				"scan",
				"reclai",//
				"run",
				"scan",
				"reclai",//	
				"pgrefill",
				"slabs",
				"wback",//
				"alloc",
				"free",
				"swpin",
				"swpout",
				"time",
				"  timestamp");
		monitor_magic_switch = 0;
	}

	strlen += snprintf((strout + strlen), 512,
				"%6d"
				"%6d"
				"%6d"//
				"%7d"
				"%6d"
				"%6d"
				"%6d"
				"%6d"//
				"%6d"
				"%6d"
				"%7d"//
				"%6d"
				"%6d"
				"%7d"//
				"%10d"
				"%7d"
				"%7d"//
				"%7d"
				"%7d"
				"%7d"
				"%7d"
				"%6d"
				"  %d\n",
				((vmstat.vm_value[PGALLOC_DMA])?(vmstat.vm_value[PGALLOC_DMA]-pre_vm_value[PGALLOC_DMA]):
				(vmstat.vm_value[PGALLOC_NORMAL]-pre_vm_value[PGALLOC_NORMAL])),
				vmstat.vm_value[PGFREE]-pre_vm_value[PGFREE],
				vmstat.vm_value[PGFAULT]-pre_vm_value[PGFAULT],//
				vmstat.vm_value[PGMAJFAULT]-pre_vm_value[PGMAJFAULT],
				vmstat.vm_value[PGPGIN]-pre_vm_value[PGPGIN],
				vmstat.vm_value[PGPGOUT]-pre_vm_value[PGPGOUT],
				vmstat.vm_value[NR_DIRTIED]-pre_vm_value[NR_DIRTIED],
				vmstat.vm_value[NR_WRITTEN]-pre_vm_value[NR_WRITTEN],//
				vmstat.vm_value[PAGEOUTRUN]-pre_vm_value[PAGEOUTRUN],
				((config->mem_type==0)?(vmstat.vm_value[PGSCAN_KSWAPD_DMA]-pre_vm_value[PGSCAN_KSWAPD_DMA]):
				(vmstat.vm_value[PGSCAN_KSWAPD_NORMAL]-pre_vm_value[PGSCAN_KSWAPD_NORMAL])),
				//vmstat.vm_value[PGSCAN_KSWAPD_NORMAL]-pre_vm_value[PGSCAN_KSWAPD_NORMAL],
				((config->mem_type==0)?(vmstat.vm_value[PGSTEAL_KSWAPD_DMA]-pre_vm_value[PGSTEAL_KSWAPD_DMA]):
				(vmstat.vm_value[PGSTEAL_KSWAPD_NORMAL]-pre_vm_value[PGSTEAL_KSWAPD_NORMAL])),//
				//vmstat.vm_value[PGSTEAL_KSWAPD_NORMAL]-pre_vm_value[PGSTEAL_KSWAPD_NORMAL],
				vmstat.vm_value[ALLOCSTALL]-pre_vm_value[ALLOCSTALL],
				((config->mem_type==0)?(vmstat.vm_value[PGSCAN_DIRECT_DMA]-pre_vm_value[PGSCAN_DIRECT_DMA]):
				(vmstat.vm_value[PGSCAN_DIRECT_NORMAL]-pre_vm_value[PGSCAN_DIRECT_NORMAL])),//
				//vmstat.vm_value[PGSCAN_DIRECT_NORMAL]-pre_vm_value[PGSCAN_DIRECT_NORMAL],
				((config->mem_type==0)?(vmstat.vm_value[PGSTEAL_DIRECT_DMA]-pre_vm_value[PGSTEAL_DIRECT_DMA]):
				(vmstat.vm_value[PGSTEAL_DIRECT_NORMAL]-pre_vm_value[PGSTEAL_DIRECT_NORMAL])),
				//vmstat.vm_value[PGSTEAL_DIRECT_NORMAL]-pre_vm_value[PGSTEAL_DIRECT_NORMAL],
				((config->mem_type==0)?(vmstat.vm_value[PGREFILL_DMA]-pre_vm_value[PGREFILL_DMA]):
				(vmstat.vm_value[PGREFILL_NORMAL]-pre_vm_value[PGREFILL_NORMAL])),				
				//vmstat.vm_value[PGREFILL_NORMAL]-pre_vm_value[PGREFILL_NORMAL],
				vmstat.vm_value[SLABS_SCANNED]-pre_vm_value[SLABS_SCANNED],
				vmstat.vm_value[NR_VMSCAN_WRITE]-pre_vm_value[NR_VMSCAN_WRITE],//	
				vmstat.vm_value[PGCMAIN]-pre_vm_value[PGCMAIN],
				vmstat.vm_value[PGCMAOUT]-pre_vm_value[PGCMAOUT],
				vmstat.vm_value[PSWPIN]-pre_vm_value[PSWPIN],
				vmstat.vm_value[PSWPOUT]-pre_vm_value[PSWPOUT],
				monitor_cnt++,
				tv.tv_sec);
	monitor_magic_switch++;
	printf("%s",strout);
	memcpy((void *)pre_vm_value, (void *)vmstat.vm_value, sizeof(pre_vm_value));
	if (monitor_cnt > 0xffffff00)
		monitor_cnt = 0;

	return 0;
fail:
	return -1;
}

struct config *monitor_init(void)
{
	struct config *config = malloc(sizeof(struct config));
	if (!config) {
		return NULL;
	}
	memset(config, 0, sizeof(struct config));
	return config;
}

s32 monitor_release(struct config *cfg)
{
	if (cfg){
		if (cfg->output)
			fclose(cfg->output);
		free(cfg);
	}
	return 0;
}


/*******************************************************************
  main
 *******************************************************************/

s32 main(s32 argc, char **argv)
{
	s32 c;
	s32 ret;
	s32 option_index = 0;
	struct config *config = NULL;
	u32 i = 0;
	u32 unit = 0;
	u32 mtimes = 0;
	config = monitor_init();
	monitor_system_platform(config);
	while (1) {
		c = getopt_long(argc, argv, "h:o:c:t:f:v:s:p:m:u:k:d:",
				monitor_long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 'o':
				if (config->output != NULL)
					fclose(config->output);
				config->output = monitor_prepare_output_file(optarg);
				if (config->output == NULL) {
					printf("Cannot Create The %s/*.log File, Need Root Permissions!\n",optarg);
					printf("you may try adb remount!\n");
					return TEST_EXIT_FAILURE;
				}
				break;

			case 't':
				sscanf(optarg, "%u", &i);
				printf("user load time -> %s\n", optarg);
				mtimes = i >0?i*1000:500000;
				do {
					ret = monitor_presensor_tempe(config);
					usleep(mtimes);
				}while(ret==0);
				break;

			case 'c':
				sscanf(optarg, "%u", &i);
				mtimes = i >0?i*1000:500000;
				do {
					ret = monitor_precpu_complex_show(config);
					usleep(mtimes);
				}while(ret==0);
				break;

			case 's':
				sscanf(optarg, "%u", &i);
				mtimes = i >0?i*1000:500000;
				do {
					ret = monitor_precpu_simple_show(config);
					usleep(mtimes);
				}while(ret==0);
				break;

			case 'f':
				sscanf(optarg, "%u", &i);
				mtimes = i >0?i*1000:500000;
				do {
					ret = monitor_precpu_complex_store(config);
					usleep(mtimes);
				}while(ret==0);
				break;

			case 'm':
				sscanf(optarg, "%u", &i);
				mtimes = i >0?i*1000:500000;
				do {
					ret = monitor_system_meminfo(config);
					usleep(mtimes);
				}while(ret==0);
				break;

			case 'k':
				sscanf(optarg, "%u", &i);
				mtimes = i >0?i*1000:500000;
				do {
					ret = monitor_system_kswap(config);
					usleep(mtimes);
				}while(ret==0);
				break;

			case 'u':
				sscanf(optarg, "%u", &unit);
				if (unit)
					config->unit = 1;
				else
					config->unit = 0;
				break;
			case 'p':
				sscanf(optarg, "%u", &i);
				break;
			case 'v':
				//sscanf(optarg, "%u", &i);
				printf("Cpu Monitor Version 1.3.0\n");
				printf("Cpu monitor git update time:2018-05-28\n");
				printf("Author:sujiajia@allwinnertech.com\n");
				goto out;
			case 'd':
				sscanf(optarg, "%u", &i);
				debug_level = i;
				printf("Cpu Monitor Debug level: %d\n", debug_level);
				continue;
			case 'h':
			case '?':
			default:
				monitor_help_usage_show();
				goto out;
		}
	}

	do {
		mtimes = 500000;
		ret = monitor_precpu_simple_show(config);
		usleep(mtimes);
	}while(ret==0);

out:
	monitor_release(config);
	exit(0);
}

