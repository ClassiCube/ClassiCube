// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui.Screens {
	public class GraphicsOptionsScreen : MenuOptionsScreen {
		
		public GraphicsOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {	
				
				MakeOpt(-1, -50, "FPS mode", OnWidgetClick,
				     g => g.FpsLimit.ToString(),
				     (g, v) => { object raw = Enum.Parse(typeof(FpsLimitMethod), v);
				     	g.SetFpsLimitMethod((FpsLimitMethod)raw);
				     	Options.Set(OptionsKey.FpsLimit, v); }),

				MakeOpt(-1, 0, "View distance", OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance(Int32.Parse(v), true)),
				
				MakeBool(-1, 50, "Advanced lighting", OptionsKey.SmoothLighting,
				     OnWidgetClick, g => g.SmoothLighting, SetSmoothLighting),
				
				MakeOpt(1, -50, "Names", OnWidgetClick,
				     g => g.Entities.NamesMode.ToString(),
				     (g, v) => { object raw = Enum.Parse(typeof(NameMode), v);
				     	g.Entities.NamesMode = (NameMode)raw;
				     	Options.Set(OptionsKey.NamesMode, v); }),
				
				MakeOpt(1, 0, "Shadows", OnWidgetClick,
				     g => g.Entities.ShadowMode.ToString(),
				     (g, v) => { object raw = Enum.Parse(typeof(EntityShadow), v);
				     	g.Entities.ShadowMode = (EntityShadow)raw;
				     	Options.Set(OptionsKey.EntityShadow, v); }),
				
				MakeBack(false, titleFont,
				         (g, w) => g.Gui.SetNewScreen(new OptionsGroupScreen(g))),
				null, null,
			};
			MakeValidators();
			MakeDescriptions();
		}
		
		void SetSmoothLighting(Game g, bool v) {
			g.SmoothLighting = v;
			ChunkMeshBuilder builder = g.MapRenderer.DefaultMeshBuilder();
			g.MapRenderer.SetMeshBuilder(builder);
			g.MapRenderer.Refresh();
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new EnumValidator(typeof(FpsLimitMethod)),
				new IntegerValidator(16, 4096),
				new BooleanValidator(),
				new EnumValidator(typeof(NameMode)),
				new EnumValidator(typeof(EntityShadow)),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[0] = new string[] {
				"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.",
				"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.",
				"&eNoLimit: &fRenders as many frames as possible each second.",
				"&cUsing NoLimit mode is discouraged for general usage.",
			};
			descriptions[2] = new string[] {
				"&cNote: &eSmooth lighting is still experimental and can heavily reduce performance.",
			};
			descriptions[3] = new string[] {
				"&eNoNames: &fNo player names are drawn.",
				"&eHoveredOnly: &fName of the targeted player is drawn see-through.",
				"&eAll: &fAll player names are drawn normally.",
				"&eAllAndHovered: &fName of the targeted player is drawn see-through.",
				"&f      All other player names are drawn normally.",
			};			
			descriptions[4] = new string[] {
				"&eNone: &fNo entity shadows are drawn.",
				"&eSnapToBlock: &fA square shadow is shown on block you are directly above.",
				"&eCircle: &fA circular shadow is shown across the blocks you are above.",
				"&eCircleAll: &fA circular shadow is shown underneath all entities.",
			};
		}
	}
}