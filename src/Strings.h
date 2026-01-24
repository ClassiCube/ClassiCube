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

/*

	OPEN AND SAVE WITH DOS/CP-437 ENCODING.
		NOT DOING SO WILL BREAK TEXT.

*/

#define CC_LANGUAGE_LANGCNT 5

#define CC_LANGUAGE_ENGLISH 0
#define CC_LANGUAGE_SPANISH 1
#define CC_LANGUAGE_TOKIPON 2 /* ma pona pi toki pona helped, Digi-Space Productions made translation.*/
#define CC_LANGUAGE_GERMAN_ 3
#define CC_LANGUAGE_JP_DSP_ 4 /* @ryokuogreen on twitter helped with this translation */

extern unsigned char CC_CurrentLanguage;
extern void applyLanguageToGame();

/* TODO (if this makes it into the CC Codebase: convince the other devs to make all UI text use strings that use longs/shorts (longs perfered) for storing text.) */

/* Generalized Option Text */
static char* ccStrings_optionsMenu[CC_LANGUAGE_LANGCNT][27] = {
	/* English */
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
	"OFF",
	"Cancel",

	"Load file...",
	"Save file...",
	"Upload",
	"Download",
	
	"Save",
	"Load"
	},

	/* Spanish */
	{
	"Opciones varias...",
	"Opciones de interfaz...",
	"Opciones gr†ficas...",
	"Controles...",
	"Opciones de chat...",
	"Configuraci¢n de hacks...",
	"Configuraci¢n de entorno...",
	"Opciones de nostalgia...",
	
	"Opciones...",
	"Cambiar paquete de texturas...",
	"Teclas de acceso r†pido...",
	"Generar nuevo nivel...",
	"Cargar nivel...",
	"Guardar nivel...",
	
	"Listo",
	"Salir del juego",
	"Volver al juego",
	"Men£ del juego",
	"Activado",
	"Desactivado",
	"Cancelar",

	"Cargar archivo...",
	"Guardar archivo...",
	"Subir",
	"Descargar",
	
	"Guardar",
	"Cargar"
	},

	/* Toki Pona */
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
	"o pali e ma sin...",
	"ante e ma...",
	"o awen e ma...",

	"o pini",
	"o weka",
	"o kama musi",
	"lipu musi",
	"wile",
	"ala",
	"o pali ala",

	"o alasa lipu",
	"o awen lipu",
	"o pana lipu",
	"o awen lipu",
	
	"o awen",
	"o alasa"
	},

	/* German */
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
	"Hotkeys...",
	"Neues Level generieren...",
	"Level laden...",
	"Level speichern...",
	
	"Fertig",
	"Spiel beenden",
	"ZurÅck zum Spiel",
	"SpielmenÅ",
	"Ein",
	"Aus",
	"Stornieren",

	"Datei laden?",
	"Datei speichern?",
	"Hochladen",
	"Herunterladen",
	
	"Speichern",
	"Laden"
	},

	/* Japanese */
	{
	"\x1D\x8E\x1F\x8E\xB5\xCC\xDF\xBC\xAE\xDD",
	"GUI\xB5\xCC\xDF\xBC\xAE\xDD",
	"\xB8\xDE\xD7\xCC\xA8\xAF\xB8",
	"\x1D\x06\x15\x1B\x83\x86\x04",
	"\xC1\xAC\xAF\xC4\xB5\xCC\xDF\xBC\xAE\xDD",
	"\xCA\xAF\xB8\x1B\x83\x86\x04",
	"\x0B\xFB\x0D\xE7\x06\x1B\x83\x86\x04",
	"\x8A\x84\x0B\x17\x04\x1B\x83\x86\x04",

	"\xB5\xCC\xDF\xBC\xAE\xDD",
	"\xC3\xB8\xBD\xC1\xAC\xCA\xDF\xAF\xB8\x98\xFB\x13\x06",
	"\xCE\xAF\xC4\xB7\xB0",
	"\x02\x1F\xE9\x17\x04\xDA\xCD\xDE\xD9\xFA\x1B\x04\x1B\x04\x19\xEB",
	"\xDA\xCD\xDE\xD9\xE8\x9F\x13\x9F",
	"\xDA\xCD\xDE\xD9\xFA\x9B\x1D\xDE\xFB",

	"\x0B\xFB\xEA\xE7\x06",
	"\xB9\xDE\xB0\xD1\xFA\xE4\xE1\xEB",
	"\xB9\xDE\xB0\xD1\x8B\xE2\x88\xDE\xEB",
	"\xB9\xDE\xB0\xD1\xD2\xC6\xAD\xB0",
	"\xB5\xDD",
	"\xB5\xCC",
	"\xB7\xAC\xDD\xBE\xD9",

	"\xCC\xA7\xB2\xD9\xFA\xDB\xB0\xC4\xDE",
	"\xCC\xA7\xB2\xD9\xFA\x9B\x1D\xDE\xFB",
	"\xB1\xAF\xCC\xDF\xDB\xB0\xC4\xDE",
	"\xC0\xDE\xB3\xDD\xDB\xB0\xC4\xDE",
	
	"\xBE\xB0\xCC\xDE",
	"\xDB\xB0\xC4\xDE"
	}
};

