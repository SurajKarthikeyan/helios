#ifndef SYSINFO_H
#define SYSINFO_H

typedef struct {
    float cpu_usage;
    long mem_total;
    long mem_used;
    float mem_percent;
    float cpu_temp;
} SysInfo;

void sysinfo_update(SysInfo *info);

#endif
