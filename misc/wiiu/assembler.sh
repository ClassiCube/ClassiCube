ASSEMBLER=/home/minty/emulators/decaf-emu-ubuntu-20.04/bin/latte-assembler

assemble()
{
	echo "Assembling $1 from VS $2 + PS $3"
	$ASSEMBLER assemble $1 --vsh $2 --psh $3
} 

assemble coloured_none.gsh vs_coloured.vsh ps_coloured_none.psh

assemble textured_none.gsh vs_textured.vsh ps_textured_none.psh
assemble textured_lin.gsh  vs_textured.vsh ps_textured_lin.psh
assemble textured_exp.gsh  vs_textured.vsh ps_textured_exp.psh

assemble offset_none.gsh   vs_offset.vsh   ps_textured_none.psh
assemble offset_lin.gsh    vs_offset.vsh   ps_textured_lin.psh
assemble offset_exp.gsh    vs_offset.vsh   ps_textured_exp.psh
