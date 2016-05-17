// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;

namespace Launcher.Patcher {
	
	public sealed class ResourceList {
		
		public const ushort cMask = 0xF000;
		public const ushort mMask = 0x0F00;
		public const ushort gMask = 0x00F0;
		public const ushort tMask = 0x000F;
		
		public static Dictionary<string, ushort> Files = new Dictionary<string, ushort>() {
			// classic jar files
			{ "char.png", cMask }, { "clouds.png", cMask },
			{ "default.png", cMask }, { "particles.png", cMask },
			{ "rain.png", cMask }, { "terrain.png", cMask | tMask },
			{ "gui_classic.png", cMask }, { "icons.png", cMask },
			//{ "arrows.png", cMask }, { "sign.png", cMask },
			{ "creeper.png", cMask }, { "pig.png", cMask },
			{ "sheep.png", cMask }, { "sheep_fur.png", cMask },
			{ "skeleton.png", cMask }, { "spider.png", cMask },
			{ "zombie.png", cMask },
			// Other files
			{ "snow.png", mMask }, { "chicken.png", mMask },
			{ "animations.png", mMask }, { "gui.png", gMask },
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
		
		public static string GetFile( string path ) {
			// Ignore directories: convert x/name to name and x\name to name.
			string name = path.ToLower();
			int i = name.LastIndexOf( '\\' );
			if( i >= 0 ) name = name.Substring( i + 1, name.Length - 1 - i );
			i = name.LastIndexOf( '/' );
			if( i >= 0 ) name = name.Substring( i + 1, name.Length - 1 - i );
			return name;
		}
	}
}
