#include <metal_stdlib>
using namespace metal;

struct coloured_in {
    float3 position  [[attribute(0)]];
    float4 color     [[attribute(1)]];
};
struct textured_in {
    float3 position  [[attribute(0)]];
    float4 color     [[attribute(1)]];
    float2 texcoords [[attribute(2)]];
};

struct coloured_out {
    float4 position  [[position]];
    float4 color;
};
struct textured_out {
    float4 position  [[position]];
    float4 color;
    float2 texcoords;
};	


// == VERTEX SHADERS ==
vertex textured_out vertex_textured_main(textured_in in [[stage_in]],
                                         device const float4x4& mvp [[buffer(1)]])
{
    textured_out out;
    out.position  = mvp * float4(in.position, 1.0);
    out.color     = in.color;
    out.texcoords = in.texcoords;
    return out;
}

vertex textured_out vertex_textured_main_offset(textured_in in [[stage_in]],
                                         device const float4x4& mvp [[buffer(1)]],
                                         device const float2& texOffset [[buffer(2)]])
{
    textured_out out;
    out.position  = mvp * float4(in.position, 1.0);
    out.color     = in.color;
    out.texcoords = in.texcoords + texOffset;
    return out;
}

vertex coloured_out vertex_coloured_main(coloured_in in [[stage_in]],
                                         device const float4x4& mvp [[buffer(1)]])
{
    coloured_out out;
    out.position  = mvp * float4(in.position, 1.0);
    out.color     = in.color;
    return out;
}


// == FRAGMENT SHADERS ==
#define FS_TEXTURED   (1 << 0)
#define FS_ALPHA_TEST (1 << 1)
#define FS_FOG_LIN    (1 << 2)
#define FS_FOG_EXP    (1 << 3)
#define FS_FOG_EXP2   (1 << 4)

#define FS_NAME  fragment_textured_none_main
#define FS_FLAGS (FS_TEXTURED)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_textured_lin_main
#define FS_FLAGS (FS_TEXTURED | FS_FOG_LIN)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_textured_exp_main
#define FS_FLAGS (FS_TEXTURED | FS_FOG_EXP)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_textured_exp2_main
#define FS_FLAGS (FS_TEXTURED | FS_FOG_EXP2)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME


#define FS_NAME  fragment_textured_none_main_at
#define FS_FLAGS (FS_TEXTURED | FS_ALPHA_TEST)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_textured_lin_main_at
#define FS_FLAGS (FS_TEXTURED | FS_ALPHA_TEST | FS_FOG_LIN)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_textured_exp_main_at
#define FS_FLAGS (FS_TEXTURED | FS_ALPHA_TEST | FS_FOG_EXP)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_textured_exp2_main_at
#define FS_FLAGS (FS_TEXTURED | FS_ALPHA_TEST | FS_FOG_EXP2)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME


#define FS_NAME  fragment_coloured_none_main
#define FS_FLAGS (0)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_coloured_lin_main
#define FS_FLAGS (0 | FS_FOG_LIN)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_coloured_exp_main
#define FS_FLAGS (0 | FS_FOG_EXP)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_coloured_exp2_main
#define FS_FLAGS (0 | FS_FOG_EXP2)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME


#define FS_NAME  fragment_coloured_none_main_at
#define FS_FLAGS (0 | FS_ALPHA_TEST)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_coloured_lin_main_at
#define FS_FLAGS (0 | FS_ALPHA_TEST | FS_FOG_LIN)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_coloured_exp_main_at
#define FS_FLAGS (0 | FS_ALPHA_TEST | FS_FOG_EXP)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME

#define FS_NAME  fragment_coloured_exp2_main_at
#define FS_FLAGS (0 | FS_ALPHA_TEST | FS_FOG_EXP2)
	#include "frag_shader.h"
#undef FS_FLAGS
#undef FS_NAME