/* Game Name */
static char* ccStrings_GameTitle[CC_LANGUAGE_LANGCNT] = {
	"ClassiCube",
	"ClassiCube (en Espa§ol)",
	"musi Kasiku",
	"ClassiCube (auf Deutsch)",
	"\xB8\xD7\xBC\xB7\xAD\xB0\xCC\xDE"
};

/* Language */
static char* csString_LanguageNames[CC_LANGUAGE_LANGCNT] = {
	"English",
	"Espa?ol",
	"toki pona (sitelen Lasina)",
	"Deutsch",
	"\x8B\x83\x9B\xDF\xFB\x13\xDE"
};

/* Worldgen */
static char* ccString_SubOption_Worldgen[CC_LANGUAGE_LANGCNT][7] = {
	/* English */
	{
	 "Generate new level",
	 "Width: ",
	 "Height: ",
	 "Length: ",
	 "Seed: ",
	 "Flatgrass",
	 "Vanilla"
	},

	/* Spanish */
	{
	 "Generate new level",
	 "Width: ",
	 "Height: ",
	 "Length: ",
	 "Seed: ",
	 "Flatgrass",
	 "Vanilla"
	},

	/* Toki Pona */
	{
	 "o pali e ma sin",
	 "suli poka: ",
	 "suli sewi: ",
	 "suli insa: ",
	 "nanpa nasa: ",
	 "ma supa",
	 "ma pi supa ala"
	},

	/* German */
	{
	 "Generate new level",
	 "Width: ",
	 "Height: ",
	 "Length: ",
	 "Seed: ",
	 "Flatgrass",
	 "Vanilla"
	},

	/* Japanese */
	{
	"\x02\x1F\xE9\x17\x04\xDA\xCD\xDE\xD9\xFA\x1B\x04\x1B\x04\x19\xEB",
	"\x8F\x8F\xDE: ",
	"\x1F\x0B\x2D\x15: ",
	"\x8A\x0B\xDE\x2D\x15: ",
	"\xBC\xB0\xC4\xDE: ",
	"\x1F\x04\xE9",
	"\x95\x84\x06"
	},
};

/* Graphics Options */
static char* ccString_SubOption_Graphics[CC_LANGUAGE_LANGCNT][11] = {
	/* English */
	{
	 "FPS mode",
	 "View distance",
	 "Smooth lighting",
	 "Lighting mode",
	 "Names",
	 "Shadows",
	 "Mipmaps",
	 "Texture filtering", /* Filter textures on Nintendo 64. */
	 "3D anaglyph"
	},

	/* Spanish */
	{
	 "FPS mode",
	 "View distance",
	 "Smooth lighting",
	 "Lighting mode",
	 "Names",
	 "Shadows",
	 "Mipmaps",
	 "Texture filtering", /* Filter textures on Nintendo 64. */
	 "3D anaglyph"
	},

	/* Toki Pona */
	{
	 "nasin pi sitelen FPS",
	 "lukin wawa",
	 "pimeja pona",
	 "nasin pimeja",
	 "nimi",
	 "pimeja soweli",
	 "sitelen lili",
	 "sitelen pona", /* Filter textures on Nintendo 64. */
	 "sitelen tu"
	},

	/* German */
	{
	 "FPS mode",
	 "View distance",
	 "Smooth lighting",
	 "Lighting mode",
	 "Names",
	 "Shadows",
	 "Mipmaps",
	 "Texture filtering", /* Filter textures on Nintendo 64. */
	 "3D anaglyph"
	},
	
	/* Japanese */
	{
	 "FPS mode",
	 "View distance",
	 "Smooth lighting",
	 "Lighting mode",
	 "Names",
	 "Shadows",
	 "Mipmaps",
	 "Texture filtering", /* Filter textures on Nintendo 64. */
	 "3D anaglyph"
	},
};

