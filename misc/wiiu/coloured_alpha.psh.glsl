varying vec4 out_col;

void main() {
  vec4 col = out_col;
  if (col.a < 0.5) discard;
  gl_FragColor = col;
}