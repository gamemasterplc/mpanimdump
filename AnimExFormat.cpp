#include "AnimExFormat.h"
#include "texdecode.h"
#include "stb_image_write.h"

void AnimExFormat::ReadHeader(FILE *file)
{
	header.magic = ReadU32(file);
	ReadSkip(file, 4);
	header.frame_cnt = ReadU32(file);
	header.ref_cnt = ReadU32(file);
	header.root_cnt = ReadU32(file);
	header.type1_cnt = ReadU32(file);
	header.transform_cnt = ReadU32(file);
	header.image_cnt = ReadU32(file);
	header.track_cnt = ReadU32(file);
	header.keyframe_cnt = ReadU32(file);
	header.texture_cnt = ReadU32(file);
	header.node_ref_cnt = ReadU32(file);
	header.frame_start_cnt = ReadU32(file);
	header.str_table_len = ReadU32(file);
	header.root_ofs = ReadU32(file);
	header.type1_ofs = ReadU32(file);
	header.transform_ofs = ReadU32(file);
	header.image_ofs = ReadU32(file);
	header.track_ofs = ReadU32(file);
	header.keyframe_ofs = ReadU32(file);
	header.texture_ofs = ReadU32(file);
	header.node_ref_ofs = ReadU32(file);
	header.frame_start_ofs = ReadU32(file);
	header.str_table_ofs = ReadU32(file);
}

void AnimExFormat::ReadNode(FILE *file, AnimExNode *node)
{
	node->type = ReadS16(file);
	node->child_count = ReadU16(file);
	ReadSkip(file, 4);
	node->children = nullptr;
}

void AnimExFormat::ReadTransforms(FILE *file)
{
	data.transform = new AnimExTransform[header.transform_cnt];
	SetSeek(file, header.transform_ofs);
	for (uint32_t i = 0; i < header.transform_cnt; i++) {
		ReadNode(file, &data.transform[i].node);
		data.transform[i].scale_x = ReadFloat(file);
		data.transform[i].scale_y = ReadFloat(file);
		data.transform[i].scale_z = ReadFloat(file);
		data.transform[i].rot_x = ReadFloat(file);
		data.transform[i].rot_y = ReadFloat(file);
		data.transform[i].rot_z = ReadFloat(file);
		data.transform[i].pos_x = ReadFloat(file);
		data.transform[i].pos_y = ReadFloat(file);
		data.transform[i].pos_z = ReadFloat(file);
		ReadSkip(file, 24);
	}
}

void AnimExFormat::ReadImages(FILE *file)
{
	data.image = new AnimExImage[header.image_cnt];
	SetSeek(file, header.image_ofs);
	for (uint32_t i = 0; i < header.image_cnt; i++) {
		ReadNode(file, &data.image[i].node);
		uint32_t name_ofs = ReadU32(file);
		data.image[i].name = &data.str_table[name_ofs-header.str_table_ofs];
		for (uint32_t j = 0; j < 12; j++) {
			data.image[i].vertices[j] = ReadFloat(file);
		}
		for (uint32_t j = 0; j < 8; j++) {
			data.image[i].uv[j] = ReadFloat(file);
		}
		for (uint32_t j = 0; j < 4; j++) {
			data.image[i].color[j] = ReadFloat(file);
		}
		uint32_t texture_ofs = ReadU32(file);
		data.image[i].texture = &data.texture[(texture_ofs - header.texture_ofs) / 20];
	}
}

void AnimExFormat::ReadKeyframes(FILE *file)
{
	data.keyframe = new AnimExKeyframe[header.keyframe_cnt];
	SetSeek(file, header.keyframe_ofs);
	for (uint32_t i = 0; i < header.keyframe_cnt; i++) {
		data.keyframe[i].interp_type = ReadU32(file);
		data.keyframe[i].frame_num = ReadU32(file);
		for (uint32_t j = 0; j < 4; j++) {
			data.keyframe[i].points[j] = ReadFloat(file);
		}
	}
}