/* Misc Options */
static char* ccString_SubOption_Misc[CC_LANGUAGE_LANGCNT][11] = {
	/* English */
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
	
	/* Spanish */
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

	/* Toki Pona */
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

	/* German */
	{
	"Sprache",
	"Reichweite",
	"Kameramasse",
	"MusiklautstÑrke",
	"SoundlautstÑrke",
	"Blockphysik",
	"Kamera ruckelfrei",
	"Ansicht wackeln",
	"Maus umkehren",
	"Mausempfindlichkeit"
	},

	/* Japanese */
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
};

/* GUI Options */
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
	"Escala de la barra de acceso r†pido",
	"Escala del inventario",
	"Escala de la mira",
	"Sombras de texto negro",
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
	"Hotbar-Skalierung",
	"Inventar-Skalierung",
	"Fadenkreuz-Skalierung",
	"Schwarze Textschatten",
	"Tab-AutovervollstÑndigung",
	"Systemschriftart verwenden",
	"Systemschriftart auswÑhlen"
	},

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
};

/* Chat Options */
static char* ccString_SubOption_Chat[CC_LANGUAGE_LANGCNT][5] = {
	{
	 "Scale with window",
	 "Chat scale",
	 "Chat lines",
	 "Log to disk",
	 "Clickable chat"
	},

	{
	 "Scale with window",
	 "Chat scale",
	 "Chat lines",
	 "Log to disk",
	 "Clickable chat"
	},

	{
	 "suli tawa sitelen",
	 "toki suli",
	 "lipu nanpa",
	 "o awen tawa lipu sina",
	 "sina ken luka toki"
	},

	{
	 "Scale with window",
	 "Chat scale",
	 "Chat lines",
	 "Log to disk",
	 "Clickable chat"
	},

	{
	 "Scale with window",
	 "Chat scale",
	 "Chat lines",
	 "Log to disk",
	 "Clickable chat"
	},
};

/* Nostalgia Options (Appearance) */
static char* ccString_SubOption_NostalgicAppearance[CC_LANGUAGE_LANGCNT][7] = {
	{
	"Classic hand model",
	"Classic walk anim",
	"Classic chat",
	"Classic inventory",
	"Classic GUI textures",
	"Classic player list",
	"Classic options"
	},
	
	{
	"Modelo de mano cl†sico",
	"Animaci¢n de andar cl†sico",
	"Chat cl†sico",
	"Inventario cl†sico",
	"Texturas de interfaz cl†sico",
	"Lista de jugadores cl†sico",
	"Opciones cl†sico"
	},

	{
	"luka lukin pi tenpo Kasi",
	"tawa lukin pi tenpo Kasi",
	"lipu toki lukin pi tenpo Kasi",
	"lipu toki lukin pi tenpo Kasi",
	"suli mute ijo lukin pi tenpo Kasi",
	"lipu jan lukin pi tenpo Kasi",
	"ante e musi pi tenpo Kasi"
	},

	{
	"Classic Handmodell",
	"Classic Laufanimation",
	"Classic Chat",
	"Classic Inventar",
	"Classic GUI-Texturen",
	"Classic Spielerliste",
	"Classic Optionen"
	},

	{
	"Classic hand model",
	"Classic walk anim",
	"Classic chat",
	"Classic inventory",
	"Classic GUI textures",
	"Classic player list",
	"Classic options"
	},
};

