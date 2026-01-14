#include "cpu_dis.h"

u32 dis_cpu_simple_head(char *buf, struct config *config)
{
	u32 blen;

	switch (config->max_cpu) {
	case CPU_NR_C8:
		blen = snprintf(buf, 512,
			"--------------------------------------------------------"
			"%s CPU[x]<Freq:MHZ>-<Usage:%%>"
			"-------------------------------------------------------\n"
			" %9s"
			" %12s"
			" %12s"
			" %12s"
			" %12s"
			" %12s"
			" %12s"
			" %12s"
			" %8s"
			" %5s"
			" %5s"
			" %6s"
			"\n",
			config->platform_info,
			"CPU0", "CPU1", "CPU2", "CPU3",
			"CPU4", "CPU5", "CPU6", "CPU7", "Temp", "GPU", "DDR" ,"Times");
		break;
	case CPU_NR_C4:
		blen = snprintf(buf, 512,
				"-------------------------"
				"%s CPU[x]<Freq:MHZ>-<Usage:%%>"
				"-------------------------\n"
				" %9s"
				" %12s"
				" %12s"
				" %12s"
				" %8s"
				" %5s"
				" %6s"
				" %5s"
				" %7s"
				"\n",
				config->platform_info,
				"CPU0", "CPU1", "CPU2", "CPU3",
				"Temp", "GPU", "GTemp", "DDR", "Times");
		break;
	case CPU_NR_C2:
		blen = snprintf(buf, 512,
				"-------------------------"
				"%s CPU[x]<Freq:MHZ>-<Usage:%%>"
				"-------------------------\n"
				" %9s"
				" %12s"
				" %8s"
				" %5s"
				" %6s"
				" %5s"
				" %7s"
				"\n",
				config->platform_info,
				"CPU0", "CPU1",
				"Temp", "GPU", "GTemp", "DDR", "Times");
		break;

	default:
		printf("err: not support cpu which has %d cores\n", config->max_cpu);
		blen = -1;
		break;
	}
	return blen;
}

