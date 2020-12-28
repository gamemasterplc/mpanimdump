#include "AtbFormat.h"
#include "tinyxml2.h"
#include "texdecode.h"
#include "stb_image_write.h"

void AtbFormat::ReadBank(FILE *file)
{
	m_bank_data = new AtbBank[m_num_banks];
	for (int16_t i = 0; i < m_num_banks; i++) {
		size_t next_bank_ofs = ftell(file) + 8;
		m_bank_data[i].num_frames = ReadS16(file);
		m_bank_data[i].frames = new AtbBankFrame[m_bank_data[i].num_frames];
		ReadSkip(file, 2);
		uint32_t frame_ofs = ReadU32(file);
		SetSeek(file, frame_ofs);
		for (int16_t j = 0; j < m_bank_data[i].num_frames; j++) {
			m_bank_data[i].frames[j].pattern = ReadS16(file);
			m_bank_data[i].frames[j].delay = ReadS16(file);
			ReadSkip(file, 8);
		}
		SetSeek(file, next_bank_ofs);
	}
}

void AtbFormat::ReadPattern(FILE *file)
{
	m_pattern_data = new AtbPattern[m_num_patterns];
	for (int16_t i = 0; i < m_num_patterns; i++) {
		size_t next_pattern_ofs = ftell(file) + 16;
		m_pattern_data[i].num_layers = ReadS16(file);
		m_pattern_data[i].center_x = ReadS16(file);
		m_pattern_data[i].center_y = ReadS16(file);
		m_pattern_data[i].w = ReadS16(file);
		m_pattern_data[i].h = ReadS16(file);
		ReadSkip(file, 2);
		uint32_t layer_ofs = ReadU32(file);
		SetSeek(file, layer_ofs);
		m_pattern_data[i].layers = new AtbLayer[m_pattern_data[i].num_layers];
		for (int16_t j = 0; j < m_pattern_data[i].num_layers; j++) {
			m_pattern_data[i].layers[j].alpha = ReadU8(file);
			m_pattern_data[i].layers[j].flip = ReadU8(file);
			m_pattern_data[i].layers[j].tex_index = ReadS16(file);
			m_pattern_data[i].layers[j].src_x = ReadS16(file);
			m_pattern_data[i].layers[j].src_y = ReadS16(file);
			m_pattern_data[i].layers[j].w = ReadS16(file);
			m_pattern_data[i].layers[j].h = ReadS16(file);
			m_pattern_data[i].layers[j].shift_x = ReadS16(file);
			m_pattern_data[i].layers[j].shift_y = ReadS16(file);
			ReadSkip(file, 4);
			for (int16_t k = 0; k < 8; k++) {
				m_pattern_data[i].layers[j].vertices[k] = ReadS16(file);
			}
		}
		SetSeek(file, next_pattern_ofs);
	}
}


void AtbFormat::ReadTexture(FILE *file)
{
	m_texture_data = new AtbTexture[m_num_textures];
	for (int16_t i = 0; i < m_num_textures; i++) {
		size_t next_texture_ofs = ftell(file) + 20;
		m_texture_data[i].bpp = ReadU8(file);
		m_texture_data[i].format = ReadU8(file) & 0xF;
		m_texture_data[i].palette_len = ReadS16(file);
		m_texture_data[i].w = ReadS16(file);
		m_texture_data[i].h = ReadS16(file);
		uint32_t data_len = ReadU32(file);
		m_texture_data[i].data = new uint8_t[data_len];
		if (m_texture_data[i].format == ATB_TEX_FORMAT_CI8 || m_texture_data[i].format == ATB_TEX_FORMAT_CI4) {
			m_texture_data[i].pal_data = new uint8_t[m_texture_data[i].palette_len * 2];
		} else {
			m_texture_data[i].pal_data = nullptr;
		}
		uint32_t pal_ofs = ReadU32(file);
		uint32_t bmp_ofs = ReadU32(file);
		SetSeek(file, bmp_ofs);
		fread(m_texture_data[i].data, 1, data_len, file);
		if (m_texture_data[i].format == ATB_TEX_FORMAT_CI8 || m_texture_data[i].format == ATB_TEX_FORMAT_CI4) {
			SetSeek(file, pal_ofs);
			fread(m_texture_data[i].pal_data, 1, m_texture_data[i].palette_len * 2, file);
		}
		SetSeek(file, next_texture_ofs);
	}
}

AtbFormat::AtbFormat(FILE *file)
{
	m_num_banks = ReadS16(file);
	m_num_patterns = ReadS16(file);
	m_num_textures = ReadS16(file) & 0x7FFF;
	ReadSkip(file, 2);
	uint32_t bank_ofs = ReadU32(file);
	uint32_t pattern_ofs = ReadU32(file);
	uint32_t texture_ofs = ReadU32(file);
	SetSeek(file, bank_ofs);
	ReadBank(file);
	SetSeek(file, pattern_ofs);
	ReadPattern(file);
	SetSeek(file, texture_ofs);
	ReadTexture(file);
}

AtbFormat::~AtbFormat()
{
	for (int16_t i = 0; i < m_num_banks; i++) {
		delete[] m_bank_data[i].frames;
	}
	delete[] m_bank_data;
	for (int16_t i = 0; i < m_num_patterns; i++) {
		delete[] m_pattern_data[i].layers;
	}
	delete[] m_pattern_data;
	for (int16_t i = 0; i < m_num_patterns; i++) {
		delete[] m_texture_data[i].data;
		delete[] m_texture_data[i].pal_data;
	}
	delete[] m_texture_data;
}

