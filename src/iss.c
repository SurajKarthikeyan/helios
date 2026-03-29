#include "iss.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#define ISS_URL "http://api.open-notify.org/iss-now.json"

typedef struct {
    char *data;
    size_t size;
} Buffer;

static ISSData iss_data = {0};
static HANDLE iss_thread = NULL;
static CRITICAL_SECTION iss_lock;
static volatile int iss_running = 1;

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Buffer *buf = (Buffer *)userp;
    buf->data = realloc(buf->data, buf->size + total + 1);
    if (!buf->data) return 0;
    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static void fetch_iss(void) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    Buffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, ISS_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && buf.data) {
        cJSON *json = cJSON_Parse(buf.data);
        if (json) {
            cJSON *pos = cJSON_GetObjectItem(json, "iss_position");
            if (pos) {
                cJSON *lat = cJSON_GetObjectItem(pos, "latitude");
                cJSON *lon = cJSON_GetObjectItem(pos, "longitude");
                if (lat && lon) {
                    ISSData tmp;
                    tmp.latitude  = (float)atof(lat->valuestring);
                    tmp.longitude = (float)atof(lon->valuestring);
                    tmp.altitude  = 408.0f;
                    tmp.velocity  = 7.66f;
                    tmp.valid     = 1;
                    EnterCriticalSection(&iss_lock);
                    iss_data = tmp;
                    LeaveCriticalSection(&iss_lock);
                }
            }
            cJSON_Delete(json);
        }
    }
    free(buf.data);
}

static DWORD WINAPI iss_thread_func(LPVOID arg) {
    (void)arg;
    while (iss_running) {
        fetch_iss();
        Sleep(5000);
    }
    return 0;
}

void iss_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    InitializeCriticalSection(&iss_lock);
    iss_running = 1;
    iss_thread = CreateThread(NULL, 0, iss_thread_func, NULL, 0, NULL);
}

void iss_update(void) {
}

ISSData iss_get(void) {
    ISSData tmp;
    EnterCriticalSection(&iss_lock);
    tmp = iss_data;
    LeaveCriticalSection(&iss_lock);
    return tmp;
}

void iss_destroy(void) {
    iss_running = 0;
    if (iss_thread) {
        WaitForSingleObject(iss_thread, 6000);
        CloseHandle(iss_thread);
    }
    DeleteCriticalSection(&iss_lock);
    curl_global_cleanup();
}
