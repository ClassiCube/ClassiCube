// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.TexturePack;

namespace Launcher {
	
	public sealed class ResourceList {
		
		// Nibbles: Classic jar, 1.6.2 jar, gui patch, terrain patch
		public static Dictionary<string, ushort> Files = new Dictionary<string, ushort>() {
			// classic jar files
			{ "char.png", 0x1000 }, { "clouds.png", 0x1000 },
			{ "default.png", 0x1000 }, { "particles.png", 0x1000 },
			{ "rain.png", 0x1000 }, { "terrain.png", 0x1001 },
			{ "gui.png", 0x1000 }, { "icons.png", 0x1000 },
			{ "arrows.png", 0x1000 }, { "sign.png", 0x1000 },
			{ "creeper.png", 0x1000 }, { "pig.png", 0x1000 },
			{ "sheep.png", 0x1000 }, { "sheep_fur.png", 0x1000 },
			{ "skeleton.png", 0x1000 }, { "spider.png", 0x1000 },
			{ "zombie.png", 0x1000 },
			// Other files
			{ "snow.png", 0x0100 }, { "chicken.png", 0x0100 },
			{ "animations.png", 0x0100 }, { "gui_classic.png", 0x0010 },
		};
		
		public static string[] DigSounds = new [] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bglass1",
			"Bglass2", "Bglass3", "Agrass1", "Agrass2", "Agrass3", "Agrass4", "Agravel1", "Agravel2",
			"Agravel3", "Agravel4", "Asand1", "Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3",
			"Asnow4", "Astone1", "Astone2", "Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		public static string[] StepSounds = new [] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bgrass1",
			"Bgrass2", "Bgrass3", "Bgrass4", "Agravel1", "Agravel2", "Agravel3", "Agravel4", "Asand1",
			"Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3", "Asnow4", "Astone1", "Astone2",
			"Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		public static string[] MusicFiles = new [] { "calm1", "calm2", "calm3", "hal1", "hal2", "hal3", "hal4" };
	}
}
