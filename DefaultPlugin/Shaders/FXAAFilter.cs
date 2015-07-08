using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace DefaultPlugin.Shaders {
	
	public sealed class FXAAFilter : PostProcessingShader {
		
		public FXAAFilter() : base() {
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
out vec4 final_colour;
uniform sampler2D texScene;
uniform vec2 invViewport;

#define FXAA_REDUCE_MIN (1.0 / 128.0)
#define FXAA_REDUCE_MUL (1.0 / 8.0)
#define FXAA_SPAN_MAX 8.0

void main() {
	vec2 fragCoord = out_texcoords / invViewport;
    vec2 inverseVP = invViewport;
    vec3 rgbNW = texture2D(texScene, (fragCoord + vec2(-1.0, -1.0)) * inverseVP).xyz;
    vec3 rgbNE = texture2D(texScene, (fragCoord + vec2(1.0, -1.0)) * inverseVP).xyz;
    vec3 rgbSW = texture2D(texScene, (fragCoord + vec2(-1.0, 1.0)) * inverseVP).xyz;
    vec3 rgbSE = texture2D(texScene, (fragCoord + vec2(1.0, 1.0)) * inverseVP).xyz;
    vec3 rgbM  = texture2D(texScene, fragCoord  * inverseVP).xyz;
    
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir = vec2(
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * inverseVP;
      
    vec3 rgbA = 0.5 * (
        texture2D(texScene, fragCoord * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture2D(texScene, fragCoord * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture2D(texScene, fragCoord * inverseVP + dir * -0.5).xyz +
        texture2D(texScene, fragCoord * inverseVP + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        final_colour = vec4(rgbA, 1.0);
    else
        final_colour = vec4(rgbB, 1.0);
}
";
		}
		
		int viewportLoc;
		protected override void GetLocations() {
			base.GetLocations();
			viewportLoc = GetUniformLocation( "invViewport" );
		}
		
		int width = 640, height = 480;
		protected override void EnableVertexAttribStates( int stride ) {
			base.EnableVertexAttribStates( stride );
			Vector2 invViewport = new Vector2( 1f / width, 1f / height );
			Console.WriteLine( invViewport );
			SetUniform( viewportLoc, ref invViewport );
		}
		
		protected override void RenderTargetResized( int newWidth, int newHeight ) {
			width = newWidth;
			height = newHeight;
		}
	}
}