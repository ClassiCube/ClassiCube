varying vec4 out_col;
varying vec2 out_uv;

uniform sampler2D texImage;
uniform vec4 fogVal;
// fogVal.rgb = fog colour
// fogVal.a   = fog density

void main() {
  vec4 col = texture(texImage, out_uv) * out_col;

  float depth = 1.0 / gl_FragCoord.w;
  float f = clamp(exp2(fogVal.a * depth), 0.0, 1.0);

  col.rgb = mix(fogVal.rgb, col.rgb, f);
  gl_FragColor = col;
}
