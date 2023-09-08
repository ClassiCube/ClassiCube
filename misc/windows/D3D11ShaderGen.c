#define _WIN32_WINNT 0x600
#define COBJMACROS
#include <stdio.h>
#include <d3dcompiler.h>

#define GetBlobPointer ID3D10Blob_GetBufferPointer
#define GetBlobSize    ID3D10Blob_GetBufferSize
#pragma comment(lib,"d3dcompiler.lib")

static const char VS_SOURCE[] =
"float4x4 mvpMatrix;                                                \n" \
"#ifdef VS_TEXTURE_OFFSET                                           \n" \
"float2 texOffset;                                                  \n" \
"#endif                                                             \n" \
"                                                                   \n" \
"struct INPUT_VERTEX {                                              \n" \
"   float3 position : POSITION;                                     \n" \
"   float4 color : COLOR0;                                          \n" \
"#ifndef VS_COLOR_ONLY                                              \n" \
"   float2 coords : TEXCOORD0;                                      \n" \
"#endif                                                             \n" \
"};                                                                 \n" \
"struct OUTPUT_VERTEX {                                             \n" \
"#ifndef VS_COLOR_ONLY                                              \n" \
"   float2 coords : TEXCOORD0;                                      \n" \
"#endif                                                             \n" \
"   float4 color : COLOR0;                                          \n" \
"   float4 position : VPOS;                                         \n" \
"};                                                                 \n" \
"                                                                   \n" \
"OUTPUT_VERTEX main(INPUT_VERTEX input) {                           \n" \
"   OUTPUT_VERTEX output;                                           \n" \
"   // https://stackoverflow.com/questions/16578765/hlsl-mul-variables-clarification \n" \
"   output.position = mul(mvpMatrix, float4(input.position, 1.0f)); \n" \
"#ifndef VS_COLOR_ONLY                                              \n" \
"   output.coords = input.coords;                                   \n" \
"#endif                                                             \n" \
"#ifdef VS_TEXTURE_OFFSET                                           \n" \
"   output.coords += texOffset;                                     \n" \
"#endif                                                             \n" \
"   output.color = input.color;                                     \n" \
"   return output;                                                  \n" \
"}";

static const char PS_SOURCE[] =
"#ifndef PS_COLOR_ONLY                                              \n" \
"Texture2D    texValue;                                             \n" \
"SamplerState texState;                                             \n" \
"#endif                                                             \n" \
"#ifdef PS_FOG_LINEAR                                               \n" \
"float  fogEnd;                                                     \n" \
"float3 fogColor;                                                   \n" \
"#endif                                                             \n" \
"#ifdef PS_FOG_DENSITY                                              \n" \
"float  fogDensity;                                                 \n" \
"float3 fogColor;                                                   \n" \
"#endif                                                             \n" \
"                                                                   \n" \
"struct INPUT_VERTEX {                                              \n" \
"#ifndef PS_COLOR_ONLY                                              \n" \
"   float2 coords : TEXCOORD0;                                      \n" \
"#endif                                                             \n" \
"   float4 color : COLOR0;                                          \n" \
"#ifdef PS_FOG_LINEAR                                               \n" \
"   float4 position : VPOS;                                         \n" \
"#endif                                                             \n" \
"#ifdef PS_FOG_DENSITY                                              \n" \
"   float4 position : VPOS;                                         \n" \
"#endif                                                             \n" \
"};                                                                 \n" \
"                                                                   \n" \
"//float4 main(float2 coords : TEXCOORD0, float4 color : COLOR0) : SV_TARGET {\n" \
"float4 main(INPUT_VERTEX input) : SV_TARGET{                       \n" \
"   float4 color;                                                   \n" \
"#ifdef PS_COLOR_ONLY                                               \n" \
"   color = input.color;                                            \n" \
"#else                                                              \n" \
"   float4 texColor = texValue.Sample(texState, input.coords);      \n" \
"   color = texColor * input.color;                                 \n" \
"#endif                                                             \n" \
"                                                                   \n" \
"#ifdef PS_ALPHA_TEST                                               \n" \
"   // https://jbrd.github.io/2008/02/14/hlsl-discard-doesnt-return.html \n" \
"   if (color.a < 0.5) { discard; return color; }                   \n" \
"#endif                                                             \n" \
"#ifdef PS_FOG_LINEAR                                               \n" \
"   float depth = input.position.w;                                 \n" \
"   float fog   = saturate((fogEnd - depth) / fogEnd);              \n" \
"   color.rgb   = lerp(fogColor, color.rgb, fog);                   \n" \
"#endif                                                             \n" \
"#ifdef PS_FOG_DENSITY                                              \n" \
"   float depth = input.position.w;                                 \n" \
"   float fog   = saturate(exp(fogDensity * depth));                \n" \
"   color.rgb   = lerp(fogColor, color.rgb, fog);                   \n" \
"#endif                                                             \n" \
"   return color;                                                   \n" \
"}";

