// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class HeldBlockRenderer : IGameComponent {
		
		Game game;
		BlockModel block;
		FakePlayer held;
		bool playAnimation, leftAnimation, swingAnimation;
		float angleY = 0;
		
		double animTime;
		byte type, lastType;
		Vector3 animPosition;
		Matrix4 heldBlockProj;
		
		public void Init( Game game ) {
			this.game = game;
			block = new BlockModel( game );
			block.NoShade = true;
			held = new FakePlayer( game );
			lastType = game.Inventory.HeldBlock;
			game.Events.HeldBlockChanged += HeldBlockChanged;
			game.UserEvents.BlockChanged += BlockChanged;
			game.Events.ProjectionChanged += ProjectionChanged;
		}

		public void Ready( Game game ) { }
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		public void Render( double delta, float t ) {
			if( game.Camera.IsThirdPerson || !game.ShowBlockInHand ) return;

			Vector3 last = animPosition;
			animPosition = Vector3.Zero;
			type = game.Inventory.HeldBlock;
			block.CosX = 1; block.SinX = 0;
			block.SwitchOrder = false;			
			if( playAnimation ) DoAnimation( delta, last );			
			SetMatrix();
			game.Graphics.SetMatrixMode( MatrixType.Projection );
			game.Graphics.LoadMatrix( ref heldBlockProj );
			bool translucent = game.BlockInfo.IsTranslucent[type];
			
			game.Graphics.Texturing = true;
			if( translucent ) game.Graphics.AlphaBlending = true;
			else game.Graphics.AlphaTest = true;
			game.Graphics.DepthTest = false;
			
			SetPos();
			block.Render( held );
			
			game.Graphics.LoadMatrix( ref game.Projection );
			game.Graphics.SetMatrixMode( MatrixType.Modelview );
			game.Graphics.LoadMatrix( ref game.View );
			
			game.Graphics.Texturing = false;
			if( translucent ) game.Graphics.AlphaBlending = false;
			else game.Graphics.AlphaTest = false;
			game.Graphics.DepthTest = true;
		}
		
		static Vector3 nOffset = new Vector3( 0.56f, -0.72f, -0.72f );
		static Vector3 sOffset = new Vector3( 0.46f, -0.52f, -0.72f );		
		void SetMatrix() {
			Player p = game.LocalPlayer;
			Vector3 eyePos = p.EyePosition;
			Matrix4 m = Matrix4.LookAt( eyePos, eyePos - Vector3.UnitZ, Vector3.UnitY ) * game.Camera.tiltM;
			game.Graphics.LoadMatrix( ref m );
		}
	   
		void SetPos() {
			// Based off details from http://pastebin.com/KFV0HkmD (Thanks goodlyay!)
			BlockInfo info = game.BlockInfo;
			Vector3 offset = info.IsSprite[type] ? sOffset : nOffset;
			Player p = game.LocalPlayer;
			held.ModelScale = 0.4f;
		   
			held.Position = p.EyePosition + animPosition;
			held.Position += offset;
			if( !info.IsSprite[type] ) {
				float height = info.MaxBB[type].Y - info.MinBB[type].Y;
				held.Position.Y += 0.2f * (1 - height);
			}
		   
			held.Position.X -= game.Camera.bobbingHor;
			held.Position.Y -= game.Camera.bobbingVer;
			held.Position.Z -= game.Camera.bobbingHor;
			   
			held.HeadYawDegrees = -45 + angleY;
			held.YawDegrees = -45 + angleY;
			held.PitchDegrees = 0;
			held.Block = type;
		}
		
		double animPeriod = 0.25, animSpeed = Math.PI / 0.25;
		void DoAnimation( double delta, Vector3 last ) {
			double angle = animTime * animSpeed;
			if( swingAnimation || !leftAnimation ) {
				animPosition.Y = -0.4f * (float)Math.Sin( angle );
				if( swingAnimation ) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if( animPosition.Y > last.Y )
						lastType = type;
					type = lastType;
				}
			} else {			
				animPosition.X = -0.325f * (float)Math.Sin( angle );
				animPosition.Y = 0.2f * (float)Math.Sin( angle * 2 );
				animPosition.Z = -0.325f * (float)Math.Sin( angle );
				angleY = 90 * (float)Math.Sin( angle );
				block.SwitchOrder = angleY >= 45;
				
				// For first cycle, do not rotate at all.
				// For second cycle, rotate the block from 0-->15 then back to 15-->0.
				float rotX = Math.Max( 0, (float)angle - 90 * Utils.Deg2Rad );				
				if( rotX >= 45 * Utils.Deg2Rad ) rotX = 90 * Utils.Deg2Rad - rotX;
				rotX /= 3;
				block.CosX = (float)Math.Cos( -rotX ); block.SinX = (float)Math.Sin( -rotX );
			}
			animTime += delta;
			if( animTime > animPeriod )
				ResetAnimationState( true, 0.25 );
		}
		
		void ResetAnimationState( bool updateLastType, double period ) {
			animTime = 0;
			playAnimation = false;
			swingAnimation = false;
			animPosition = Vector3.Zero;
			angleY = 0;
			animPeriod = period;
			animSpeed = Math.PI / period;
			
			if( updateLastType )
				lastType = game.Inventory.HeldBlock;
			animPosition = Vector3.Zero;
		}
		
		/// <summary> Sets the current animation state of the held block.<br/>
		/// true = left mouse pressed, false = right mouse pressed. </summary>
		public void SetAnimationClick( bool left ) {
			ResetAnimationState( true, 0.25 );
			swingAnimation = false;
			leftAnimation = left;
			playAnimation = true;
		}
		
		public void SetAnimationSwitchBlock() {
			if( swingAnimation ) return;
			ResetAnimationState( false, 0.3 );
			swingAnimation = true;
			playAnimation = true;
		}
		
		void HeldBlockChanged( object sender, EventArgs e ) {
			SetAnimationSwitchBlock();
		}
		
		void ProjectionChanged( object sender, EventArgs e ) {
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			heldBlockProj = Matrix4.CreatePerspectiveFieldOfView( 70 * Utils.Deg2Rad,
																 aspectRatio, zNear, game.ViewDistance );
		}
		
		public void Dispose() {
			game.Events.HeldBlockChanged -= HeldBlockChanged;
			game.UserEvents.BlockChanged -= BlockChanged;
			game.Events.ProjectionChanged -= ProjectionChanged;
		}
		
		void BlockChanged( object sender, BlockChangedEventArgs e ) {
			if( e.Block == 0 ) return;
			SetAnimationClick( false );
		}
	}
	
	/// <summary> Skeleton implementation of a player entity so we can reuse
	/// block model rendering code. </summary>
	internal class FakePlayer : Player {
		
		public FakePlayer( Game game ) : base( game ) {
		}
		public byte Block;
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) { }
		
		public override void Tick( double delta ) { }

		public override void RenderModel( double deltaTime, float t ) { }
		
		public override void RenderName() { }
	}
}