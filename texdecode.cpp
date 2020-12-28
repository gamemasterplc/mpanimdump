#include "texdecode.h"
#include "mpanimdump.h"

const uint8_t color_3_to_8[8] =
{
	0x00,0x24,0x49,0x6d, 0x92,0xb6,0xdb,0xff
};

const uint8_t color_4_to_8[16] =
{
	0x00,0x11,0x22,0x33, 0x44,0x55,0x66,0x77, 0x88,0x99,0xaa,0xbb, 0xcc,0xdd,0xee,0xff
};

const uint8_t color_5_to_8[32] =
{
	0x00,0x08,0x10,0x19, 0x21,0x29,0x31,0x3a, 0x42,0x4a,0x52,0x5a, 0x63,0x6b,0x73,0x7b,
	0x84,0x8c,0x94,0x9c, 0xa5,0xad,0xb5,0xbd, 0xc5,0xce,0xd6,0xde, 0xe6,0xef,0xf7,0xff
};

const uint8_t color_6_to_8[64] =
{
	0x00,0x04,0x08,0x0c, 0x10,0x14,0x18,0x1c, 0x20,0x24,0x28,0x2d, 0x31,0x35,0x39,0x3d,
	0x41,0x45,0x49,0x4d, 0x51,0x55,0x59,0x5d, 0x61,0x65,0x69,0x6d, 0x71,0x75,0x79,0x7d,
	0x82,0x86,0x8a,0x8e, 0x92,0x96,0x9a,0x9e, 0xa2,0xa6,0xaa,0xae, 0xb2,0xb6,0xba,0xbe,
	0xc2,0xc6,0xca,0xce, 0xd2,0xd7,0xdb,0xdf, 0xe3,0xe7,0xeb,0xef, 0xf3,0xf7,0xfb,0xff
};

static uint8_t *DecodeRGBA8(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h*4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t pixel_ofs = ((((i >> 2) * ((w + 3) >> 2) + (j >> 2)) << 5) + ((i & 3) << 2) + (j & 3)) << 1;
			dst[(((i * w) + j) * 4) + 0] = data[pixel_ofs+1];
			dst[(((i * w) + j) * 4) + 1] = data[pixel_ofs+32];
			dst[(((i * w) + j) * 4) + 2] = data[pixel_ofs+33];
			dst[(((i * w) + j) * 4) + 3] = data[pixel_ofs];
		}
	}
	return dst;
}

static void WriteRGB5A3Pixel(uint8_t *dst, int16_t w, int16_t x, int16_t y, uint16_t value)
{
	uint8_t r, g, b, a;
	if (value & 0x8000) {
		r = color_5_to_8[(value >> 10) & 0x1F];
		g = color_5_to_8[(value >> 5) & 0x1F];
		b = color_5_to_8[value & 0x1F];
		a = 255;
	}
	else {
		r = color_4_to_8[(value >> 8) & 0xF];
		g = color_4_to_8[(value >> 4) & 0xF];
		b = color_4_to_8[value & 0xF];
		a = color_4_to_8[(value >> 12) & 0x7];
	}
	dst[(((y * w) + x) * 4) + 0] = r;
	dst[(((y * w) + x) * 4) + 1] = g;
	dst[(((y * w) + x) * 4) + 2] = b;
	dst[(((y * w) + x) * 4) + 3] = a;
}

static uint8_t *DecodeRGB5A3(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 4) * ((w + 3) / 4)) + (j / 4));
			uint32_t pixel_idx = ((i % 4) * 4) + (j % 4);
			uint32_t pixel_ofs = (block_idx * 32) + (pixel_idx * 2);
			WriteRGB5A3Pixel(dst, w, j, i, (data[pixel_ofs] << 8) | data[pixel_ofs + 1]);
		}
	}
	return dst;
}

static uint8_t *DecodeCI8(uint8_t *data, uint8_t *pal_data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 4) * ((w + 7) / 8)) + (j / 8));
			uint32_t pixel_idx = ((i % 4) * 8) + (j % 8);
			uint32_t pixel_ofs = (block_idx * 32) + pixel_idx;
			WriteRGB5A3Pixel(dst, w, j, i, (pal_data[data[pixel_ofs] * 2] << 8) | pal_data[(data[pixel_ofs] * 2) + 1]);
		}
	}
	return dst;
}