static void CompileShader(LPCSTR src, LPCSTR name, LPCSTR profile, const D3D_SHADER_MACRO* defines) {
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL1;// | D3DCOMPILE_DEBUG;
    ID3DBlob* shaderBlob = NULL;
    ID3DBlob* errorBlob  = NULL;
    HRESULT hr = D3DCompile(src, strlen(src), name, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "main", profile, flags, 0, &shaderBlob, &errorBlob );

	if (hr && errorBlob) {
		char* errText = (char*)GetBlobPointer(errorBlob);
		OutputDebugStringA(errText);
		printf("// Compiling %s failed\n", name);
		printf("//   %s\n", errText);
	}
	if (hr) {
		printf("// Compiling %s failed - %08X\n", name, hr);
		return;
	}

	unsigned char* data = GetBlobPointer(shaderBlob);
	SIZE_T size         = GetBlobSize(shaderBlob);
	printf("static const unsigned char %s[%i] = {\n", name, size);

	for (int i = 0; i < size; )
	{
		int count = min(size - i, 32);
		printf("\t");
		for (int j = 0; j < count; j++)
		{
			printf("0x%02x,", data[i + j]);
		}
		printf("\n");
		i += count;
	}
	printf("};\n");
}

#define CompileVertexShader(name, defines) CompileShader(VS_SOURCE, name, "vs_4_0_level_9_1", defines)
#define CompilePixelShader( name, defines) CompileShader(PS_SOURCE, name, "ps_4_0_level_9_1", defines)
int main() {
	const D3D_SHADER_MACRO vs_colored[]  = { "VS_COLOR_ONLY","1",  NULL,NULL };
	const D3D_SHADER_MACRO vs_textured[] = {                       NULL,NULL };
	const D3D_SHADER_MACRO vs_offset[]   = { "VS_TEXTURE_OFFSET","1",  NULL,NULL };

	const D3D_SHADER_MACRO ps_colored[]       = { "PS_COLOR_ONLY","1",  NULL,NULL };
	const D3D_SHADER_MACRO ps_textured[]      = {                       NULL,NULL };
	const D3D_SHADER_MACRO ps_colored_test[]  = { "PS_COLOR_ONLY","1",  "PS_ALPHA_TEST","1",  NULL,NULL };
	const D3D_SHADER_MACRO ps_textured_test[] = {                       "PS_ALPHA_TEST","1",  NULL,NULL };

	const D3D_SHADER_MACRO ps_colored_linear[]       = { "PS_FOG_LINEAR","1",  "PS_COLOR_ONLY","1",  NULL,NULL };
	const D3D_SHADER_MACRO ps_textured_linear[]      = { "PS_FOG_LINEAR","1",                        NULL,NULL };
	const D3D_SHADER_MACRO ps_colored_test_linear[]  = { "PS_FOG_LINEAR","1",  "PS_COLOR_ONLY","1",  "PS_ALPHA_TEST","1",  NULL,NULL };
	const D3D_SHADER_MACRO ps_textured_test_linear[] = { "PS_FOG_LINEAR","1",                        "PS_ALPHA_TEST","1",  NULL,NULL };

	const D3D_SHADER_MACRO ps_colored_density[]       = { "PS_FOG_DENSITY","1",  "PS_COLOR_ONLY","1",  NULL,NULL };
	const D3D_SHADER_MACRO ps_textured_density[]      = { "PS_FOG_DENSITY","1",                        NULL,NULL };
	const D3D_SHADER_MACRO ps_colored_test_density[]  = { "PS_FOG_DENSITY","1",  "PS_COLOR_ONLY","1",  "PS_ALPHA_TEST","1",  NULL,NULL };
	const D3D_SHADER_MACRO ps_textured_test_density[] = { "PS_FOG_DENSITY","1",                        "PS_ALPHA_TEST","1",  NULL,NULL };

	printf("// Generated using misc/D3D11ShaderGen.c\n\n");
	printf("//########################################################################################################################\n");
	printf("//------------------------------------------------------Vertex shaders----------------------------------------------------\n");
	printf("//########################################################################################################################\n");
	CompileVertexShader("vs_colored",         vs_colored);
	CompileVertexShader("vs_textured",        vs_textured);
	CompileVertexShader("vs_textured_offset", vs_offset);

	printf("\n\n");
	printf("//########################################################################################################################\n");
	printf("//------------------------------------------------------Pixel shaders-----------------------------------------------------\n");
	printf("//########################################################################################################################\n");
	CompilePixelShader("ps_colored",       ps_colored);
	CompilePixelShader("ps_textured",      ps_textured);
	CompilePixelShader("ps_colored_test",  ps_colored_test);
	CompilePixelShader("ps_textured_test", ps_textured_test);

	CompilePixelShader("ps_colored_linear",       ps_colored_linear);
	CompilePixelShader("ps_textured_linear",      ps_textured_linear);
	CompilePixelShader("ps_colored_test_linear",  ps_colored_test_linear);
	CompilePixelShader("ps_textured_test_linear", ps_textured_test_linear);

	CompilePixelShader("ps_colored_density",       ps_colored_density);
	CompilePixelShader("ps_textured_density",      ps_textured_density);
	CompilePixelShader("ps_colored_test_density",  ps_colored_test_density);
	CompilePixelShader("ps_textured_test_density", ps_textured_test_density);

	//Sleep(5000);
	return 0;
}
