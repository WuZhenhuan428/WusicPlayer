#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double h;
    double s;
    double v;
} hsv_t;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} rgb_t;

#ifdef __cplusplus
}
#endif