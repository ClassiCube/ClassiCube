// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public enum NameMode {
		NoNames,
		HoveredOnly,
		AllNames,
		AllNamesAndHovered,
	}
	
	public class EntityList : IDisposable {
		
		public const int MaxCount = 256;
		public Player[] Players = new Player[MaxCount];
		public Game game;
		public EntityShadow ShadowMode = EntityShadow.None;
		byte closestId;
		
		/// <summary> Mode of how names of hovered entities are rendered (with or without depth testing),
		/// and how other entity names are rendered. </summary>
		public NameMode NamesMode = NameMode.AllNamesAndHovered;
		
		public EntityList( Game game ) {
			this.game = game;
			game.Events.ChatFontChanged += ChatFontChanged;
			NamesMode = Options.GetEnum( OptionsKey.NamesMode, NameMode.AllNamesAndHovered );
			if( game.PureClassicMode ) NamesMode = NameMode.HoveredOnly;
			ShadowMode = Options.GetEnum( OptionsKey.EntityShadow, EntityShadow.None );
			if( game.PureClassicMode ) ShadowMode = EntityShadow.None;
		}
		
		/// <summary> Performs a tick call for all player entities contained in this list. </summary>
		public void Tick( double delta ) {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] == null ) continue;
				Players[i].Tick( delta );
			}
		}
		
		/// <summary> Renders the models of all player entities contained in this list. </summary>
		public void RenderModels( IGraphicsApi api, double delta, float t ) {
			api.Texturing = true;
			api.AlphaTest = true;
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] == null ) continue;
				Players[i].RenderModel( delta, t );
			}
			api.Texturing = false;
			api.AlphaTest = false;
		}
		
		/// <summary> Renders the names of all player entities contained in this list.<br/>
		/// If ShowHoveredNames is false, this method only renders names of entities that are
		/// not currently being looked at by the user. </summary>
		public void RenderNames( IGraphicsApi api, double delta, float t ) {
			if( NamesMode == NameMode.NoNames )
				return;
			api.Texturing = true;
			api.AlphaTest = true;
			LocalPlayer localP = game.LocalPlayer;
			Vector3 eyePos = localP.EyePosition;
			closestId = 255;
			
			if( NamesMode != NameMode.AllNames )
				closestId = GetClosetPlayer( game.LocalPlayer );
			if( NamesMode == NameMode.HoveredOnly || !game.LocalPlayer.Hacks.CanSeeAllNames ) {
				api.Texturing = false;
				api.AlphaTest = false;
				return;
			}
			
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] == null ) continue;
				if( i != closestId || i == 255 )
					Players[i].RenderName();
			}
			api.Texturing = false;
			api.AlphaTest = false;
		}
		
		public void RenderHoveredNames( IGraphicsApi api, double delta, float t ) {
			if( NamesMode == NameMode.NoNames || NamesMode == NameMode.AllNames )
				return;
			api.Texturing = true;
			api.AlphaTest = true;
			api.DepthTest = false;
			
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null && i == closestId && i != 255 )
					Players[i].RenderName();
			}
			api.Texturing = false;
			api.AlphaTest = false;
			api.DepthTest = true;
		}
		
		void ChatFontChanged( object sender, EventArgs e ) {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null )
					Players[i].UpdateName();
			}
		}
		
		/// <summary> Disposes of all player entities contained in this list. </summary>
		public void Dispose() {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null )
					Players[i].Despawn();
			}
			game.Events.ChatFontChanged -= ChatFontChanged;
		}
		
		public byte GetClosetPlayer( Player src ) {
			Vector3 eyePos = src.EyePosition;
			Vector3 dir = Utils.GetDirVector( src.HeadYawRadians, src.PitchRadians );
			float closestDist = float.PositiveInfinity;
			byte targetId = 255;
			
			for( int i = 0; i < Players.Length - 1; i++ ) { // -1 because we don't want to pick against local player
				Player p = Players[i];
				if( p == null ) continue;
				
				float t0, t1;
				if( Intersection.RayIntersectsRotatedBox( eyePos, dir, p, out t0, out t1 ) && t0 < closestDist ) {
					closestDist = t0;
					targetId = (byte)i;
				}
			}
			return targetId;
		}
		
		/// <summary> Gets or sets the player entity for the specified id. </summary>
		public Player this[int id] {
			get { return Players[id]; }
			set {
				Players[id] = value;
				if( value != null )
					value.ID = (byte)id;
			}
		}
		
		public void DrawShadows() {
			if( ShadowMode == EntityShadow.None ) return;
			ShadowComponent.boundShadowTex = false;
			game.Graphics.AlphaArgBlend = true;
			game.Graphics.DepthWrite = false;
			game.Graphics.AlphaBlending = true;
			game.Graphics.Texturing = true;
			
			Players[255].shadow.Draw();
			if( ShadowMode == EntityShadow.CircleAll )
				DrawOtherShadows();
			game.Graphics.AlphaArgBlend = false;
			game.Graphics.DepthWrite = true;
			game.Graphics.AlphaBlending = false;
			game.Graphics.Texturing = false;
		}
		
		void DrawOtherShadows() {
			for( int i = 0; i < 255; i++) {
				if( Players[i] == null ) continue;
				Players[i].shadow.Draw();
			}
		}
	}
}
