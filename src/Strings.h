#pragma once

#ifndef CC_STRINGS_H
#define CC_STRINGS_H

/* 
Translated menu strings
   Also provides conversions betweens strings and numbers
   Also provides converting code page 437 indices to/from unicode
   Also provides wrapping a single line of text into multiple lines
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

/* Can be removed if causes issues */

#define CC_LANGUAGE_LANGCNT 4

#define CC_LANGUAGE_ENGLISH 0
#define CC_LANGUAGE_SPANISH 1
#define CC_LANGUAGE_TOKIPON 2
#define CC_LANGUAGE_GERMAN_ 3

extern unsigned char CC_CurrentLanguage;

/* TODO (if this makes it into the CC Codebase: convince the other devs to make all UI text use strings that use longs/shorts (longs perfered) for storing text.) */
static char* ccStrings_optionsMenu[CC_LANGUAGE_LANGCNT][18] = {
	{
	"Misc options...",
	"Gui options...",
	"Graphics options...",
	"Controls...",
	"Chat options...",
	"Hacks settings...",
	"Env settings...",
	"Nostalgia options...",

	"Options...",
	"Change texture pack...",
	"Hotkeys...",
	"Generate new level...",
	"Load level...",
	"Save level...",

	"Done",
	"Quit game",
	"Back to game",
	"Game menu"
	},

	{
	"Opciones varias...",
	"Opciones de Gui...",
	"Opciones de gr†ficos...",
	"Opciones de controles...",
	"Opciones de charlar...",
	"Opciones de cortar...",
	"Opciones de ambientales...",
	"Opciones de nostalgia...",

	"Opciones...",
	"Paquetes de texturas...",
	"Teclas de acceso r†pido...",
	"Generar nuevo nivel...",
	"Nivel de carga...",
	"Guardar nivel...",

	"Hecho",
	"Salir del juego",
	"De vuelta al juego",
	"Men£ del juego"
	},

	{
	"ante e nasin pi nasin ala...",
	"ante e nasin pi ilo sitelen...",
	"ante e nasin pi sitelen...",
	"ante e nasin pi ilo luka...",
	"ante e nasin toki...",
	"ante e nasin pi jan wawa...",
	"ante e nasin pi ma lukin...",
	"ante e nasin pi tenpo pona...",

	"ante e musi...",
	"ante e poki sitelen...",
	"ante e nena wawa...",
	"o pali ma sin...",
	"ante e ma...",
	"o awen e ma...",

	"ale pona",
	"o weka",
	"o kama musi",
	"lipu musi"
	},

	{
	"Verschiedene Optionen...",
	"GUI-Optionen...",
	"Grafikoptionen...",
	"Steuerung...",
	"Chat-Optionen...",
	"Hacks-Einstellungen...",
	"Umgebungseinstellungen...",
	"Nostalgie-Optionen...",

	"Optionen...",
	"Texturpaket Ñndern...",
	"Tastenkombinationen...",
	"Neues Level generieren...",
	"Level laden...",
	"Level speichern...",

	"Fertig",
	"Spiel beenden",
	"ZurÅck zum Spiel",
	"SpielmenÅ"
	},
};

static char* ccStrings_GameTitle[CC_LANGUAGE_LANGCNT] = {
	"ClassiCube",
	"ClassiCube (en Espa§ol)",
	"musi Kasiku",
	"ClassiCube (auf Deutsch)"
};

static char* csString_LanguageNames[CC_LANGUAGE_LANGCNT] = {
	"English",
	"Espa§ol",
	"toki pona (sitelen Lasina)",
	"Deutsch"
};

#endif