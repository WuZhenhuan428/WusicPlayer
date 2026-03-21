#include "hsv_to_rgb.h"
#include <float.h>
#include <math.h>

#define MAX(x,y,z)  (((x) > (y)) ? ((x) > (z) ? (x) : (z)) : ((y) > (z) ? (y) : (z)))
#define MIN(x,y,z)  (((x) < (y)) ? ((x) < (z) ? (x) : (z)) : ((y) < (z) ? (y) : (z)))

// hsv: 360, 100, 100
hsv_t rgb_to_hsv(rgb_t rgb) {
    hsv_t hsv = { 0 };

    double r = rgb.r / 255.0;
    double g = rgb.g / 255.0;
    double b = rgb.b / 255.0;
    double max = MAX(r, g, b);
    double min = MIN(r, g, b);
    
    hsv.v = max * 100.0;
    double delta = max - min;

    if (max > 0) {
        hsv.s = (delta / max) * 100.0;
    } else {
        hsv.s = 0;
    }
    
    if (delta < FLT_EPSILON) {
        hsv.h = 0;  // 灰色，色相未定义，通常设为0
    } else {
        if (max == r) {
            hsv.h = 60.0 * fmodf((g - b) / delta, 6.0f);
        } else if (max == g) {
            hsv.h = 60.0 * (((b - r) / delta) + 2.0f);
        } else {
            hsv.h = 60.0 * (((r - g) / delta) + 4.0f);
        }
        
        if (hsv.h < 0) {
            hsv.h += 360.0;
        }
    }

    return hsv;
}

rgb_t hsv_to_rgb(hsv_t hsv) {
    rgb_t rgb = {0, 0, 0};

    float r, g, b;
    float h, s, v;
    float f, p, q, t;
    int hi;
    
    // normalize
    h = hsv.h / 360.0;
    s = hsv.s / 100.0;
    v = hsv.v / 100.0;
    
    // boundary
    if (s <= 0.0f) {
        r = g = b = v;
    } else {
        hi = (int)(h * 6) % 6;
        f = (h * 6) - hi;
        p = v * (1 - s);
        q = v * (1 - f * s);
        t = v * (1 - (1 - f) * s);
        
        switch (hi) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }
    
    rgb.r = (unsigned char)(r * 255.0f + 0.5f);
    rgb.g = (unsigned char)(g * 255.0f + 0.5f);
    rgb.b = (unsigned char)(b * 255.0f + 0.5f);
    
    return rgb;
}