static uint8_t *DecodeCI4(uint8_t *data, uint8_t *pal_data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 8) * ((w + 7) / 8)) + (j / 8));
			uint32_t pixel_idx = ((i % 8) * 8) + (j % 8);
			uint32_t pixel_ofs = (block_idx * 32) + (pixel_idx / 2);
			uint8_t pixel = data[pixel_ofs];
			if (j % 2) {
				pixel &= 0xF;
			} else {
				pixel >>= 4;
			}
			WriteRGB5A3Pixel(dst, w, j, i, (pal_data[pixel * 2] << 8) | pal_data[(pixel * 2) + 1]);
		}
	}
	return dst;
}

static uint8_t *DecodeIA8(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 4) * ((w + 3) / 4)) + (j / 4));
			uint32_t pixel_idx = ((i % 4) * 4) + (j % 4);
			uint32_t pixel_ofs = (block_idx * 32) + (pixel_idx * 2);
			dst[(((i * w) + j) * 4) + 0] = data[pixel_ofs + 1];
			dst[(((i * w) + j) * 4) + 1] = data[pixel_ofs + 1];
			dst[(((i * w) + j) * 4) + 2] = data[pixel_ofs + 1];
			dst[(((i * w) + j) * 4) + 3] = data[pixel_ofs];
		}
	}
	return dst;
}

static uint8_t *DecodeIA4(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 4) * ((w + 7) / 8)) + (j / 8));
			uint32_t pixel_idx = ((i % 4) * 8) + (j % 8);
			uint32_t pixel_ofs = (block_idx * 32) + pixel_idx;
			uint8_t i_value = color_4_to_8[data[pixel_ofs] & 0xF];
			uint8_t a_value = color_4_to_8[(data[pixel_ofs] >> 4) & 0xF];
			dst[(((i * w) + j) * 4) + 0] = i_value;
			dst[(((i * w) + j) * 4) + 1] = i_value;
			dst[(((i * w) + j) * 4) + 2] = i_value;
			dst[(((i * w) + j) * 4) + 3] = a_value;
		}
	}
	return dst;
}

static uint8_t *DecodeI8(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 4) * ((w + 7) / 8)) + (j / 8));
			uint32_t pixel_idx = ((i % 4) * 8) + (j % 8);
			uint32_t pixel_ofs = (block_idx * 32) + pixel_idx;
			uint8_t value = data[pixel_ofs];
			dst[(((i * w) + j) * 4) + 0] = value;
			dst[(((i * w) + j) * 4) + 1] = value;
			dst[(((i * w) + j) * 4) + 2] = value;
			dst[(((i * w) + j) * 4) + 3] = value;
		}
	}
	return dst;
}

static uint8_t *DecodeI4(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 8) * ((w + 7) / 8)) + (j / 8));
			uint32_t pixel_idx = ((i % 8) * 8) + (j % 8);
			uint32_t pixel_ofs = (block_idx * 32) + (pixel_idx / 2);
			uint8_t value;
			if (j % 2) {
				value = data[pixel_ofs] >> 4;
			} else {
				value = data[pixel_ofs] & 0xF;
			}
			dst[(((i * w) + j) * 4) + 0] = value;
			dst[(((i * w) + j) * 4) + 1] = value;
			dst[(((i * w) + j) * 4) + 2] = value;
			dst[(((i * w) + j) * 4) + 3] = value;
		}
	}
	return dst;
}

static uint8_t *DecodeA8(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 4) * ((w + 7) / 8)) + (j / 8));
			uint32_t pixel_idx = ((i % 4) * 8) + (j % 8);
			uint32_t pixel_ofs = (block_idx * 32) + pixel_idx;
			uint8_t value = data[pixel_ofs];
			dst[(((i * w) + j) * 4) + 0] = 255;
			dst[(((i * w) + j) * 4) + 1] = 255;
			dst[(((i * w) + j) * 4) + 2] = 255;
			dst[(((i * w) + j) * 4) + 3] = value;
		}
	}
	return dst;
}

static void DecodeRGB565(uint16_t value, uint8_t *dst)
{
	dst[0] = color_5_to_8[(value >> 11) & 0x1F];
	dst[1] = color_6_to_8[(value >> 5) & 0x3F];
	dst[2] = color_5_to_8[value & 0x1F];
	dst[3] = 0xFF;
}

