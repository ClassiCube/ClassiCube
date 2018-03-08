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
		
		public static string[] DigSounds = new string[] {
			"cloth1",  "cloth2",  "cloth3",  "cloth4",  "grass1", "grass2", "grass3", "grass4",
			"gravel1", "gravel2", "gravel3", "gravel4", "sand1",  "sand2",  "sand3",  "sand4",
			"snow1",   "snow2",   "snow3",   "snow4",   "stone1", "stone2", "stone3", "stone4",
			"wood1",   "wood2",   "wood3",   "wood4",   "glass1", "glass2", "glass3",
		};
		
		public static string[] DigHashes = new string[] {
			"5f/5fd568d724ba7d53911b6cccf5636f859d2662e8", "56/56c1d0ac0de2265018b2c41cb571cc6631101484",
			"9c/9c63f2a3681832dc32d206f6830360bfe94b5bfc", "55/55da1856e77cfd31a7e8c3d358e1f856c5583198",
			"41/41cbf5dd08e951ad65883854e74d2e034929f572", "86/86cb1bb0c45625b18e00a64098cd425a38f6d3f2",
			"f7/f7d7e5c7089c9b45fa5d1b31542eb455fad995db", "c7/c7b1005d4926f6a2e2387a41ab1fb48a72f18e98",
			"e8/e8b89f316f3e9989a87f6e6ff12db9abe0f8b09f", "c3/c3b3797d04cb9640e1d3a72d5e96edb410388fa3",
			"48/48f7e1bb098abd36b9760cca27b9d4391a23de26", "7b/7bf3553a4fe41a0078f4988a13d6e1ed8663ef4c",
			"9e/9e59c3650c6c3fc0a475f1b753b2fcfef430bf81", "0f/0fa4234797f336ada4e3735e013e44d1099afe57",
			"c7/c75589cc0087069f387de127dd1499580498738e", "37/37afa06f97d58767a1cd1382386db878be1532dd",
			"e9/e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c", "58/5887d10234c4f244ec5468080412f3e6ef9522f3",
			"a4/a4bc069321a96236fde04a3820664cc23b2ea619", "e2/e26fa3036cdab4c2264ceb19e1cd197a2a510227",
			"4e/4e094ed8dfa98656d8fec52a7d20c5ee6098b6ad", "9c/9c92f697142ae320584bf64c0d54381d59703528",
			"8f/8f23c02475d388b23e5faa680eafe6b991d7a9d4", "36/363545a76277e5e47538b2dd3a0d6aa4f7a87d34",
			"9b/9bc2a84d0aa98113fc52609976fae8fc88ea6333", "98/98102533e6085617a2962157b4f3658f59aea018",
			"45/45b2aef7b5049e81b39b58f8d631563fadcc778b", "dc/dc66978374a46ab2b87db6472804185824868095",
			"72/7274a2231ed4544a37e599b7b014e589e5377094", "87/87c47bda3645c68f18a49e83cbf06e5302d087ff",
			"ad/ad7d770b7fff3b64121f75bd60cecfc4866d1cd6",
		};
			
		public static string[] StepSounds = new string[] {
			"cloth1",  "cloth2",  "cloth3",  "cloth4",  "grass1", "grass2", "grass3", "grass4",
			"gravel1", "gravel2", "gravel3", "gravel4", "sand1",  "sand2",  "sand3",  "sand4",
			"snow1",   "snow2",   "snow3",   "snow4",   "stone1", "stone2", "stone3", "stone4",
			"wood1",   "wood2",   "wood3",   "wood4",
		};
		
		public static string[] StepHashes = new string[] {
			"5f/5fd568d724ba7d53911b6cccf5636f859d2662e8", "56/56c1d0ac0de2265018b2c41cb571cc6631101484",
			"9c/9c63f2a3681832dc32d206f6830360bfe94b5bfc", "55/55da1856e77cfd31a7e8c3d358e1f856c5583198",
			"22/227ab99bf7c6cf0b2002e0f7957d0ff7e5cb0c96", "5c/5c971029d9284676dce1dda2c9d202f8c47163b2",
			"76/76de0a736928eac5003691d73bdc2eda92116198", "bc/bc28801224a0aa77fdc93bb7c6c94beacdf77331",
			"1d/1d761cb3bcb45498719e4fba0751e1630e134f1a", "ac/ac7a7c8d106e26abc775b1b46150c083825d8ddc",
			"c1/c109b985a7e6d5d3828c92e00aefa49deca0eb8c", "a4/a47adece748059294c5f563c0fcac02fa0e4bab4",
			"98/9813c8185197f4a4296649f27a9d738c4a6dfc78", "bd/bd1750c016f6bab40934eff0b0fb64c41c86e44b",
			"ab/ab07279288fa49215bada5c17627e6a54ad0437c", "a4/a474236fb0c75bd65a6010e87dbc000d345fc185",
			"e9/e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c", "58/5887d10234c4f244ec5468080412f3e6ef9522f3",
			"a4/a4bc069321a96236fde04a3820664cc23b2ea619", "e2/e26fa3036cdab4c2264ceb19e1cd197a2a510227",
			"4a/4a2e3795ffd4d3aab0834b7e41903af3a8f7d197", "22/22a383d9c22342305a4f16fec0bb479a885f8da2",
			"a5/a533e7ae975e62592de868e0d0572778614bd587", "d5/d5218034051a13322d7b5efc0b5a9af739615f04",
			"af/afb01196e2179e3b15b48f3373cea4c155d56b84", "1e/1e82a43c30cf8fcbe05d0bc2760ecba5d2320314",
			"27/27722125968ac60c0f191a961b17e406f1351c6e", "29/29586f60bfe6f521dbc748919d4f0dc5b28beefd",
		};

		
		public static string[] MusicFiles = new string[] {
			"calm1.ogg", "calm2.ogg", "calm3.ogg", "hal1.ogg", "hal2.ogg", "hal3.ogg", "hal4.ogg"
		};
		
		public static string[] MusicHashes = new string[] {
			"50/50a59a4f56e4046701b758ddbb1c1587efa4cadf", "74/74da65c99aa578486efa7b69983d3533e14c0d6e",
			"14/14ae57a6bce3d4254daa8be2b098c2d99743cc3f", "df/df1ff11b79757432c5c3f279e5ecde7b63ceda64",
			"ce/ceaaaa1d57dfdfbb0bd4da5ea39628b42897a687", "dd/dd85fb564e96ee2dbd4754f711ae9deb08a169f9",
			"5e/5e7d63e75c6e042f452bc5e151276911ef92fed8",
		};
		
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
