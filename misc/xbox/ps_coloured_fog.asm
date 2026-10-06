// TS - Texture shader:
//  - unused
// RC - Register combiners:
//  - final : out  = col0
!!RC1.0
out.rgb = lerp(fog.a, col0.rgb, fog.rgb);
out.a = unsigned(col0.a);
// 0 instructions
