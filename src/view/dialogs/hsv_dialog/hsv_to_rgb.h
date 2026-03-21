#pragma once

#include "core/hsv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

hsv_t rgb_to_hsv(rgb_t rgb);
rgb_t hsv_to_rgb(hsv_t hsv);

#ifdef __cplusplus
}
#endif
