attribute vec3 in_pos;
attribute vec4 in_col;
varying vec4 out_col;
uniform mat4 mvp;

void main() {
  gl_Position = mvp * vec4(in_pos, 1.0);
  out_col = in_col;
}
