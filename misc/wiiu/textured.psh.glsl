varying vec4 out_col;
varying vec2 out_uv;
uniform sampler2D texImage;

void main() {
  vec4 col = texture(texImage, out_uv) * out_col;
  gl_FragColor = col;
}

