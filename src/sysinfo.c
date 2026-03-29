#include "sysinfo.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

static ULONGLONG prev_idle = 0;
static ULONGLONG prev_kernel = 0;
static ULONGLONG prev_user = 0;

static ULONGLONG filetime_to_ull(FILETIME ft) {
    return ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

void sysinfo_update(SysInfo *info) {
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        ULONGLONG u_idle   = filetime_to_ull(idle);
        ULONGLONG u_kernel = filetime_to_ull(kernel);
        ULONGLONG u_user   = filetime_to_ull(user);

        ULONGLONG diff_idle   = u_idle   - prev_idle;
        ULONGLONG diff_kernel = u_kernel - prev_kernel;
        ULONGLONG diff_user   = u_user   - prev_user;

        ULONGLONG total = diff_kernel + diff_user;
        if (total > 0)
            info->cpu_usage = 100.0f * (1.0f - (float)diff_idle / (float)total);
        else
            info->cpu_usage = 0.0f;

        prev_idle   = u_idle;
        prev_kernel = u_kernel;
        prev_user   = u_user;
    }

    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        info->mem_total   = (long)(mem.ullTotalPhys / 1024);
        info->mem_used    = (long)((mem.ullTotalPhys - mem.ullAvailPhys) / 1024);
        info->mem_percent = 100.0f * (float)info->mem_used / (float)info->mem_total;
    }

    info->cpu_temp = -1;
}

#else

static long prev_idle = 0;
static long prev_total = 0;

void sysinfo_update(SysInfo *info) {
    FILE *f = fopen("/proc/stat", "r");
    if (f) {
        long user, nice, system, idle, iowait, irq, softirq;
        fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq);
        fclose(f);

        long total = user + nice + system + idle + iowait + irq + softirq;
        long diff_idle  = idle  - prev_idle;
        long diff_total = total - prev_total;

        info->cpu_usage = 100.0f * (1.0f - (float)diff_idle / diff_total);
        prev_idle  = idle;
        prev_total = total;
    }

    f = fopen("/proc/meminfo", "r");
    if (f) {
        char label[32];
        long value;
        long mem_available = 0;
        while (fscanf(f, "%s %ld kB", label, &value) == 2) {
            if (strcmp(label, "MemTotal:")     == 0) info->mem_total = value;
            if (strcmp(label, "MemAvailable:") == 0) mem_available  = value;
        }
        fclose(f);
        info->mem_used    = info->mem_total - mem_available;
        info->mem_percent = 100.0f * (float)info->mem_used / info->mem_total;
    }

    f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (f) {
        int temp;
        fscanf(f, "%d", &temp);
        info->cpu_temp = temp / 1000.0f;
        fclose(f);
    } else {
        info->cpu_temp = -1;
    }
}
#endif