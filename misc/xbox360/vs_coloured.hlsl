struct INPUT_VERTEX 
{
	float3 pos : POSITION;
	float4 col : COLOR0;
};

struct OUTPUT_VERTEX
{ 
	float4 pos : POSITION;
	float4 col : TEXCOORD0;
};

float4x4 mvpMatrix: register(c0);

OUTPUT_VERTEX main(INPUT_VERTEX input) {
   OUTPUT_VERTEX output;
   output.pos = mul(mvpMatrix, float4(input.pos, 1.0f));
   output.col = input.col;
   return output;
}