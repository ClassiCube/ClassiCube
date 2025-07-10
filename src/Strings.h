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
extern void applyLanguageToGame();

/* TODO (if this makes it into the CC Codebase: convince the other devs to make all UI text use strings that use longs/shorts (longs perfered) for storing text.) */

/* Generalized Option Text */
static char* ccStrings_optionsMenu[CC_LANGUAGE_LANGCNT][27] = {
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
	}
};

/* Game Name */
static char* ccStrings_GameTitle[CC_LANGUAGE_LANGCNT] = {
	"ClassiCube",
	"ClassiCube (en Espa§ol)",
	"musi Kasiku",
	"ClassiCube (auf Deutsch)"
};

/* Language */
static char* csString_LanguageNames[CC_LANGUAGE_LANGCNT] = {
	"English",
	"Espa?ol",
	"toki pona (sitelen Lasina)",
	"Deutsch"
};

/* Graphics Options */
static char* ccString_SubOption_Graphics[CC_LANGUAGE_LANGCNT][11] = {
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
	"Ansicht wackeln",
	"Maus umkehren",
	"Mausempfindlichkeit"
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
	}
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
	}
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
	}
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
	"o pali ma sin",
	"lili",
	"meso",
	"suli"
	},
	{
	"Neues Level generieren",
	"Klein",
	"RegulÑr",
	"Riesig"
	}
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
	}
};

/* Blocks */
static char* ccString_SubOption_Blocks[CC_LANGUAGE_LANGCNT][66] = {
	{
	"Air",		"Stone",		"Grass", 	"Dirt",			"Cobblestone",	"Wood",		"Sapling",		"Bedrock",
	"Water",	"Still water",	"Lava", 	"Still lava",	"Sand",			"Gravel",	"Gold ore",		"Iron ore",
	"Coal ore",	"Log",			"Leaves",	"Sponge",		"Glass",		"Red",		"Orange",		"Yellow",
	"Lime",		"Green",		"Teal",		"Aqua",			"Cyan",			"Blue",		"Indigo",		"Violet",	
	
	"Magenta",			"Pink",		"Black",	"Gray",			"White",		"Dandelion",	"Rose",	"Brown mushroom",
	"Red mushroom",		"Gold",		"Iron",		"Double slab",	"Slab",			"Brick",		"TNT",	"Bookshelf",
	"Mossy rocks",		"Obsidian",	"Cobblestone slab", "Rope", "Sandstone",	"Snow",			"Fire",	"Light pink",
	"Forest green",		"Brown",	"Deep blue",	"Turquoise", "Ice",			"Ceramic tile","Magma",	"Pillar",
	"Crate",			"Stone brick"
	},
	
	{
	"Aire", "Piedra", "Hierba", "Tierra", "Adoqu°n", "Madera", "?rbol joven", "Roca madre",
	"Agua", "Agua estancada", "Lava", "Lava estancada", "Arena", "Grava", "Mena de oro", "Mena de hierro",
	"Mena de carb¢n", "Tronco", "Hojas", "Esponja", "Vidrio", "Rojo", "Naranja", "Amarillo",
	"Lima", "Verde", "Verde azulado", "Aguamarina", "Cian", "Azul", "?ndigo", "Violeta",

	"Magenta", "Rosa", "Negro", "Gris", "Blanco", "Diente de le¢n", "Rosa", "Hongo marr¢n",
	"Hongo rojo", "Oro", "Hierro", "Losa doble", "Losa", "Ladrillo", "TNT", "Estanter°a",
	"Rocas musgosas", "Obsidiana", "Losa de adoqu°n", "Cuerda", "Arenisca", "Nieve", "Fuego", "Rosa claro",
	"Verde bosque", "Marr¢n", "Azul oscuro", "Turquesa", "Hielo", "Azulejo cer†mico", "Magma", "Pilar",
	"Caja", "Ladrillo de piedra"
	},

	{
	"kon",						"kiwen",		"kasi",			"ma",					"kiwen nena",		"kiwen kipisi pi kasi kiwen",		"kasi lili",					"kiwen pi pakala ala",
	"telo",						"telo li awen",	"telo moli",	"telo moli li awen",	"ko",				"kiwen lili mute",					"kiwen jelo lon insa kiwen",	"kiwen walo lon insa kiwen",
	"ko pimeja lon insa kiwen",	"sijelo pi kasi kili",			"lipu pi kasi kili",	"ko pi weka telo",	"kiwen lukin",						"loje",		"loje jelo",		"jelo",
	"jelo laso",		"laso",		"laso suno",		"laso telo suno",			"laso sewi",			"laso",		"laso loje pimeja",		"laso loje",	
	
	"loje pimeja",		"loje suno",	"pimeja",				"pimeja suno",	"suno",							"kasi jelo",		"kasi loje",		"soko",
	"soko loje",		"leko mani",	"leko kiwen",			"supa",			"supa tu",						"leko",				"ilo pakala",		"poki pi lipu mute",
	"Mossy rocks",		"telo moli kiwen lete",		"supa pi kiwen nena",	 "linja",		"ko kiwen",					"ko pi telo lete",	"seli",				"loje suno mute",
	"laso kasi",		"pimeja ma",	"laso telo pimeja",		"laso pimeja",	"leko pi telo lete",			"ko kiwen mute",	"telo moli kiwen",	"palisa",
	"poki",				"kiwen leko"
	},

	{
	"Air",		"Stone",		"Grass", 	"Dirt",			"Cobblestone",	"Wood",		"Sapling",		"Bedrock",
	"Water",	"Still water",	"Lava", 	"Still lava",	"Sand",			"Gravel",	"Gold ore",		"Iron ore",
	"Coal ore",	"Log",			"Leaves",	"Sponge",		"Glass",		"Red",		"Orange",		"Yellow",
	"Lime",		"Green",		"Teal",		"Aqua",			"Cyan",			"Blue",		"Indigo",		"Violet",	
	
	"Magenta",			"Pink",		"Black",	"Gray",			"White",		"Dandelion",	"Rose",	"Brown mushroom",
	"Red mushroom",		"Gold",		"Iron",		"Double slab",	"Slab",			"Brick",		"TNT",	"Bookshelf",
	"Mossy rocks",		"Obsidian",	"Cobblestone slab", "Rope", "Sandstone",	"Snow",			"Fire",	"Light pink",
	"Forest green",		"Brown",	"Deep blue",	"Turquoise", "Ice",			"Ceramic tile","Magma",	"Pillar",
	"Crate",			"Stone brick"
	}
};
#endif