/* Render Distance Text */
static const char* const ccString_ViewDistanceNames[CC_LANGUAGE_LANGCNT][4] = {
	{
	"TINY",
	"SHORT",
	"NORMAL",
	"FAR"
	},
	
	{
	"PEQUE•O",
	"BAJO",
	"NORMAL",
	"LEJO"
	},

	{
	"lili mute",
	"lili",
	"suli",
	"suli mute"
	},

	{
	"KLEIN",
	"KURZ",
	"NORMAL",
	"WEIT"
	},

	{
	"TINY",
	"SHORT",
	"NORMAL",
	"FAR"
	},
};

/* Texture Options */
static char* ccString_SubOption_TexturePack[CC_LANGUAGE_LANGCNT][1] = {
	{
	"Select a texture pack"
	},
	{
	"Seleccione un paquete de texturas"
	},
	{
	"poki sitelen seme?"
	},
	{
	"WÑhlen Sie ein Texturpaket"
	},
	{
	"Select a texture pack"
	},
};

/* Worldgen Options */
static char* ccString_SubOption_WorldGen[CC_LANGUAGE_LANGCNT][4] = {
	{
	"Generate new level",
	"Small",
	"Normal",
	"Huge"
	},
	{
	"Generar nuevo nivel",
	"Peque§o",
	"Promedio",
	"Enorme"
	},
	{
	"o pali e ma sin",
	"lili",
	"meso",
	"suli"
	},
	{
	"Neues Level generieren",
	"Klein",
	"RegulÑr",
	"Riesig"
	},
	{
	"Generate new level",
	"Small",
	"Normal",
	"Huge"
	},
};

/* Classic Options */
static char* ccString_SubOption_ClassicOptions[CC_LANGUAGE_LANGCNT][6] = {
	{
	"Music",
	"Limit framerate",
	"3D anaglyph",
	"Sound",
	"Render distance",
	"Hacks enabled"
	},
	{
	"M£sica",
	"Limitar velocidad de fotogramas",
	"Anaglifo 3D",
	"Sonido",
	"Distancia de visi¢n",
	"Hacks habilitados"
	},
	{
	"kalama musi",
	"wan supa sinpin",
	"lukin 3D",
	"kalama",
	"lukin wawa",
	"nasin wawa"
	},
	{
	"Musik",
	"Framerate begrenzen",
	"3D-Anaglyphen",
	"Klang",
	"Sichtweite",
	"Hacks aktiviert"
	},
	{
	"Music",
	"Limit framerate",
	"3D anaglyph",
	"Sound",
	"Render distance",
	"Hacks enabled"
	},
};

/* Classic GUI */
static char* ccString_SubOption_ClassicGUI[CC_LANGUAGE_LANGCNT][1] = {
	{
	 "Select block"
	},
	{
	 "Seleccionar bloque"
	},
	{
	 "leko seme?"
	},
	{
	 "Block auswÑhlen"
	},
	{
	 "Select block"
	},
};

