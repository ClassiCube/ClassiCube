ASSEMBLER=/home/minty/emulators/decaf-emu-ubuntu-20.04/bin/latte-assembler

assemble()
{
	echo "Assembling $1 from VS $2 + PS $3"
	$ASSEMBLER assemble $1 --vsh $2 --psh $3
} 

assemble coloured_none.gsh vs_coloured.vsh        ps_coloured_none.psh
assemble textured_none.gsh vs_textured.vsh        ps_textured_none.psh
assemble textured_ofst.gsh vs_textured_offset.vsh ps_textured_none.psh
