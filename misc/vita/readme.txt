Shaders on the PS Vita use the proprietary .gxp binary format

To compile CG shaders to .gxp you need to use
1) Extract libshacccg.suprx - see https://cimmerian.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx
2) Copy libshacccg.suprx to ur0:/data/libshacccg.suprx (or change path in shader_compiler.c)
3) Copy shaders to ux0:/data/shaders (or change path in shader_compiler.c)
4) Run make_shader_compiler to create shader_compiler.vpk
5) Run shader_compiler.vpk (note that shaders can be changed without needing to recompile)

You can then use include the corresponding .h in src/Graphics_PSVita.c (autobuilt by makefile)

Note that you only need to perform these steps if you want to compile modified shaders - you don't need libshacccg.suprx to run ClassiCube
