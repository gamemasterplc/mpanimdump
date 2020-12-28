#pragma once

#include <stdio.h>
#include <stdint.h>

void PrintError(const char *fmt, ...);

void ReadSkip(FILE *file, size_t count);
void SetSeek(FILE *file, size_t ofs);
uint8_t ReadU8(FILE *file);
int8_t ReadS8(FILE *file);
uint16_t ReadU16(FILE *file);
int16_t ReadS16(FILE *file);
uint32_t ReadU32(FILE *file);
int32_t ReadS32(FILE *file);
float ReadFloat(FILE *file);