void AnimExFormat::ReadTracks(FILE *file)
{
	data.track = new AnimExTrack[header.track_cnt];
	SetSeek(file, header.track_ofs);
	for (uint32_t i = 0; i < header.track_cnt; i++) {
		data.track[i].node_type = ReadS16(file);
		data.track[i].node_id = ReadU16(file);
		data.track[i].track_type = ReadU16(file);
		data.track[i].var_id = ReadU16(file);
		data.track[i].num_keyframes = ReadU32(file);
		uint32_t keyframe_ofs = ReadU32(file);
		data.track[i].keyframe = &data.keyframe[(keyframe_ofs - header.keyframe_ofs) / 24];
	}
}

void AnimExFormat::ReadTextures(FILE *file)
{
	data.texture = new AnimExTexture[header.texture_cnt];
	for (uint32_t i = 0; i < header.texture_cnt; i++) {
		SetSeek(file, header.texture_ofs + (i * 20));
		data.texture[i].bpp = ReadU8(file);
		data.texture[i].format = ReadU8(file);
		data.texture[i].palette_len = ReadS16(file);
		data.texture[i].w = ReadS16(file);
		data.texture[i].h = ReadS16(file);
		data.texture[i].data_len = ReadU32(file);
		uint32_t pal_ofs = ReadU32(file);
		uint32_t data_ofs = ReadU32(file);
		SetSeek(file, data_ofs);
		data.texture[i].data = new uint8_t[data.texture[i].data_len];
		fread(data.texture[i].data, 1, data.texture[i].data_len, file);
		if (data.texture[i].format == ANIMEX_TEX_FORMAT_CI8 || data.texture[i].format == ANIMEX_TEX_FORMAT_CI4) {
			SetSeek(file, pal_ofs);
			data.texture[i].pal_data = new uint8_t[data.texture[i].palette_len*2];
			fread(data.texture[i].pal_data, 2, data.texture[i].palette_len, file);
		} else {
			data.texture[i].pal_data = nullptr;
		}
	}
}

void AnimExFormat::ReadNodeRefs(FILE *file)
{
	data.node_ref_list = new AnimExNode *[header.node_ref_cnt];
	for (uint32_t i = 0; i < header.node_ref_cnt; i++) {
		SetSeek(file, header.node_ref_ofs + (i * 4));
		uint32_t ofs = ReadU32(file);
		SetSeek(file, ofs);
		int16_t type = ReadS16(file);
		switch (type) {
			case ANIMEX_NODE_TYPE_ROOT:
				data.node_ref_list[i] = data.root;
				break;

			case 1:
				data.node_ref_list[i] = data.type1;
				break;

			case ANIMEX_NODE_TYPE_TRANSFORM:
				data.node_ref_list[i] = &data.transform[(ofs - header.transform_ofs) / 68].node;
				break;

			case ANIMEX_NODE_TYPE_IMAGE:
				data.node_ref_list[i] = &data.image[(ofs - header.image_ofs) / 112].node;
				break;

			default:
				data.node_ref_list[i] = nullptr;
				break;
		}
	}
}

void AnimExFormat::ReadFrameStartList(FILE *file)
{
	data.frame_start_list = new uint32_t[header.frame_start_cnt];
	SetSeek(file, header.frame_start_ofs);
	for (uint32_t i = 0; i < header.frame_start_cnt; i++) {
		data.frame_start_list[i] = ReadU32(file);
	}
}

void AnimExFormat::ReadStringTable(FILE *file)
{
	data.str_table = new char[header.str_table_len];
	SetSeek(file, header.str_table_ofs);
	fread(data.str_table, 1, header.str_table_len, file);
}

void AnimExFormat::FixupNode(AnimExNode *node, uint32_t file_ofs, FILE *file)
{
	SetSeek(file, file_ofs + 4);
	uint32_t child_list_ofs = ReadU32(file);
	if (child_list_ofs != 0) {
		node->children = &data.node_ref_list[(child_list_ofs - header.node_ref_ofs) / 4];
	} else {
		node->children = nullptr;
	}
}

