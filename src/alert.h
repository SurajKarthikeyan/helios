#ifndef ALERT_H
#define ALERT_H

#define MAX_ALERTS 32
#define ALERT_MSG_LEN 64

typedef enum {
    ALERT_INFO,
    ALERT_WARN,
    ALERT_CRIT
} AlertLevel;

typedef struct {
    char timestamp[12];
    AlertLevel level;
    char message[ALERT_MSG_LEN];
} Alert;

typedef struct {
    Alert entries[MAX_ALERTS];
    int count;
    int head;
} AlertLog;

void alert_init(AlertLog *log);
void alert_push(AlertLog *log, AlertLevel level, const char *msg);
void alert_check_sysinfo(AlertLog *log, float cpu, float mem_percent);

#endif