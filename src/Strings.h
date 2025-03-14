#ifndef CC_STRINGS_H
#define CC_STRINGS_H
CC_BEGIN_HEADER

/* 
Translated menu strings
   Also provides conversions betweens strings and numbers
   Also provides converting code page 437 indices to/from unicode
   Also provides wrapping a single line of text into multiple lines
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

/* Can be removed if causes issues */

#define CC_LANGUAGE_ENGLISH 0
#define CC_LANGUAGE_SPANISH 1
#define CC_LANGUAGE_TOKIPON 2

/* TODO (if this makes it into the CC Codebase: convince the other devs to make all UI text use strings that use longs/shorts (longs perfered) for storing text.) */
const char* ccStrings_optionsMenu[][8] = {
	{
	"Misc options...",
	"Gui options...",
	"Graphics options...",
	"Controls...",
	"Chat options...",
	"Hacks settings...",
	"Env settings...",
	"Nostalgia options..."
	},

	{
	"Opciones varias...",
	"Opciones de Gui...",
	"Opciones de gráficos...",
	"Opciones de controles...",
	"Opciones de charlar...",
	"Opciones de cortar...",
	"Opciones de ambientales...",
	"Opciones de nostalgia..."
	},

	{
	"ante e nasin pi nasin ala...",
	"ante e nasin pi ilo sitelen...",
	"ante e nasin pi sitelen...",
	"ante e nasin pi ilo luka...",
	"ante e nasin toki...",
	"ante e nasin pi jan wawa...",
	"ante e nasin pi ma lukin...",
	"ante e nasin pi tenpo pona pi sin ala..."
	},
};

CC_END_HEADER
#endif