u32 dis_cpu_simple_content(char *buf, struct config *config, u32 blen, u32 monitor_cnt)
{
	switch (config->max_cpu) {
	case CPU_NR_C8:
		blen += snprintf((buf + blen), 512,
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  %4d "
				"  %3d "
				"  %3d "
				"  %3d \n",
				config->cpu_freq.cpufreq_list[0], config->cpu_load[0].busy,
				config->cpu_freq.cpufreq_list[1], config->cpu_load[1].busy,
				config->cpu_freq.cpufreq_list[2], config->cpu_load[2].busy,
				config->cpu_freq.cpufreq_list[3], config->cpu_load[3].busy,
				config->cpu_freq.cpufreq_list[4], config->cpu_load[4].busy,
				config->cpu_freq.cpufreq_list[5], config->cpu_load[5].busy,
				config->cpu_freq.cpufreq_list[6], config->cpu_load[6].busy,
				config->cpu_freq.cpufreq_list[7], config->cpu_load[7].busy,
				config->themp,config->gpu_dvfs, config->dram_dvfs,
				monitor_cnt);

		break;
	case CPU_NR_C4:
		blen += snprintf((buf + blen), 512,
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  %4d "
				"  %3d "
				"  %4d "
				"  %3d "
				"  %3d \n",
				config->cpu_freq.cpufreq_list[0], config->cpu_load[0].busy,
				config->cpu_freq.cpufreq_list[1], config->cpu_load[1].busy,
				config->cpu_freq.cpufreq_list[2], config->cpu_load[2].busy,
				config->cpu_freq.cpufreq_list[3], config->cpu_load[3].busy,
				config->themp,config->gpu_dvfs, config->gpu_temp, config->dram_dvfs,
				monitor_cnt);
		break;
	case CPU_NR_C2:
		blen += snprintf((buf + blen), 512,
				"  |%4d %3d%%|"
				"  |%4d %3d%%|"
				"  %4d "
				"  %3d "
				"  %4d "
				"  %3d "
				"  %3d \n",
				config->cpu_freq.cpufreq_list[0], config->cpu_load[0].busy,
				config->cpu_freq.cpufreq_list[1], config->cpu_load[1].busy,
				config->themp,config->gpu_dvfs, config->gpu_temp, config->dram_dvfs,
				monitor_cnt);
		break;
	default:
		printf("err: not support cpu which has %d cores\n", config->max_cpu);
		blen = -1;
		break;
	}
	return blen;
}
u32 dis_cpu_complex_head(char *buf, struct config *config)
{
	u32 blen;
	switch (config->max_cpu) {
	case CPU_NR_C8:
		blen = snprintf(buf, 512,
				"-------------------------------------------------------------"
				"%s CPU[x]-------------------------------------------------------------------\n"
				"-------------------------------------------------"
				"<Freq:MHZ> <Usage:%%>-<Iowait:%%>------------------------------------------------------\n"
				" %9s"
				" %15s"
				" %16s"
				" %15s"
				" %15s"
				" %15s"
				" %15s"
				" %15s"
				" %8s"
				" %5s"
				" %5s"
				"\n",
				config->platform_info,
				"CPU0", "CPU1", "CPU2", "CPU3",
				"CPU4", "CPU5", "CPU6", "CPU7", "Temp", "GPU", "DDR");

		break;
	case CPU_NR_C4:
		blen = snprintf(buf, 512,
				"-------------------------------"
				"%s CPU[x]----------------------------------\n"
				"------------------"
				"<Freq:MHZ> <Usage:%%>-<Iowait:%%>----------------------\n"
				" %9s"
				" %15s"
				" %16s"
				" %15s"
				" %8s"
				" %5s"
				" %5s"
				"\n",
				config->platform_info,
				"CPU0", "CPU1", "CPU2", "CPU3",
				"Temp", "GPU", "DDR");
		break;
	case CPU_NR_C2:
		blen = snprintf(buf, 512,
				"-------------------------------"
				"%s CPU[x]----------------------------------\n"
				"------------------"
				"<Freq:MHZ> <Usage:%%>-<Iowait:%%>----------------------\n"
				" %9s"
				" %15s"
				" %8s"
				" %5s"
				" %5s"
				"\n",
				config->platform_info,
				"CPU0", "CPU1",
				"Temp", "GPU", "DDR");
		break;
	default:
		printf("err: not support cpu which has %d cores\n", config->max_cpu);
		blen = -1;
		break;
	}
	return blen;
}
u32 dis_cpu_complex_content(char *buf, struct config *config, u32 blen)
{
	switch (config->max_cpu) {
	case CPU_NR_C8:
		blen += snprintf((buf + blen), 512,
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%3d %6d %5d\n",
				config->cpu_freq.cpufreq_list[0], config->cpu_load[0].busy, config->cpu_load[0].iowait,
				config->cpu_freq.cpufreq_list[1], config->cpu_load[1].busy, config->cpu_load[1].iowait,
				config->cpu_freq.cpufreq_list[2], config->cpu_load[2].busy, config->cpu_load[2].iowait,
				config->cpu_freq.cpufreq_list[3], config->cpu_load[3].busy, config->cpu_load[3].iowait,
				config->cpu_freq.cpufreq_list[4], config->cpu_load[4].busy, config->cpu_load[4].iowait,
				config->cpu_freq.cpufreq_list[5], config->cpu_load[5].busy, config->cpu_load[5].iowait,
				config->cpu_freq.cpufreq_list[6], config->cpu_load[6].busy, config->cpu_load[6].iowait,
				config->cpu_freq.cpufreq_list[7], config->cpu_load[7].busy, config->cpu_load[7].iowait,
				config->themp, config->gpu_dvfs, config->dram_dvfs);

		break;
	case CPU_NR_C4:
		blen += snprintf((buf + blen), 512,
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%3d %6d %5d \n",
				config->cpu_freq.cpufreq_list[0], config->cpu_load[0].busy, config->cpu_load[0].iowait,
				config->cpu_freq.cpufreq_list[1], config->cpu_load[1].busy, config->cpu_load[1].iowait,
				config->cpu_freq.cpufreq_list[2], config->cpu_load[2].busy, config->cpu_load[2].iowait,
				config->cpu_freq.cpufreq_list[3], config->cpu_load[3].busy, config->cpu_load[3].iowait,
				config->themp, config->gpu_dvfs, config->dram_dvfs);

		break;
	case CPU_NR_C2:
		blen += snprintf((buf + blen), 512,
				"%5d %3d%%%3d%%\t"
				"%5d %3d%%%3d%%\t"
				"%3d %6d %5d \n",
				config->cpu_freq.cpufreq_list[0], config->cpu_load[0].busy, config->cpu_load[0].iowait,
				config->cpu_freq.cpufreq_list[1], config->cpu_load[1].busy, config->cpu_load[1].iowait,
				config->themp, config->gpu_dvfs, config->dram_dvfs);

		break;
	default:
		printf("err: not support cpu which has %d cores\n", config->max_cpu);
		blen = -1;
		break;
	}
	return blen;
}