static uint8_t *DecodeCMPR(uint8_t *data, int16_t w, int16_t h)
{
	uint8_t *dst = new uint8_t[w * h * 4];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			uint32_t block_idx = (((i / 8) * ((w + 7) / 8)) + (j / 8));
			uint32_t block_4x4_idx = (((i & 4) != 0) * 2) + ((j & 4) != 0);
			uint32_t pixel_idx = ((i % 4) * 4) + (j % 4);
			uint32_t block_4x4_ofs = (block_idx * 32) + (block_4x4_idx * 8);
			uint32_t pixel_ofs = block_4x4_ofs + 4 + (pixel_idx / 4);
			uint16_t color0, color1;
			uint8_t decoded_color[4][4];
			color0 = (data[block_4x4_ofs] << 8) | data[block_4x4_ofs+1];
			color1 = (data[block_4x4_ofs + 2] << 8) | data[block_4x4_ofs + 3];
			decoded_color[0][0] = color_5_to_8[(color0 >> 11) & 0x1F];
			decoded_color[0][1] = color_6_to_8[(color0 >> 5) & 0x3F];
			decoded_color[0][2] = color_5_to_8[color0 & 0x1F];
			decoded_color[0][3] = 255;
			decoded_color[1][0] = color_5_to_8[(color1 >> 11) & 0x1F];
			decoded_color[1][1] = color_6_to_8[(color1 >> 5) & 0x3F];
			decoded_color[1][2] = color_5_to_8[color1 & 0x1F];
			decoded_color[1][3] = 255;
			if (color0 > color1) {
				decoded_color[2][0] = ((decoded_color[1][0] * 3 + decoded_color[0][0] * 5) >> 3);
				decoded_color[2][1] = ((decoded_color[1][1] * 3 + decoded_color[0][1] * 5) >> 3);
				decoded_color[2][2] = ((decoded_color[1][2] * 3 + decoded_color[0][2] * 5) >> 3);
				decoded_color[2][3] = 255;
				decoded_color[3][0] = ((decoded_color[0][0] * 3 + decoded_color[1][0] * 5) >> 3);
				decoded_color[3][1] = ((decoded_color[0][1] * 3 + decoded_color[1][1] * 5) >> 3);
				decoded_color[3][2] = ((decoded_color[0][2] * 3 + decoded_color[1][2] * 5) >> 3);
				decoded_color[3][3] = 255;
			} else {
				decoded_color[2][0] = (decoded_color[1][0] + decoded_color[0][0]) / 2;
				decoded_color[2][1] = (decoded_color[1][1] + decoded_color[0][1]) / 2;
				decoded_color[2][2] = (decoded_color[1][2] + decoded_color[0][2]) / 2;
				decoded_color[2][3] = 255;
				decoded_color[3][0] = (decoded_color[1][0] + decoded_color[0][0]) / 2;
				decoded_color[3][1] = (decoded_color[1][1] + decoded_color[0][1]) / 2;
				decoded_color[3][2] = (decoded_color[1][2] + decoded_color[0][2]) / 2;
				decoded_color[3][3] = 0;
			}
			uint8_t color_index = (data[pixel_ofs] >> (6 - (2 * (j % 4)))) & 0x3;
			dst[(((i * w) + j) * 4) + 0] = decoded_color[color_index][0];
			dst[(((i * w) + j) * 4) + 1] = decoded_color[color_index][1];
			dst[(((i * w) + j) * 4) + 2] = decoded_color[color_index][2];
			dst[(((i * w) + j) * 4) + 3] = decoded_color[color_index][3];
		}
	}
	return dst;
}

uint8_t *DecodeTexture(uint8_t *data, uint8_t *pal_data, int16_t w, int16_t h, uint8_t format)
{
	switch (format) {
		case TEX_FORMAT_RGBA8:
			return DecodeRGBA8(data, w, h);

		case TEX_FORMAT_RGB5A3:
			return DecodeRGB5A3(data, w, h);

		case TEX_FORMAT_CI8:
			return DecodeCI8(data, pal_data, w, h);

		case TEX_FORMAT_CI4:
			return DecodeCI4(data, pal_data, w, h);

		case TEX_FORMAT_IA8:
			return DecodeIA8(data, w, h);

		case TEX_FORMAT_IA4:
			return DecodeIA4(data, w, h);

		case TEX_FORMAT_I8:
			return DecodeI8(data, w, h);

		case TEX_FORMAT_I4:
			return DecodeI4(data, w, h);

		case TEX_FORMAT_A8:
			return DecodeA8(data, w, h);

		case TEX_FORMAT_CMPR:
			return DecodeCMPR(data, w, h);

		default:
			PrintError("Texture format %d unimplemented.", format);
			return nullptr;
	}
}