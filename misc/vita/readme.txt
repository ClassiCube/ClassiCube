Shaders on the PS Vita use the proprietary .gxp binary format

To compile CG shaders to .gxp you need to use
1) Extract libshacccg.suprx - see https://cimmerian.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx
2) Use vitaShaRK to compile the CG shader (see sample 3 in https://github.com/Rinnegatamante/vitaShaRK for some reference)

You can then use compiled gxp_to_c to convert all the .gxp shaders into .h files, which src/Graphics_PSVita.c can then include

Note that you only need to perform these steps if you want to compile modified shaders - you don't need libshacccg.suprx to run ClassiCube