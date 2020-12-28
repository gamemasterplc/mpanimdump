#pragma once

#include <vector>
#include "AnimFormat.h"
#include "mpanimdump.h"
#include "tinyxml2.h"

#define ANIMEX_TEX_FORMAT_RGBA8 0
#define ANIMEX_TEX_FORMAT_RGB5A3 1
#define ANIMEX_TEX_FORMAT_RGB5A3_DUPE 2
#define ANIMEX_TEX_FORMAT_CI8 3
#define ANIMEX_TEX_FORMAT_CI4 4
#define ANIMEX_TEX_FORMAT_IA8 5
#define ANIMEX_TEX_FORMAT_IA4 6
#define ANIMEX_TEX_FORMAT_I8 7
#define ANIMEX_TEX_FORMAT_I4 8
#define ANIMEX_TEX_FORMAT_A8 9
#define ANIMEX_TEX_FORMAT_CMPR 10
#define ANIMEX_TEX_FORMAT_COUNT 11

#define ANIMEX_NODE_TYPE_ROOT 0
#define ANIMEX_NODE_TYPE_TRANSFORM 2
#define ANIMEX_NODE_TYPE_IMAGE 3

#define ANIMEX_INTERP_MODE_NONE 11
#define ANIMEX_INTERP_MODE_LINEAR 12
#define ANIMEX_INTERP_MODE_SPLINE 13

#define ANIMEX_TRACK_POS 0
#define ANIMEX_TRACK_ROTATE 1
#define ANIMEX_TRACK_SCALE 2
#define ANIMEX_TRACK_COLOR 3

#define ANIMEX_TRACK_VAR_X 4
#define ANIMEX_TRACK_VAR_Y 5
#define ANIMEX_TRACK_VAR_Z 6
#define ANIMEX_TRACK_VAR_R 7
#define ANIMEX_TRACK_VAR_G 8
#define ANIMEX_TRACK_VAR_B 9
#define ANIMEX_TRACK_VAR_A 10

typedef struct animex_texture {
	uint8_t bpp;
	uint8_t format;
	int16_t palette_len;
	int16_t w;
	int16_t h;
	uint32_t data_len;
	uint8_t *data;
	uint8_t *pal_data;
} AnimExTexture;

typedef struct animex_file_texture {
	uint8_t bpp;
	uint8_t format;
	int16_t palette_len;
	int16_t w;
	int16_t h;
	uint32_t data_len;
	uint32_t data_ofs;
	uint32_t pal_data_ofs;
} AnimExFileTexture;

typedef struct animex_node {
	int16_t type;
	uint16_t child_count;
	struct animex_node **children;
} AnimExNode;

typedef struct animex_file_node {
	int16_t type;
	uint16_t child_count;
	uint32_t children_ofs;
} AnimExFileNode;

typedef struct animex_transform {
	AnimExNode node;
	float scale_x;
	float scale_y;
	float scale_z;
	float rot_x;
	float rot_y;
	float rot_z;
	float pos_x;
	float pos_y;
	float pos_z;
} AnimExTransform;

typedef struct animex_file_transform {
	AnimExFileNode node;
	float scale_x;
	float scale_y;
	float scale_z;
	float rot_x;
	float rot_y;
	float rot_z;
	float pos_x;
	float pos_y;
	float pos_z;
} AnimExFileTransform;

typedef struct animex_image {
	AnimExNode node;
	char *name;
	float vertices[12];
	float uv[8];
	float color[4];
	AnimExTexture *texture;
} AnimExImage;

typedef struct animex_file_image {
	AnimExFileNode node;
	uint32_t name_ofs;
	float vertices[12];
	float uv[8];
	float color[4];
	uint32_t texture_ofs;
} AnimExFileImage;

typedef struct animex_keyframe {
	uint32_t interp_type;
	uint32_t frame_num;
	float points[4];
} AnimExKeyframe;

typedef struct animex_track {
	int16_t node_type;
	uint16_t node_id;
	uint16_t track_type;
	uint16_t var_id;
	uint32_t num_keyframes;
	AnimExKeyframe *keyframe;
} AnimExTrack;

typedef struct animex_file_track {
	int16_t node_type;
	uint16_t node_id;
	uint16_t track_type;
	uint16_t var_id;
	uint32_t num_keyframes;
	uint32_t keyframe_ofs;
} AnimExFileTrack;

typedef struct animex_data {
	AnimExNode *root;
	AnimExNode *type1;
	AnimExTransform *transform;
	AnimExImage *image;
	AnimExTrack *track;
	AnimExKeyframe *keyframe;
	AnimExTexture *texture;
	AnimExNode **node_ref_list;
	uint32_t *frame_start_list;
	char *str_table;
} AnimExData;

typedef struct animex_header {
	uint32_t magic;
	uint32_t frame_cnt;
	uint32_t ref_cnt;
	uint32_t root_cnt;
	uint32_t type1_cnt;
	uint32_t transform_cnt;
	uint32_t image_cnt;
	uint32_t track_cnt;
	uint32_t keyframe_cnt;
	uint32_t texture_cnt;
	uint32_t node_ref_cnt;
	uint32_t frame_start_cnt;
	uint32_t str_table_len;
	uint32_t root_ofs;
	uint32_t type1_ofs;
	uint32_t transform_ofs;
	uint32_t image_ofs;
	uint32_t track_ofs;
	uint32_t keyframe_ofs;
	uint32_t texture_ofs;
	uint32_t node_ref_ofs;
	uint32_t frame_start_ofs;
	uint32_t str_table_ofs;
} AnimExHeader;

class AnimExFormat : public AnimFormat
{
public:
	AnimExFormat(FILE *file);

public:
	virtual ~AnimExFormat();
	virtual void Dump(std::string dir, std::string name);

private:
	AnimExHeader header;
	AnimExData data;

private:
	void ReadHeader(FILE *file);
	void ReadNode(FILE *file, AnimExNode *node);
	void ReadTransforms(FILE *file);
	void ReadImages(FILE *file);
	void ReadKeyframes(FILE *file);
	void ReadTracks(FILE *file);
	void ReadTextures(FILE *file);
	void ReadNodeRefs(FILE *file);
	void ReadFrameStartList(FILE *file);
	void ReadStringTable(FILE *file);
	void ReadData(FILE *file);
	void FixupNode(AnimExNode *node, uint32_t file_ofs, FILE *file);
	void FixupNodes(FILE *file);
	void DumpNode(tinyxml2::XMLDocument *document, tinyxml2::XMLElement *parent, AnimExNode *node);
	void DumpTransformNode(tinyxml2::XMLElement *xml_node, AnimExTransform *node);
	void DumpImageNode(tinyxml2::XMLElement *xml_node, AnimExImage *node);
	void DumpTracks(tinyxml2::XMLDocument *document, tinyxml2::XMLElement *root);
	void DumpTextures(std::string dir, std::string name, tinyxml2::XMLDocument *document, tinyxml2::XMLElement *root);
	void DumpBanks(tinyxml2::XMLDocument *document, tinyxml2::XMLElement *root);
};
