#ifndef ISS_H
#define ISS_H

typedef struct {
    float latitude;
    float longitude;
    float altitude;
    float velocity;
    int valid;
} ISSData;

void iss_init(void);
void iss_update(void);
ISSData iss_get(void);
void iss_destroy(void);

#endif
