varying vec4 out_col;
varying vec2 out_uv;
uniform sampler2D texImage;
uniform vec3 fogCol;
uniform float fogDensity;

void main() {
  vec4 col = texture(texImage, out_uv) * out_col;
  float depth = 1.0 / gl_FragCoord.w;
  float f = clamp(exp2(fogDensity * depth), 0.0, 1.0);
 col.rgb = mix(fogCol, col.rgb, f);
  gl_FragColor = col;
}
