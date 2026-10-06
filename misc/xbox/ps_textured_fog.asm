// TS - Texture shader:
//  - lookup texture0 at tex0(out_tex) coordinates
// RC - Register combiners:
//  - stage1: col0 = col0(out_col) * tex0 (lookuped value for tex0)
//  - final : out  = col0
!!TS1.0
texture_2d();
// End of program
!!RC1.0
{
  rgb
  {
    col0 = tex0.rgb * col0.rgb;
  }
  alpha
  {
    col0 = tex0.a * col0.a;
  }
}
out.rgb = lerp(col0.rgb, fog.rgb, fog.a);
out.a = unsigned(col0.a);
// 3 instructions
