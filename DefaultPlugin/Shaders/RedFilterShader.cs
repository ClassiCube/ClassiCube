using System;
using ClassicalSharp.GraphicsAPI;

namespace DefaultPlugin.Shaders {
	
	public sealed class RedFilter : PostProcessingShader {
		
		public RedFilter() : base() {
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
out vec4 final_colour;
uniform sampler2D texScene;

void main() {
  vec2 uv = out_texcoords;
  final_colour = texture2D(texScene, uv);
  final_colour.g = 0;
  final_colour.b = 0;
}";
		}
		
		protected override void RenderTargetResized( int newWidth, int newHeight ) {
			
		}
	}
}