void AtbFormat::Dump(std::string dir, std::string name)
{
	tinyxml2::XMLDocument document;
	tinyxml2::XMLNode *root = document.NewElement("anim");
	document.InsertFirstChild(root);
	tinyxml2::XMLElement *banks = document.NewElement("banks");
	for (int16_t i = 0; i < m_num_banks; i++) {
		tinyxml2::XMLElement *bank = document.NewElement("bank");
		std::string bank_name = "bank" + std::to_string(i);
		bank->SetAttribute("name", bank_name.c_str());
		for (int16_t j = 0; j < m_bank_data[i].num_frames; j++) {
			if (m_bank_data[i].frames[j].pattern == -1) { //Prematurely Terminate Animation Frame
				break;
			}
			tinyxml2::XMLElement *frame = document.NewElement("frame");
			std::string pattern = "pattern" + std::to_string(m_bank_data[i].frames[j].pattern);
			frame->SetAttribute("pattern", pattern.c_str());
			frame->SetAttribute("delay", m_bank_data[i].frames[j].delay);
			bank->InsertEndChild(frame);
		}
		banks->InsertEndChild(bank);
	}
	root->InsertEndChild(banks);
	tinyxml2::XMLElement *patterns = document.NewElement("patterns");
	for (int16_t i = 0; i < m_num_patterns; i++) {
		tinyxml2::XMLElement *pattern = document.NewElement("pattern");
		std::string pattern_name = "pattern" + std::to_string(i);
		pattern->SetAttribute("name", pattern_name.c_str());
		pattern->SetAttribute("center_x", m_pattern_data[i].center_x);
		pattern->SetAttribute("center_y", m_pattern_data[i].center_y);
		pattern->SetAttribute("w", m_pattern_data[i].w);
		pattern->SetAttribute("h", m_pattern_data[i].h);
		for (int16_t j = 0; j < m_pattern_data[i].num_layers; j++) {
			tinyxml2::XMLElement *layer = document.NewElement("layer");
			std::string texture_name = "texture"+ std::to_string(m_pattern_data[i].layers[j].tex_index);
			layer->SetAttribute("alpha", m_pattern_data[i].layers[j].alpha);
			layer->SetAttribute("flip_x", (m_pattern_data[i].layers[j].flip & 0x1) != 0);
			layer->SetAttribute("flip_y", (m_pattern_data[i].layers[j].flip & 0x2) != 0);
			layer->SetAttribute("tex_name", texture_name.c_str());
			layer->SetAttribute("src_x", m_pattern_data[i].layers[j].src_x);
			layer->SetAttribute("src_y", m_pattern_data[i].layers[j].src_y);
			layer->SetAttribute("w", m_pattern_data[i].layers[j].w);
			layer->SetAttribute("h", m_pattern_data[i].layers[j].h);
			layer->SetAttribute("shift_x", m_pattern_data[i].layers[j].shift_x);
			layer->SetAttribute("shift_y", m_pattern_data[i].layers[j].shift_y);
			pattern->InsertEndChild(layer);
		}
		patterns->InsertEndChild(pattern);
	}
	root->InsertEndChild(patterns);
	tinyxml2::XMLElement *textures = document.NewElement("textures");
	for (int16_t i = 0; i < m_num_textures; i++) {
		const char *format_names[ATB_TEX_FORMAT_COUNT] = { "RGBA8", "RGB5A3", "RGB5A3", "CI8", "CI4", "IA8", "IA4", "I8", "I4", "A8", "CMPR" };
		tinyxml2::XMLElement *texture = document.NewElement("texture");
		std::string texture_name = "texture" + std::to_string(i);
		std::string texture_path = name + "\\" + texture_name + ".png";
		std::string texture_full_path = dir + "\\" + texture_path;
		texture->SetAttribute("name", texture_name.c_str());
		texture->SetAttribute("format", format_names[m_texture_data[i].format]);
		texture->SetAttribute("file", texture_path.c_str());
		uint8_t *decoded_data = nullptr;
		switch (m_texture_data[i].format) {
			case ATB_TEX_FORMAT_RGBA8:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_RGBA8);
				break;

			case ATB_TEX_FORMAT_RGB5A3:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_RGB5A3);
				break;

			case ATB_TEX_FORMAT_CI8:
				decoded_data = DecodeTexture(m_texture_data[i].data, m_texture_data[i].pal_data, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_CI8);
				break;

			case ATB_TEX_FORMAT_CI4:
				decoded_data = DecodeTexture(m_texture_data[i].data, m_texture_data[i].pal_data, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_CI4);
				break;

			case ATB_TEX_FORMAT_IA8:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_IA8);
				break;

			case ATB_TEX_FORMAT_IA4:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_IA4);
				break;

			case ATB_TEX_FORMAT_I8:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_I8);
				break;

			case ATB_TEX_FORMAT_I4:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_I4);
				break;

			case ATB_TEX_FORMAT_A8:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_A8);
				break;

			case ATB_TEX_FORMAT_CMPR:
				decoded_data = DecodeTexture(m_texture_data[i].data, nullptr, m_texture_data[i].w, m_texture_data[i].h, TEX_FORMAT_CMPR);
				break;

			default:
				break;
		}
		stbi_write_png(texture_full_path.c_str(), m_texture_data[i].w, m_texture_data[i].h, 4, decoded_data, 4 * m_texture_data[i].w);
		textures->InsertEndChild(texture);
	}
	root->InsertEndChild(textures);
	std::string out_name = dir + "\\" + name + ".xml";
	document.SaveFile(out_name.c_str());
}