void AnimExFormat::FixupNodes(FILE *file)
{
	FixupNode(data.root, header.root_ofs, file);
	FixupNode(data.type1, header.type1_ofs, file);
	for (uint32_t i = 0; i < header.transform_cnt; i++) {
		FixupNode(&data.transform[i].node, header.transform_ofs+(i*68), file);
	}
	for (uint32_t i = 0; i < header.image_cnt; i++) {
		FixupNode(&data.image[i].node, header.image_ofs + (i * 112), file);
	}
}

void AnimExFormat::ReadData(FILE *file)
{
	ReadStringTable(file);
	data.root = new AnimExNode;
	SetSeek(file, header.root_ofs);
	ReadNode(file, data.root);
	data.type1 = new AnimExNode;
	SetSeek(file, header.type1_ofs);
	ReadNode(file, data.type1);
	ReadTransforms(file);
	ReadTextures(file);
	ReadImages(file);
	ReadKeyframes(file);
	ReadTracks(file);
	ReadNodeRefs(file);
	ReadFrameStartList(file);
}

AnimExFormat::AnimExFormat(FILE *file)
{
	ReadHeader(file);
	ReadData(file);
	FixupNodes(file);
}

AnimExFormat::~AnimExFormat()
{
	for (uint32_t i = 0; i < header.texture_cnt; i++) {
		delete[] data.texture[i].data;
		delete[] data.texture[i].pal_data;
	}
	delete[] data.str_table;
	delete data.root;
	delete data.type1;
	delete[] data.transform;
	delete[] data.texture;
	delete[] data.image;
	delete[] data.keyframe;
	delete[] data.track;
	delete[] data.node_ref_list;
	delete[] data.frame_start_list;
}

void AnimExFormat::DumpTransformNode(tinyxml2::XMLElement *xml_node, AnimExTransform *node)
{
	uint32_t transform_idx = node-data.transform;
	std::string name = "transform" + std::to_string(transform_idx);
	xml_node->SetAttribute("name", name.c_str());
	xml_node->SetAttribute("scale_x", node->scale_x);
	xml_node->SetAttribute("scale_y", node->scale_y);
	xml_node->SetAttribute("rot_z", node->rot_z);
	xml_node->SetAttribute("pos_x", node->pos_x);
	xml_node->SetAttribute("pos_y", node->pos_y);
}

void AnimExFormat::DumpImageNode(tinyxml2::XMLElement *xml_node, AnimExImage *node)
{
	xml_node->SetAttribute("name", node->name);
	float uv_x, uv_y, uv_w, uv_h;
	uv_x = node->uv[0];
	uv_y = node->uv[1];
	uv_w = node->uv[4] - uv_x;
	uv_h = node->uv[5] - uv_y;
	uint32_t texture_idx = node->texture - data.texture;
	std::string texture_name = "texture" + std::to_string(texture_idx);
	xml_node->SetAttribute("texture_name", texture_name.c_str());
	xml_node->SetAttribute("uv_x", uv_x);
	xml_node->SetAttribute("uv_y", uv_y);
	xml_node->SetAttribute("uv_w", uv_w);
	xml_node->SetAttribute("uv_h", uv_h);
	xml_node->SetAttribute("x", node->vertices[0]);
	xml_node->SetAttribute("y", node->vertices[1]);
	xml_node->SetAttribute("w", node->vertices[6] - node->vertices[0]);
	xml_node->SetAttribute("h", node->vertices[7] - node->vertices[1]);
	xml_node->SetAttribute("color_r", node->color[0]);
	xml_node->SetAttribute("color_g", node->color[1]);
	xml_node->SetAttribute("color_b", node->color[2]);
	xml_node->SetAttribute("color_a", node->color[3]);
}

