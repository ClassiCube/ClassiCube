// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;

namespace Launcher.Patcher {
	
	public static class ResourceList {	
		public const byte mask_classic = 0x01;
		public const byte mask_modern  = 0x02;
		public const byte mask_gui     = 0x04;
		public const byte mask_terrain = 0x08;
		
		public static string[] Filenames = new string[] {
			// classic jar files
			"char.png", "clouds.png", "default.png", "particles.png",
			"rain.png", "gui_classic.png", "icons.png", "terrain.png",
			"creeper.png", "pig.png", "sheep.png", "sheep_fur.png",
			"skeleton.png", "spider.png", "zombie.png", // "arrows.png", "sign.png"			
			// other files
			"snow.png", "chicken.png", "animations.png", "gui.png",
		};
		
		public static byte[] FileMasks = new byte[] {
			// classic jar files
			mask_classic, mask_classic, mask_classic, mask_classic,
			mask_classic, mask_classic, mask_classic, mask_classic | mask_terrain | mask_modern,
			mask_classic, mask_classic, mask_classic, mask_classic,
			mask_classic, mask_classic, mask_classic, // cMask, cMask
			// other files
			mask_modern, mask_modern, mask_modern, mask_gui,
		};
		
		public static bool[] FilesExist = new bool[Filenames.Length];
		
		public static byte GetFetchFlags() {
			byte flags = 0;
			for (int i = 0; i < Filenames.Length; i++) {
				if (FilesExist[i]) continue;
				flags |= FileMasks[i];
			}
			return flags;
		}
		
		public static string[] DigSounds = new string[] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bglass1",
			"Bglass2", "Bglass3", "Agrass1", "Agrass2", "Agrass3", "Agrass4", "Agravel1", "Agravel2",
			"Agravel3", "Agravel4", "Asand1", "Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3",
			"Asnow4", "Astone1", "Astone2", "Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		public static string[] StepSounds = new string[] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Agrass1",
			"Agrass2", "Agrass3", "Agrass4", "Agravel1", "Agravel2", "Agravel3", "Agravel4", "Asand1",
			"Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3", "Asnow4", "Astone1", "Astone2",
			"Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		public static string[] MusicFiles = new string[] { "calm1", "calm2", "calm3", "hal1", "hal2", "hal3", "hal4" };
		
		public static string GetFile(string path) {
			// Ignore directories: convert x/name to name and x\name to name.
			string name = path.ToLower();
			int i = name.LastIndexOf('\\');
			if (i >= 0) name = name.Substring(i + 1, name.Length - 1 - i);
			i = name.LastIndexOf('/');
			if (i >= 0) name = name.Substring(i + 1, name.Length - 1 - i);
			return name;
		}
	}
}
