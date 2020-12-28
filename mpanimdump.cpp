#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "AnimFormat.h"
#include "AnimExFormat.h"
#include "AtbFormat.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static char *prog_name;

void PrintError(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    exit(1);
    va_end(args);
}

static void PrintUsage()
{
    printf("Usage: %s src_file [out_name]", prog_name);
    exit(1);
}

void ReadSkip(FILE *file, size_t count)
{
    fseek(file, count, SEEK_CUR);
}

void SetSeek(FILE *file, size_t ofs)
{
    fseek(file, ofs, SEEK_SET);
}

uint8_t ReadU8(FILE *file)
{
    uint8_t temp;
    fread(&temp, 1, 1, file);
    return temp;
}

int8_t ReadS8(FILE *file)
{
    return ReadU8(file);
}

uint16_t ReadU16(FILE *file)
{
    uint8_t temp[2];
    fread(&temp, 1, 2, file);
    return (temp[0] << 8)|temp[1];
}

int16_t ReadS16(FILE *file)
{
    return ReadU16(file);
}

uint32_t ReadU32(FILE *file)
{
    uint8_t temp[4];
    fread(&temp, 1, 4, file);
    return (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
}

int32_t ReadS32(FILE *file)
{
    return ReadU32(file);
}

float ReadFloat(FILE *file)
{
    int32_t raw = ReadS32(file);
    return *(float *)(&raw);
}

int GetFileType(FILE *file)
{
    SetSeek(file, 0);
    if (ReadU32(file) != 0x414E494D) {
        SetSeek(file, 12);
        if (ReadU32(file) == 0x14) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return 1;
    }
}

int main(int argc, char **argv)
{
    AnimFormat *format = NULL;
    prog_name = argv[0];
    if (argc != 2 && argc != 3) {
        PrintUsage();
    }
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        PrintError("Failed to open %s for reading.", argv[1]);
    }
    int file_type = GetFileType(file);
    SetSeek(file, 0);
    switch (file_type) {
        case 0:
            format = new AtbFormat(file);
            break;

        case 1:
            format = new AnimExFormat(file);
            break;

        default:
            fclose(file);
            PrintError("File %s is an invalid animation file.\n", argv[1]);
            break;
    }
    std::string out_dir = "";
    fclose(file);
    out_dir = argv[1];
    if (out_dir.find_last_of("\\/") != std::string::npos) {
        out_dir = out_dir.substr(0, out_dir.find_last_of("\\/"));
    } else {
        out_dir = "";
    }
    std::string out_name = "";
    if (argc == 3) {
        out_name = argv[2];
    } else {
        out_name = argv[1];
        if(out_name.find_last_of("\\/") != std::string::npos && out_name.find_last_of(".") != std::string::npos) {
            size_t start = out_name.find_last_of("\\/") + 1;
            size_t end = out_name.find_last_of(".");
            std::string temp(out_name.c_str()+start, end-start);
            out_name = temp;
        } else if (out_name.find_last_of(".") != std::string::npos) {
            size_t length = out_name.find_last_of(".");
            std::string temp(out_name.c_str(), length);
            out_name = temp;
        }
    }
    std::string create_dir = out_dir + "\\" + out_name + "\\";
    if (mkdir(create_dir.c_str()) == ENOENT) {
        PrintError("Failed to create directory %s.\n", create_dir.c_str());
    }
    format->Dump(out_dir, out_name);
    delete format;
}