void AnimExFormat::DumpTracks(tinyxml2::XMLDocument *document, tinyxml2::XMLElement *root)
{
	uint32_t i;
	for (i = 0; i < header.track_cnt; i++) {
		tinyxml2::XMLElement *element = document->NewElement("track");
		if (data.track[i].node_type == ANIMEX_NODE_TYPE_TRANSFORM) {
			std::string name = "transform" + std::to_string(data.track[i].node_id);
			element->SetAttribute("target_name", name.c_str());
			switch (data.track[i].track_type) {
				case ANIMEX_TRACK_POS:
					switch (data.track[i].var_id) {
						case ANIMEX_TRACK_VAR_X:
							element->SetAttribute("var", "pos_x");
							break;

						case ANIMEX_TRACK_VAR_Y:
							element->SetAttribute("var", "pos_y");
							break;

						default:
							break;
					}
					break;

				case ANIMEX_TRACK_ROTATE:
					switch (data.track[i].var_id) {
						case ANIMEX_TRACK_VAR_Z:
							element->SetAttribute("var", "rot_z");
							break;

						default:
							break;
					}
					break;

				case ANIMEX_TRACK_SCALE:
					switch (data.track[i].var_id) {
						case ANIMEX_TRACK_VAR_X:
							element->SetAttribute("var", "scale_x");
							break;

						case ANIMEX_TRACK_VAR_Y:
							element->SetAttribute("var", "scale_y");
							break;

						default:
							break;
					}
					break;

				default:
					break;
			}
		} else if (data.track[i].node_type == ANIMEX_NODE_TYPE_IMAGE) {
			if (data.track[i].track_type == ANIMEX_TRACK_COLOR) {
				element->SetAttribute("target_name", data.image[data.track[i].node_id].name);
				switch (data.track[i].var_id) {
					case ANIMEX_TRACK_VAR_R:
						element->SetAttribute("var", "color_r");
						break;

					case ANIMEX_TRACK_VAR_G:
						element->SetAttribute("var", "color_g");
						break;

					case ANIMEX_TRACK_VAR_B:
						element->SetAttribute("var", "color_b");
						break;

					case ANIMEX_TRACK_VAR_A:
						element->SetAttribute("var", "color_a");
						break;

					default:
						break;
				}
			}
		}
		for (uint32_t j = 0; j < data.track[i].num_keyframes; j++) {
			tinyxml2::XMLElement *keyframe_element = document->NewElement("keyframe");
			switch (data.track[i].keyframe[j].interp_type) {
				case ANIMEX_INTERP_MODE_LINEAR:
					keyframe_element->SetAttribute("interp_mode", "linear");
					break;

				case ANIMEX_INTERP_MODE_SPLINE:
					keyframe_element->SetAttribute("interp_mode", "spline");
					break;

				default:
					keyframe_element->SetAttribute("interp_mode", "none");
					break;
			}
			keyframe_element->SetAttribute("frame_num", data.track[i].keyframe[j].frame_num);
			if (data.track[i].keyframe[j].interp_type == ANIMEX_INTERP_MODE_SPLINE) {
				keyframe_element->SetAttribute("point1", data.track[i].keyframe[j].points[0]);
				keyframe_element->SetAttribute("use_point3", data.track[i].keyframe[j].points[1] == 3.0f);
				keyframe_element->SetAttribute("point2", data.track[i].keyframe[j].points[2]);
				keyframe_element->SetAttribute("point3", data.track[i].keyframe[j].points[3]);
			} else {
				keyframe_element->SetAttribute("point", data.track[i].keyframe[j].points[0]);
			}
			element->InsertEndChild(keyframe_element);
		}
		root->InsertEndChild(element);
	}
}

void AnimExFormat::DumpNode(tinyxml2::XMLDocument *document, tinyxml2::XMLElement *parent, AnimExNode *node)
{
	tinyxml2::XMLElement *element;
	switch (node->type) {
		case ANIMEX_NODE_TYPE_ROOT:
			element = document->NewElement("root");
			break;

		case 1:
			element = document->NewElement("type1");
			break;

		case ANIMEX_NODE_TYPE_TRANSFORM:
			element = document->NewElement("transform");
			DumpTransformNode(element, (AnimExTransform *)node);
			break;

		case ANIMEX_NODE_TYPE_IMAGE:
			element = document->NewElement("image");
			DumpImageNode(element, (AnimExImage *)node);
			break;

		default:
			element = nullptr;
			break;
	}
	for (uint16_t i = 0; i < node->child_count; i++) {
		DumpNode(document, element, node->children[i]);
	}
	parent->InsertEndChild(element);
}

