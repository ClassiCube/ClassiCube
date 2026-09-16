//
//  File.metal
//  ClassiCube
//
//  Created by Admin on 16/9/2026.
//  Copyright © 2026 ClassiCube. All rights reserved.
//

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

vertex coloured_out vertex_coloured_main(coloured_in in [[stage_in]],
                                         device const float4x4& mvp [[buffer(1)]])
{
    coloured_out out;
    out.position  = mvp * float4(in.position, 1.0);
    out.color     = in.color;
    return out;
}


// == FRAGMENT SHADERS ==
float4 fragment fragment_textured_main(struct textured_out in [[stage_in]], texture2d<float, access::sample> tex [[texture(0)]])
{
    constexpr sampler s(address::repeat, filter::nearest);
    float4 texel = tex.sample(s, in.texcoords).rgba;
    return texel * in.color;
}

float4 fragment fragment_textured_main_at(struct textured_out in [[stage_in]], texture2d<float, access::sample> tex [[texture(0)]])
{
    constexpr sampler s(address::repeat, filter::nearest);
    float4 texel = tex.sample(s, in.texcoords).rgba;
    float4 color = texel * in.color;
    
    if (color.a < 0.5f) discard_fragment();
    return color;
}

float4 fragment fragment_coloured_main(struct coloured_out in [[stage_in]])
{
    return in.color;
}

float4 fragment fragment_coloured_main_at(struct coloured_out in [[stage_in]])
{
    if (in.color.a < 0.5f) discard_fragment();
    return in.color;
}
