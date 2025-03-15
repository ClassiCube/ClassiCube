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
static char* ccStrings_optionsMenu[CC_LANGUAGE_LANGCNT][20] = {
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
	"Game menu",
	"ON",
	"OFF"
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
	"Men£ del juego",
	"En",
	"Apagado"
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
	"lipu musi",
	"wile",
	"ala"
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
	"SpielmenÅ",
	"An",
	"Aus"
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

static char* ccString_Desc[CC_LANGUAGE_LANGCNT][2] = {
	{"&eSets the game's language",
	 "???"
	},

	{"&eEstablece el idioma del juego.",
	 "???"
	},

	{"&eante e musi toki",
	 "???"
	},

	{"&eLegt die Sprache des Spiels fest",
	 "???"
	},
};

static char* ccString_SubOption_Misc[CC_LANGUAGE_LANGCNT][11] = {
	{
	"Language",
	"Reach distance",
	"Camera Mass",
	"Music volume",
	"Sounds volume",
	"Block Physics",
	"Smooth Camera",
	"View bobbing",
	"Invert mouse",
	"Mouse sensitivity"
	},
	
	{
	"Idioma",
	"Distancia de alcance",
	"Masa de la c†mara",
	"Volumen de la m£sica",
	"Volumen de los sonidos",
	"F°sica de bloques",
	"Suavizar c†mara",
	"Vista de balanceo",
	"Invertir rat¢n",
	"Sensibilidad del rat¢n"
	},

	{
	"toki",
	"luka suli",
	"lukin suli",
	"kalama musi wawa",
	"kalama wawa",
	"leko tawa",
	"lukin pona",
	"lukin tawa",
	"lukin pi sewi ala",
	"lukin wawa"
	},

	{
	"Sprache",
	"Reichweite",
	"Kameramasse",
	"MusiklautstÑrke",
	"SoundlautstÑrke",
	"Blockphysik",
	"Kamera ruckelfrei",
	"Ansichtsbewegung",
	"Maus invertieren",
	"Mausempfindlichkeit"
	},
};

static char* ccString_SubOption_GUI[CC_LANGUAGE_LANGCNT][8] = {
	{
	"Show FPS",
	"Hotbar scale",
	"Inventory scale",
	"Crosshair scale",
	"Black text shadows",
	"Tab auto-complete",
	"Use system font",
	"Select system font"
	},
	
	{
	"Mostrar FPS",
	"Escala de la barra de objetos",
	"Escala del inventario",
	"Escala de la cruz",
	"Sombras de texto negras",
	"Autocompletar pesta§as",
	"Usar fuente del sistema",
	"Seleccionar fuente del sistema"
	},

	{
	"ken sitelen FPS",
	"suli mute ijo lili",
	"suli mute ijo",
	"suli ilo lukin",
	"pimeja sitelen",
	"sitelen pona pi nena Tab",
	"kepeken sitelen Sisen",
	"seme sitelen Sisen"
	},

	{
	"FPS anzeigen",
	"Artikelleistenskala",
	"Inventar-Skala",
	"Fadenkreuzskala",
	"Schwarze Textschatten",
	"Tab-AutovervollstÑndigung",
	"Systemschriftart verwenden",
	"Systemschriftart auswÑhlen"
	},
};

#endif