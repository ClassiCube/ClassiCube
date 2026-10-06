// TS - Texture shader:
//  - unused
// RC - Register combiners:
//  - final : out  = col0
!!RC1.0
out.rgb = lerp(col0.rgb, fog.rgb, fog.a);
out.a = unsigned(col0.a);
// 0 instructions
