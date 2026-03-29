#include "alert.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

void alert_init(AlertLog *log) {
    log->count = 0;
    log->head  = 0;
}

void alert_push(AlertLog *log, AlertLevel level, const char *msg) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    Alert *a = &log->entries[log->head];
    snprintf(a->timestamp, sizeof(a->timestamp), "%02d:%02d:%02d",
             t->tm_hour, t->tm_min, t->tm_sec);
    a->level = level;
    strncpy(a->message, msg, ALERT_MSG_LEN - 1);
    a->message[ALERT_MSG_LEN - 1] = '\0';

    log->head = (log->head + 1) % MAX_ALERTS;
    if (log->count < MAX_ALERTS) log->count++;
}

void alert_check_sysinfo(AlertLog *log, float cpu, float mem_percent) {
    static float prev_cpu = 0;
    static float prev_mem = 0;
    static int cpu_warned = 0;
    static int mem_warned = 0;

    if (cpu > 80.0f && !cpu_warned) {
        alert_push(log, ALERT_CRIT, "CPU usage critical");
        cpu_warned = 1;
    } else if (cpu < 70.0f && cpu_warned) {
        alert_push(log, ALERT_INFO, "CPU usage nominal");
        cpu_warned = 0;
    }

    if (mem_percent > 80.0f && !mem_warned) {
        alert_push(log, ALERT_CRIT, "RAM usage critical");
        mem_warned = 1;
    } else if (mem_percent > 60.0f && !mem_warned) {
        alert_push(log, ALERT_WARN, "RAM usage elevated");
        mem_warned = 1;
    } else if (mem_percent < 60.0f && mem_warned) {
        alert_push(log, ALERT_INFO, "RAM usage nominal");
        mem_warned = 0;
    }

    prev_cpu = cpu;
    prev_mem = mem_percent;
}