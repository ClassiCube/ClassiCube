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
"	float3 position : POSITION;                                     \n" \
"	float4 color : COLOR0;                                          \n" \
"#ifndef VS_COLOR_ONLY                                              \n" \
"	float2 coords : TEXCOORD0;                                      \n" \
"#endif                                                             \n" \
"};                                                                 \n" \
"struct OUTPUT_VERTEX {                                             \n" \
"#ifndef VS_COLOR_ONLY                                              \n" \
"	float2 coords : TEXCOORD0;                                      \n" \
"#endif                                                             \n" \
"	float4 color : COLOR0;                                          \n" \
"	float4 position : SV_POSITION;                                  \n" \
"};                                                                 \n" \
"                                                                   \n" \
"OUTPUT_VERTEX main(INPUT_VERTEX input) {                           \n" \
"	OUTPUT_VERTEX output;                                           \n" \
"   // https://stackoverflow.com/questions/16578765/hlsl-mul-variables-clarification \n" \
"	output.position = mul(mvpMatrix, float4(input.position, 1.0f)); \n" \
"#ifndef VS_COLOR_ONLY                                              \n" \
"	output.coords = input.coords;                                   \n" \
"#endif                                                             \n" \
"#ifdef VS_TEXTURE_OFFSET                                           \n" \
"	output.coords += texOffset;                                     \n" \
"#endif                                                             \n" \
"	output.color = input.color;                                     \n" \
"	return output;                                                  \n" \
"}";

static const char PS_SOURCE[] =
"#ifndef PS_COLOR_ONLY                                              \n" \
"Texture2D    texValue;                                             \n" \
"SamplerState texState;                                             \n" \
"#endif                                                             \n" \
"                                                                   \n" \
"struct INPUT_VERTEX {                                              \n" \
"#ifndef PS_COLOR_ONLY                                              \n" \
"	float2 coords : TEXCOORD0;                                      \n" \
"#endif                                                             \n" \
"	float4 color : COLOR0;                                          \n" \
"};                                                                 \n" \
"                                                                   \n" \
"//float4 main(float2 coords : TEXCOORD0, float4 color : COLOR0) : SV_TARGET {\n" \
"float4 main(INPUT_VERTEX input) : SV_TARGET{                       \n" \
"#ifdef PS_COLOR_ONLY                                               \n" \
"	return input.color;                                             \n" \
"#else                                                              \n" \
"	float4 texColor = texValue.Sample(texState, input.coords);      \n" \
"   return texColor * input.color;                                  \n" \
"#endif                                                             \n" \
"}";

static HRESULT CompileShader(LPCSTR src, LPCSTR srcName, LPCSTR profile, ID3DBlob** blob )
{
    *blob = NULL;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;// | D3DCOMPILE_DEBUG;
    const D3D_SHADER_MACRO defines[] =  { "PS_COLOR_ONLY","1",  NULL,NULL};

    ID3DBlob* shaderBlob = NULL;
    ID3DBlob* errorBlob  = NULL;
    HRESULT hr = D3DCompile(src, strlen(src), srcName, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            "main", profile, flags, 0, &shaderBlob, &errorBlob );

	if (hr && errorBlob) {
		OutputDebugStringA(GetBlobPointer(errorBlob));
	}

	*blob = shaderBlob;
    return hr;
}

static void OutputShader(HRESULT hr, ID3DBlob* blob, const char* name) {
	if (hr) {
		printf("Failed compiling %s shader %08X\n", name, hr);
		return;
	}

	unsigned char* data = GetBlobPointer(blob);
	SIZE_T size         = GetBlobSize(blob);
	printf("static unsigned char %s_shader[%i] = {\n", name, size);

	for (int i = 0; i < size; )
	{
		int count = min(size - i, 16);
		for (int j = 0; j < count; j++)
		{
			printf("0x%02x,", data[i + j]);
		}
		//printf("\n");
		i += count;
	}
	printf("};\n");
}

int main() {
	HRESULT hr;
    ID3DBlob* blob = NULL;

    hr = CompileShader(VS_SOURCE, "vs_shader.hlsl", "vs_4_0_level_9_1", &blob);
	OutputShader(hr, blob, "vs");

    hr = CompileShader(PS_SOURCE, "ps_shader.hlsl", "ps_4_0_level_9_1", &blob);
	OutputShader(hr, blob, "ps");

    return 0;
}

