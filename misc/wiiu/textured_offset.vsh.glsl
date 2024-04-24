attribute vec3 in_pos;
attribute vec4 in_col;
attribute vec2 in_uv;
varying vec4 out_col;
varying vec2 out_uv;
uniform mat4 mvp;
uniform vec2 texOffset;

void main() {
  gl_Position = mvp * vec4(in_pos, 1.0);
  out_col = in_col;
  out_uv  = in_uv + texOffset;
}