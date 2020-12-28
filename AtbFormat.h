#pragma once

#include "AnimFormat.h"
#include "mpanimdump.h"

#define ATB_TEX_FORMAT_RGBA8 0
#define ATB_TEX_FORMAT_RGB5A3 1
#define ATB_TEX_FORMAT_RGB5A3_DUPE 2
#define ATB_TEX_FORMAT_CI8 3
#define ATB_TEX_FORMAT_CI4 4
#define ATB_TEX_FORMAT_IA8 5
#define ATB_TEX_FORMAT_IA4 6
#define ATB_TEX_FORMAT_I8 7
#define ATB_TEX_FORMAT_I4 8
#define ATB_TEX_FORMAT_A8 9
#define ATB_TEX_FORMAT_CMPR 10
#define ATB_TEX_FORMAT_COUNT 11

struct AtbBankFrame {
	int16_t pattern;
	int16_t delay;
	int16_t unused[4];
};

struct AtbBank {
	int16_t num_frames;
	AtbBankFrame *frames;
};

struct AtbLayer {
	uint8_t alpha;
	uint8_t flip;
	int16_t tex_index;
	int16_t src_x;
	int16_t src_y;
	int16_t w;
	int16_t h;
	int16_t shift_x;
	int16_t shift_y;
	int16_t vertices[8];
};

struct AtbPattern {
	int16_t num_layers;
	int16_t center_x;
	int16_t center_y;
	int16_t w;
	int16_t h;
	AtbLayer *layers;
};

struct AtbTexture {
	uint8_t bpp;
	uint8_t format;
	int16_t palette_len;
	int16_t w;
	int16_t h;
	uint8_t *data;
	uint8_t *pal_data;
};

class AtbFormat : public AnimFormat
{
public:
	AtbFormat(FILE *file);

public:
	virtual ~AtbFormat();
	virtual void Dump(std::string dir, std::string name);

private:
	int16_t m_num_banks;
	int16_t m_num_patterns;
	int16_t m_num_textures;
	AtbBank *m_bank_data;
	AtbPattern *m_pattern_data;
	AtbTexture *m_texture_data;

private:
	void ReadBank(FILE *file);
	void ReadPattern(FILE *file);
	void ReadTexture(FILE *file);
};

