using System;
using ClassicalSharp.GraphicsAPI;

namespace DefaultPlugin.Shaders {
	
	public sealed class GrayscaleFilter : PostProcessingShader {
		
		public GrayscaleFilter() : base() {
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
out vec4 final_colour;
uniform sampler2D texScene;

void main() {
  vec2 uv = out_texcoords;
  vec4 col = texture2D(texScene, uv).rgba;
  float intensity = 0.299 * col.r + 0.587 * col.g + 0.114 * col.b;
  final_colour = vec4(intensity, intensity, intensity, 1);
}";
		}
		
		protected override void RenderTargetResized( int newWidth, int newHeight ) {
			
		}
	}
}