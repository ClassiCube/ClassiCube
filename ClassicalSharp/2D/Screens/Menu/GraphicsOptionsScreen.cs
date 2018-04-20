// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Textures;

namespace ClassicalSharp.Gui.Screens {
	public class GraphicsOptionsScreen : MenuOptionsScreen {
		
		public GraphicsOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			validators = new MenuInputValidator[widgets.Length];
			defaultValues = new string[widgets.Length];
			
			validators[0]    = new EnumValidator(typeof(FpsLimitMethod));
			validators[1]    = new IntegerValidator(8, 4096);
			defaultValues[1] = "512";
			validators[3]    = new EnumValidator(typeof(NameMode));
			validators[4]    = new EnumValidator(typeof(EntityShadow));
			
			MakeDescriptions();
		}
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnInputClick;
			ClickHandler onEnum = OnEnumClick;
			ClickHandler onBool = OnBoolClick;
			
			widgets = new Widget[] {
				MakeOpt(-1, -50, "FPS mode",         onEnum,  GetFPS,      SetFPS),
				MakeOpt(-1, 0, "View distance",      onClick, GetViewDist, SetViewDist),
				MakeOpt(-1, 50, "Advanced lighting", onBool,  GetSmooth,   SetSmooth),
				
				MakeOpt(1, -50, "Names",             onEnum,  GetNames,    SetNames),
				MakeOpt(1, 0, "Shadows",             onEnum,  GetShadows,  SetShadows),
				MakeOpt(1, 50, "Mipmaps",            onBool,  GetMipmaps,  SetMipmaps),
				
				MakeBack(false, titleFont, SwitchOptions),
				null, null, null,
			};
		}

		static string GetViewDist(Game g) { return g.ViewDistance.ToString(); }
		static void SetViewDist(Game g, string v) { g.SetViewDistance(Int32.Parse(v), true); }
		
		static string GetSmooth(Game g) { return GetBool(g.SmoothLighting); }
		static void SetSmooth(Game g, string v) {
			g.SmoothLighting = SetBool(v, OptionsKey.SmoothLighting);
			ChunkMeshBuilder builder = g.ChunkUpdater.DefaultMeshBuilder();
			g.ChunkUpdater.SetMeshBuilder(builder);
			g.ChunkUpdater.Refresh();
		}
		
		static string GetNames(Game g) { return g.Entities.NamesMode.ToString(); }
		static void SetNames(Game g, string v) {
			object raw = Enum.Parse(typeof(NameMode), v);
			g.Entities.NamesMode = (NameMode)raw;
			Options.Set(OptionsKey.NamesMode, v);
		}
		
		static string GetShadows(Game g) { return g.Entities.ShadowMode.ToString(); }
		static void SetShadows(Game g, string v) {
			object raw = Enum.Parse(typeof(EntityShadow), v);
			g.Entities.ShadowMode = (EntityShadow)raw;
			Options.Set(OptionsKey.EntityShadow, v);
		}
		
		static string GetMipmaps(Game g) { return GetBool(g.Graphics.Mipmaps); }
		static void SetMipmaps(Game g, string v) {
			g.Graphics.Mipmaps = SetBool(v, OptionsKey.Mipmaps);
			
			string url = g.World.TextureUrl;
			// always force a reload from cache
			g.World.TextureUrl = "~`#$_^*()@";
			TexturePack.ExtractCurrent(g, url);
			g.World.TextureUrl = url;
		}
		
		void MakeDescriptions() {
			string[][] descs = new string[widgets.Length][];
			descs[0] = new string[] {
				"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.",
				"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.",
				"&eNoLimit: &fRenders as many frames as possible each second.",
				"&cUsing NoLimit mode is discouraged.",
			};
			descs[2] = new string[] {
				"&cNote: &eSmooth lighting is still experimental and can heavily reduce performance.",
			};
			descs[3] = new string[] {
				"&eNone: &fNo names of players are drawn.",
				"&eHovered: &fName of the targeted player is drawn see-through.",
				"&eAll: &fNames of all other players are drawn normally.",
				"&eAllHovered: &fAll names of players are drawn see-through.",
				"&eAllUnscaled: &fAll names of players are drawn see-through without scaling.",
			};
			descs[4] = new string[] {
				"&eNone: &fNo entity shadows are drawn.",
				"&eSnapToBlock: &fA square shadow is shown on block you are directly above.",
				"&eCircle: &fA circular shadow is shown across the blocks you are above.",
				"&eCircleAll: &fA circular shadow is shown underneath all entities.",
			};
			descriptions = descs;
		}
	}
}