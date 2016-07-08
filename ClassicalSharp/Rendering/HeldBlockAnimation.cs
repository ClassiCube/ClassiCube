// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	internal class HeldBlockAnimation {
		
		internal bool doAnim, digAnim, swingAnim;
		internal float angleY = 0;
		internal Vector3 animPosition;
		
		double animTime;
		byte lastType;		
		Game game;
		HeldBlockRenderer held;
		
		internal void Init( Game game, HeldBlockRenderer held ) {
			this.game = game;
			this.held = held;
			lastType = game.Inventory.HeldBlock;
			
			game.Events.HeldBlockChanged += DoSwitchBlockAnim;
			game.UserEvents.BlockChanged += BlockChanged;
		}
		
		internal void Dispose() {
			game.Events.HeldBlockChanged -= DoSwitchBlockAnim;
			game.UserEvents.BlockChanged -= BlockChanged;
		}
		
		/// <summary> Sets the current animation state of the held block.<br/>
		/// true = left mouse pressed, false = right mouse pressed. </summary>
		internal void SetClickAnim( bool dig ) {
			ResetAnimationState( true, 0.25 );
			swingAnim = false;
			digAnim = dig;
			doAnim = true;
			// Start place animation at bottom of cycle
			if( !dig ) animTime = 0.25 / 2;
		}
		
		void DoSwitchBlockAnim( object sender, EventArgs e ) {
			if( swingAnim ) {
				// Like graph -sin(x) : x=0.5 and x=2.5 have same y values
				// but increasing x causes y to change in opposite directions
				if( animTime > animPeriod * 0.5 )
					animTime = animPeriod - animTime;
			} else {
				ResetAnimationState( false, 0.25 );
				doAnim = true;
				swingAnim = true;
			}
		}
		
		void BlockChanged( object sender, BlockChangedEventArgs e ) {
			if( e.Block == 0 ) return;
			SetClickAnim( false );
		}

		double animPeriod = 0.25, animSpeed = Math.PI / 0.25;
		internal void Update( double delta, Vector3 last ) {
			double angle = animTime * animSpeed;
			if( swingAnim || !digAnim ) {
				animPosition.Y = -0.4f * (float)Math.Sin( angle );
				if( swingAnim ) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if( animPosition.Y > last.Y )
						lastType = held.type;
					held.type = lastType;
				}
			} else {
				animPosition.X = -0.325f * (float)Math.Sin( angle );
				animPosition.Y = 0.2f * (float)Math.Sin( angle * 2 );
				animPosition.Z = -0.325f * (float)Math.Sin( angle );
				angleY = 90 * (float)Math.Sin( angle );
				held.block.SwitchOrder = angleY >= 45;
				
				// For first cycle, do not rotate at all.
				// For second cycle, rotate the block from 0-->15 then back to 15-->0.
				float rotX = Math.Max( 0, (float)angle - 90 * Utils.Deg2Rad );
				if( rotX >= 45 * Utils.Deg2Rad ) rotX = 90 * Utils.Deg2Rad - rotX;
				
				rotX /= 3;
				held.block.CosX = (float)Math.Cos( -rotX ); 
				held.block.SinX = (float)Math.Sin( -rotX );
			}
			animTime += delta;
			if( animTime > animPeriod )
				ResetAnimationState( true, 0.25 );
		}
		
		void ResetAnimationState( bool updateLastType, double period ) {
			animTime = 0;
			doAnim = false;
			swingAnim = false;
			animPosition = Vector3.Zero;
			angleY = 0;
			animPeriod = period;
			animSpeed = Math.PI / period;
			
			if( updateLastType )
				lastType = game.Inventory.HeldBlock;
			animPosition = Vector3.Zero;
		}
	}
}