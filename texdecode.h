#pragma once

#include <stdint.h>

#define TEX_FORMAT_RGBA8 0
#define TEX_FORMAT_RGB5A3 1
#define TEX_FORMAT_CI8 2
#define TEX_FORMAT_CI4 3
#define TEX_FORMAT_IA8 4
#define TEX_FORMAT_IA4 5
#define TEX_FORMAT_I8 6
#define TEX_FORMAT_I4 7
#define TEX_FORMAT_A8 8
#define TEX_FORMAT_CMPR 9

uint8_t *DecodeTexture(uint8_t *data, uint8_t *pal_data, int16_t w, int16_t h, uint8_t format);