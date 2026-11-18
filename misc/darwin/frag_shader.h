float4 fragment FS_NAME(
#if FS_FLAGS & FS_TEXTURED
						struct textured_out in [[stage_in]]
#else
						struct coloured_out in [[stage_in]]
#endif
#if FS_FLAGS & (FS_FOG_LIN | FS_FOG_EXP | FS_FOG_EXP2)
						,device const float2& fogValue [[buffer(1)]]
                        ,device const float3& fogColor [[buffer(2)]]
#endif
#if FS_FLAGS & FS_TEXTURED
						,texture2d<float, access::sample> tex [[texture(0)]]
						,sampler s [[sampler(0)]]
#endif
)
{
#if FS_FLAGS & FS_TEXTURED
    float4 color = in.color * tex.sample(s, in.texcoords).rgba;
#else
    float4 color = in.color;
#endif

#if FS_FLAGS & FS_ALPHA_TEST
    if (color.a < 0.5f) discard_fragment();
#endif

#if FS_FLAGS & FS_FOG_LIN
	float depth = 1.0f / in.position.w;
	float fog   = saturate(depth * fogValue.x);
    color.rgb   = mix(color.rgb, fogColor, fog);
#endif
#if FS_FLAGS & FS_FOG_EXP
	float depth = 1.0f / in.position.w;
	float fog   = saturate(exp(depth * fogValue.y));
	color.rgb   = mix(fogColor, color.rgb, fog);
#endif
#if FS_FLAGS & FS_FOG_EXP2
	float depth = 1.0f / in.position.w;
	float fog   = saturate(exp(-(fogValue.y * depth) * (fogValue.y * depth)));
	color.rgb   = mix(fogColor, color.rgb, fog);
#endif
	return color;
}