/* Blocks */
static char* ccString_SubOption_Blocks[CC_LANGUAGE_LANGCNT][66] = {
	{
		"Air", "Stone", "Grass", 	"Dirt", "Cobblestone",	"Wood", "Sapling", "Bedrock",
		"Water",	"Still water",	"Lava", 	"Still lava",	"Sand", "Gravel",	"Gold ore", "Iron ore",
		"Coal ore",	"Log", "Leaves",	"Sponge", "Glass", "Red", "Orange", "Yellow",
		"Lime", "Green", "Teal", "Aqua", "Cyan", "Blue", "Indigo", "Violet",	
		
		"Magenta", "Pink", "Black",	"Gray", "White", "Dandelion",	"Rose",	"Brown mushroom",
		"Red mushroom", "Gold", "Iron", "Double slab",	"Slab", "Brick", "TNT",	"Bookshelf",
		"Mossy rocks", "Obsidian",	"Cobblestone slab", "Rope", "Sandstone",	"Snow", "Fire",	"Light pink",
		"Forest green", "Brown",	"Deep blue",	"Turquoise", "Ice", "Ceramic tile","Magma",	"Pillar",
		"Crate", "Stone brick"
	},
	
	{
		"Aire",			"Piedra",    "Hierba", "Tierra",   "Adoqu°n",  "Madera",  "Brote",    "Roca madre",
		"Agua",			"Agua quieta",      "Lava",   "Lava quieta",     "Arena",    "Grava",   "Mineral de oro",  "Mineral de hierro",
		"Mineral de carb¢n","Tronco",    "Hojas",  "Esponja",  "Vidrio",   "Rojo",    "Naranja",  "Amarillo",
		"Verde lima",	"Verde",     "Azul verdoso",  "Aguamarina",      "Cian",     "Azul",    "Indigo",   "Violeta",
		
		"Magenta",		"Rosa", "Negro",  "Gris",     "Blanco",   "Diente de le¢n",  "Rosa",     "Hongo marr¢n",
		"Hongo rojo",	"Oro", "Hierro", "Loseta doble",    "Loseta",   "Ladrillo", "Dinamita",	"Estanter°a",
		"Rocas musgosas",	"Obsidiana", "Loseta de adoqu°n","Cuerda", "Arenisca", "Nieve",    "Fuego",    "Rosa claro",
		"Verde bosque",	"Marr¢n", "Azul oscuro",   "Turquesa", "Hielo",    "Baldosa cer†mica","Magma",    "Pilar",
		"Caja",			"Ladrillo de piedra"
    },

	{
		"kon",			"kiwen", "kasi", "ma", "kiwen nena", "kiwen kipisi pi kasi kiwen", "kasi lili", "kiwen pi pakala ala",
		"telo",			"telo li awen",	"telo moli", "telo moli li awen", "ko", "kiwen lili mute", "kiwen jelo lon insa kiwen",	"kiwen walo lon insa kiwen",
		"ko pimeja lon insa kiwen",	"sijelo pi kasi kili", "lipu pi kasi kili", "ko pi weka telo", "kiwen lukin", "loje", "loje jelo", "jelo",
		"jelo laso",	"laso", "laso suno", "laso telo suno", "laso sewi", "laso", "laso loje pimeja", "laso loje",	
		
		"loje pimeja",	"loje suno", "pimeja", "pimeja suno", "suno", "kasi jelo", "kasi loje", "soko",
		"soko loje",	"leko mani", "leko kiwen", "supa", "supa tu", "leko", "ilo pakala", "poki pi lipu mute",
		"Mossy rocks",	"telo moli kiwen lete", "supa pi kiwen nena", "linja", "ko kiwen", "ko pi telo lete", "seli", "loje suno mute",
		"laso kasi",	"pimeja ma", "laso telo pimeja", "laso pimeja", "leko pi telo lete", "ko kiwen mute", "telo moli kiwen", "palisa",
		"poki",			"kiwen leko"
	},

	{
		"Luft",			"Stein", "Gras", "Erde",			 "Pflasterstein", "Holz", "Setzling", "Grundgestein",
		"Wasser",		"Stilles Wasser", "Lava", "Stilles Lava", "Sand", "Kies", "Golderz", "Eisenerz",
		"Kohleerz",		"Stamm", "BlÑtter", "Schwamm", "Glas", "Rot", "Orange", "Gelb",
		"LimettengrÅn",	"GrÅn", "BlaugrÅn",	"Aqua",			 "Zyan", "Blau", "Indigo", "Violett",
		
		"Magenta",		"Rosa", "Schwarz", "Grau",			 "Wei·", "Lîwenzahn", "Rose",		 "Brauner Pilz",
		"Roter Pilz",	"Gold", "Eisen", "Doppelplatte", "Platte",			 "Ziegel", "TNT",		 "BÅcherregal",
		"Moosige Felsen","Obsidian", "Pflastersteinplatte", "Seil",	"Sandstein", "Schnee", "Feuer", "Hellrosa",
		"WaldgrÅn",		"Braun", "Dunkelblau", "TÅrkis", "Eis", "Keramikfliese","Magma", "SÑule",
		"Kiste",		"Steinziegel"
	},

	{
		"\x0F\x06\x0D", "\x04\x17", "\x0F\x15\xCC\xDE\xDB\xAF\xB8", "\x84\x81", "\x9E\xEB\x04\x17", "\x04\x1F\x15\xDE\x04", "\x8A\x08\x0D\xDE", "\x0B\xDE\xFB\x8F\xDE\xFB",
		"\x9F\x19\xDE", "\x1B\x04\x17\x17\x1F\x9F\x19\xDE", "\xE8\x06\x0B\xDE\xFB", "\x1B\x04\x17\x17\x1F\xE8\x06\x0B\xDE\xFB", "\x19\x8A", "\x17\xDE\xE3\xEA", "\x0D\xFB\x13\x06\x1B\x0D", "\x86\x83\x13\x06\x1B\x0D",
		"\x1B\x0D\x1F\xFB\x13\x06\x1B\x0D", "\x0D", "\x8F",	"\xBD\xCE\xDF\xDD\xBC\xDE", "\xB6\xDE\xD7\xBD", "\x02\x0B\x04\xED\x8E\xE8\x06\xE2\x06","\x1F\xDE\x04\x1F\xDE\x04\x04\xED\x8E\xE8\x06\xE2\x06","\x0D\x04\xED\x8E\xE8\x06\xE2\x06",
		"\x0D\x9F\x88\xDE\xEA\x04\xED\x8E\xE8\x06\xE2\x06","\x9F\x88\xDE\xEA\x04\xED\x8E\xE8\x06\xE2\x06","\x17\xFB\xEA\xE7\x0F\x17\xE7\x0F\x8E\xE8\x06\xE2\x06","\x02\x0A\x9F\x88\xDE\xEA\x04\xED\x8E\xE8\x06\xE2\x06","\x9F\x19\xDE\x04\xED\x8E\xE8\x06\xE2\x06","\x0F\xDE\xFB\x17\xDE\xE7\x06\x17\xE7\x0F\x8E\xE8\x06\xE2\x06","\x19\x9F\xEC\x04\xED\x8E\xE8\x06\xE2\x06","\xE0\xE9\x15\x0D\x04\xED\x8E\xE8\x06\xE2\x06",	
		
		"\x02\x0B\xE0\xE9\x15\x0D\x04\xED\x8E\xE8\x06\xE2\x06","\x8F\xDE\xE9\x04\xED\x8E\xE8\x06\xE2\x06","\x8E\x06\x8F\x04\x04\xED\x8E\xE8\x06\xE2\x06","\x06\x19\x8F\x04\x04\xED\x8E\xE8\x06\xE2\x06","\x8F\x0F\x17\xE7\x0F\x8E\xE8\x06\xE2\x06", "\xC0\xDD\xCE\xDF\xCE\xDF","\xCA\xDE\xD7","\x81\xE3\x04\xED\x8E\xB7\xC9\xBA",
		"\x02\x0B\x04\xED\x8E\xB7\xC9\xBA","\x0D\xFB\xCC\xDE\xDB\xAF\xB8","\x86\x84\xCC\xDE\xDB\xAF\xB8","\x0B\x15\x8D\x1F\xCA\xB0\xCC\xCC\xDE\xDB\xAF\xB8","\xCA\xB0\xCC\xCC\xDE\xDB\xAF\xB8","\xDA\xDD\xB6\xDE","TNT","\x9B\xFB\x1F\xDE\x8A",
		
		"\x13\x11\xE0\x17\x1F\x9E\xEB\x04\x17","\x13\x0F\xE8\x06\x1B\x0D",
		"\x9E\xEB\x04\x17\x8E\xCA\xB0\xCC\xCC\xDE\xDB\xAF\xB8","\xDB\xB0\xCC\xDF","\x15\x0B\xDE\xFB","\x13\x8A\xE6\x0D","\x08\xFB","\x02\x0B\xEB\x04\xCB\xDF\xDD\xB8\xE8\x06\xE2\x06",
		"\x95\x0B\x04\x9F\x88\xDE\xEA\xE8\x06\xE2\x06","\x81\xE3\x04\xED\xE8\x06\xE2\x06","\x13\x04\x02\x0A\x04\xED\xE8\x06\xE2\x06","\xC0\xB0\xBA\xB2\xBD\xDE\xE8\x06\xE2\x06","\x13\x0A\xEA","\x1F\xDE\x04\xEA\x1B\x0D\xC0\xB2\xD9","\xCF\xB8\xDE\xCF\xCC\xDE\xDB\xAF\xB8","\x1F\xDE\x04\xEA\x1B\x0D\x8E\x8F\x17\xE9",
		"\x8F\x13","\x04\x17\xDA\xDD\xB6\xDE"
	},

};
#endif