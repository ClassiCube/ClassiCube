// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public class GraphicsOptionsScreen : MenuOptionsScreen {
		
		public GraphicsOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			ContextRecreated();
			MakeValidators();
			MakeDescriptions();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[] {
				
				MakeOpt(-1, -50, "FPS mode", OnWidgetClick,
				        g => g.FpsLimit.ToString(),
				        (g, v) => { }),

				MakeOpt(-1, 0, "View distance", OnWidgetClick,
				        g => g.ViewDistance.ToString(),
				        (g, v) => g.SetViewDistance(Int32.Parse(v), true)),
				
				MakeBool(-1, 50, "Advanced lighting", OptionsKey.SmoothLighting,
				         OnWidgetClick, g => g.SmoothLighting, SetSmoothLighting),
				
				MakeOpt(1, -50, "Names", OnWidgetClick,
				        g => g.Entities.NamesMode.ToString(),
				        (g, v) => {
				        	object rawNames = Enum.Parse(typeof(NameMode), v);
				        	g.Entities.NamesMode = (NameMode)rawNames;
				        	Options.Set(OptionsKey.NamesMode, v);
				        }),
				
				MakeOpt(1, 0, "Shadows", OnWidgetClick,
				        g => g.Entities.ShadowMode.ToString(),
				        (g, v) => {
				        	object rawShadows = Enum.Parse(typeof(EntityShadow), v);
				        	g.Entities.ShadowMode = (EntityShadow)rawShadows;
				        	Options.Set(OptionsKey.EntityShadow, v);
				        }),
				
				MakeBack(false, titleFont,
				         (g, w) => g.Gui.SetNewScreen(new OptionsGroupScreen(g))),
				null, null,
			};
			
			// NOTE: we need to override the default setter here, because changing FPS limit method
			// recreates the graphics context on some backends (such as Direct3D9)
			ButtonWidget btn = (ButtonWidget)widgets[0];
			btn.SetValue = SetFPSLimitMethod;
		}
		
		void SetFPSLimitMethod(Game g, string v) {
			object rawFps = Enum.Parse(typeof(FpsLimitMethod), v);
			g.SetFpsLimitMethod((FpsLimitMethod)rawFps);
			Options.Set(OptionsKey.FpsLimit, v);
			
			// NOTE: OpenGL backend doesn't recreate context, so cheat and act like recreated anyways
			ContextLost();
			ContextRecreated();
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