void AnimExFormat::DumpTextures(std::string dir, std::string name, tinyxml2::XMLDocument *document, tinyxml2::XMLElement *root)
{
	tinyxml2::XMLElement *textures = document->NewElement("textures");
	for (uint32_t i = 0; i < header.texture_cnt; i++) {
		tinyxml2::XMLElement *element = document->NewElement("texture");
		std::string tex_name = "texture" + std::to_string(i);
		std::string tex_path = name + "\\" + tex_name + ".png";
		std::string tex_full_path = dir + "\\" + tex_path;
		element->SetAttribute("name", tex_name.c_str());
		const char *format_names[ANIMEX_TEX_FORMAT_COUNT] = { "RGBA8", "RGB5A3", "RGB5A3", "CI8", "CI4", "IA8", "IA4", "I8", "I4", "A8", "CMPR" };
		element->SetAttribute("format", format_names[data.texture[i].format]);
		element->SetAttribute("file", tex_path.c_str());
		uint8_t *decoded_data = nullptr;
		switch (data.texture[i].format) {
		case ANIMEX_TEX_FORMAT_RGBA8:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_RGBA8);
			break;

		case ANIMEX_TEX_FORMAT_RGB5A3:
		case ANIMEX_TEX_FORMAT_RGB5A3_DUPE:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_RGB5A3);
			break;

		case ANIMEX_TEX_FORMAT_CI8:
			decoded_data = DecodeTexture(data.texture[i].data, data.texture[i].pal_data, data.texture[i].w, data.texture[i].h, TEX_FORMAT_CI8);
			break;

		case ANIMEX_TEX_FORMAT_CI4:
			decoded_data = DecodeTexture(data.texture[i].data, data.texture[i].pal_data, data.texture[i].w, data.texture[i].h, TEX_FORMAT_CI4);
			break;

		case ANIMEX_TEX_FORMAT_IA8:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_IA8);
			break;

		case ANIMEX_TEX_FORMAT_IA4:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_IA4);
			break;

		case ANIMEX_TEX_FORMAT_I8:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_I8);
			break;

		case ANIMEX_TEX_FORMAT_I4:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_I4);
			break;

		case ANIMEX_TEX_FORMAT_A8:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_A8);
			break;

		case ANIMEX_TEX_FORMAT_CMPR:
			decoded_data = DecodeTexture(data.texture[i].data, nullptr, data.texture[i].w, data.texture[i].h, TEX_FORMAT_CMPR);
			break;

		default:
			break;
		}
		stbi_write_png(tex_full_path.c_str(), data.texture[i].w, data.texture[i].h, 4, decoded_data, 4 * data.texture[i].w);
		textures->InsertEndChild(element);
	}
	root->InsertEndChild(textures);
}

void AnimExFormat::DumpBanks(tinyxml2::XMLDocument *document, tinyxml2::XMLElement *root)
{
	tinyxml2::XMLElement *banks = document->NewElement("banks");
	for (uint32_t i = 0; i < header.frame_start_cnt; i++) {
		tinyxml2::XMLElement *bank = document->NewElement("bank");
		std::string name = "bank" + std::to_string(i);
		bank->SetAttribute("name", name.c_str());
		bank->SetAttribute("frame_start", data.frame_start_list[i]);
		banks->InsertEndChild(bank);
	}
	root->InsertEndChild(banks);
}

void AnimExFormat::Dump(std::string dir, std::string name)
{
	tinyxml2::XMLDocument document;
	tinyxml2::XMLElement *root = document.NewElement("animex");
	document.InsertFirstChild(root);
	root->SetAttribute("length", header.frame_cnt);
	DumpNode(&document, root, data.root);
	DumpTracks(&document, root);
	DumpTextures(dir, name, &document, root);
	DumpBanks(&document, root);
	std::string out_name = dir + "\\" + name + ".xml";
	document.SaveFile(out_name.c_str());
}