using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class GraphicsOptionsScreen : MenuOptionsScreen {
		
		public GraphicsOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			widgets = new Widget[] {				
				
				Make( -140, 0, "FPS limit mode", OnWidgetClick,
				     g => g.FpsLimit.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(FpsLimitMethod), v );
				     	g.SetFpsLimitMethod( (FpsLimitMethod)raw );
				     	Options.Set( OptionsKey.FpsLimit, v ); } ),

				Make( -140, 50, "View distance", OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ), true ) ),
				
				Make( 140, 0, "Names mode", OnWidgetClick,
				     g => g.Players.NamesMode.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(NameMode), v );
				     	g.Players.NamesMode = (NameMode)raw;
				     	Options.Set( OptionsKey.NamesMode, v ); } ),
				
				Make( 140, 50, "Entity shadows", OnWidgetClick,
				     g => g.Players.ShadowMode.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(EntityShadow), v );
				     	g.Players.ShadowMode = (EntityShadow)raw;
				     	Options.Set( OptionsKey.EntityShadow, v ); } ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				null, null,
			};
			widgets[0].Metadata = typeof(FpsLimitMethod);
			widgets[2].Metadata = typeof(NameMode);		
			widgets[3].Metadata = typeof(EntityShadow);
			MakeValidators();
			MakeDescriptions();
		}
		
		void MakeValidators() {
			INetworkProcessor network = game.Network;
			validators = new MenuInputValidator[] {
				new EnumValidator(),
				new IntegerValidator( 16, 4096 ),
				new EnumValidator(),			
				new EnumValidator(),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[0] = new[] {
				"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.",
				"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.",
				"&eNoLimit: &Renders as many frames as the GPU can handle each second.",
				"&cUsing NoLimit mode is discouraged for general usage.",
			};
			descriptions[2] = new[] {
				"&eNoNames: &fNo player names are drawn.",
				"&eHoveredOnly: &fName of the targeted player is drawn see-through.",
				"&eAllNames: &fAll player names are drawn normally.",
				"&eAllNamesAndHovered: &fName of the targeted player is drawn see-through.",
				"&f               All other player names are drawn normally.",
			};			
			descriptions[3] = new[] {
				"&eNone: &fNo entity shadows are drawn.",
				"&eSnapToBlock: &fA square shadow is shown on block you are directly above.",
				"&eCircle: &fA circular shadow is shown across the blocks you are above.",
				"&eCircleAll: &fA circular shadow is shown underneath all entities.",
			};
		}
	}
}