sampler s;

struct INPUT_VERTEX {
   float4 col : COLOR0;
   float2 uv  : TEXCOORD0;
};

float4 main(INPUT_VERTEX input) : COLOR0 {
	return tex2D(s, input.uv) * input.col;
}