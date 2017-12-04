#ifndef CC_GFXENUMS_H
#define CC_GFXENUMS_H

/* Vertex data format types*/
#define VERTEX_FORMAT_P3FC4B 0
#define VERTEX_FORMAT_P3FT2FC4B 1

/* 3D graphics pixel comparison functions */
#define COMPARE_FUNC_ALWAYS 0
#define COMPARE_FUNC_NOTEQUAL 1
#define COMPARE_FUNC_NEVER 2
#define COMPARE_FUNC_LESS 3
#define COMPARE_FUNC_LESSEQUAL 4
#define COMPARE_FUNC_EQUAL 5
#define COMPARE_FUNC_GREATEREQUAL 6
#define COMPARE_FUNC_GREATER 7

/* 3D graphics pixel blending functions */
#define BLEND_FUNC_ZERO 0
#define BLEND_FUNC_ONE 1
#define BLEND_FUNC_SRC_ALPHA 2
#define BLEND_FUNC_INV_SRC_ALPHA 3
#define BLEND_FUNC_DST_ALPHA 4
#define BLEND_FUNC_INV_DST_ALPHA 5

/* 3D graphics pixel fog blending functions */
#define FOG_LINEAR 0
#define FOG_EXP 1
#define FOG_EXP2 2

/* 3D graphics matrix types */
#define MATRIX_TYPE_PROJECTION 0
#define MATRIX_TYPE_MODELVIEW 1
#define MATRIX_TYPE_TEXTURE 2
#endif