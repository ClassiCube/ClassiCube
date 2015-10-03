using System;
using System.Drawing;
using OpenTK;
using ClassicalSharp.Network;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace ClassicalSharp {

	public abstract partial class Player : Entity {
		
		/// <summary> Gets the position of the player's eye in the world. </summary>
		public Vector3 EyePosition {
			get { return new Vector3( Position.X, Position.Y + Model.GetEyeY( this ), Position.Z ); }
		}
		
		protected Game game;
		public string DisplayName, SkinName;
		public SkinType SkinType;	
		
		public Player( Game game ) : base( game ) {
			this.game = game;
			StepSize = 0.5f;
			SkinType = game.DefaultPlayerSkinType;
			SetModel( "humanoid" );
		}
		
		/// <summary> Gets the block just underneath the player's feet position. </summary>
		public Block BlockUnderFeet {
			get { return GetBlock( new Vector3( Position.X, Position.Y - 0.01f, Position.Z ) ); }
		}
		
		/// <summary> Gets the block at player's eye position. </summary>
		public Block BlockAtHead {
			get { return GetBlock( EyePosition ); }
		}
		
		Block GetBlock( Vector3 coords ) {
			Vector3I blockCoords = Vector3I.Floor( coords );
			return map.IsValidPos( blockCoords ) ?
				(Block)map.GetBlock( blockCoords ) : Block.Air;
		}
		
		public abstract void Tick( double delta );
		
		public abstract void SetLocation( LocationUpdate update, bool interpolate );
		
		public float leftLegXRot, leftArmXRot, leftArmZRot;
		public float rightLegXRot, rightArmXRot, rightArmZRot;
		protected float walkTimeO, walkTimeN, swingO, swingN;
		
		protected void UpdateAnimState( Vector3 oldPos, Vector3 newPos, double delta ) {
			walkTimeO = walkTimeN;
			swingO = swingN;
			float dx = newPos.X - oldPos.X;
			float dz = newPos.Z - oldPos.Z;
			double distance = Math.Sqrt( dx * dx + dz * dz );
			
			if( distance > 0.05 ) {
				walkTimeN += (float)distance * 2 * (float)( 20 * delta );
				swingN += (float)delta * 3;
			} else {
				swingN -= (float)delta * 3;
			}
			Utils.Clamp( ref swingN, 0, 1 );
		}
		
		const float armMax = (float)( 90 * Math.PI / 180.0 );
		const float legMax = (float)( 80 * Math.PI / 180.0 );
		const float idleMax = (float)( 3 * Math.PI / 180.0 );
		const float idleXPeriod = (float)( 2 * Math.PI / 5f );
		const float idleZPeriod = (float)( 2 * Math.PI / 3.5f );
		protected void SetCurrentAnimState( float t ) {
			float swing = Utils.Lerp( swingO, swingN, t );
			float walkTime = Utils.Lerp( walkTimeO, walkTimeN, t );
			float idleTime = (float)( game.accumulator );
			float idleXRot = (float)( Math.Sin( idleTime * idleXPeriod ) * idleMax );
			float idleZRot = (float)( idleMax + Math.Cos( idleTime * idleZPeriod ) * idleMax );
			
			leftArmXRot = (float)( Math.Cos( walkTime ) * swing * armMax ) - idleXRot;
			rightArmXRot = -leftArmXRot;
			rightLegXRot = (float)( Math.Cos( walkTime ) * swing * legMax );
			leftLegXRot = -rightLegXRot;
			rightArmZRot = idleZRot;
			leftArmZRot = -idleZRot;
		}
		
		protected void CheckSkin() {
			DownloadedItem item;
			game.AsyncDownloader.TryGetItem( "skin_" + SkinName, out item );
			if( item != null && item.Data != null ) {
				Bitmap bmp = (Bitmap)item.Data;
				game.Graphics.DeleteTexture( ref PlayerTextureId );
				if( !FastBitmap.CheckFormat( bmp.PixelFormat ) ) {
					Bitmap newBmp = game.Drawer2D.ConvertTo32Bpp( bmp );
					bmp.Dispose();
					bmp = newBmp;
				}
				
				try {
					SkinType = Utils.GetSkinType( bmp );
					PlayerTextureId = game.Graphics.CreateTexture( bmp );				
					MobTextureId = -1;
					
					// Custom mob textures.
					if( Utils.IsUrl( item.Url ) && item.TimeAdded > lastModelChange ) {
						MobTextureId = PlayerTextureId;
					}
					RenderHat = HasHat( bmp, SkinType );
				} catch( NotSupportedException ) {
					string formatString = "Skin {0} has unsupported dimensions({1}, {2}), reverting to default.";
					Utils.LogWarning( formatString, SkinName, bmp.Width, bmp.Height );
					MobTextureId = -1;
					PlayerTextureId = -1;
					SkinType = game.DefaultPlayerSkinType;
					RenderHat = false;
				}
				bmp.Dispose();
			}
			
		}
		
		unsafe bool HasHat( Bitmap bmp, SkinType skinType ) {
			using( FastBitmap fastBmp = new FastBitmap( bmp, true ) ) {
				int sizeX = (bmp.Width / 64) * 32;
				int yScale = skinType == SkinType.Type64x32 ? 32 : 64;
				int sizeY = (bmp.Height / yScale) * 16;
				int fullWhite = FastColour.White.ToArgb();
				int fullBlack = FastColour.Black.ToArgb();
				
				for( int y = 0; y < sizeY; y++ ) {
					int* row = fastBmp.GetRowPtr( y );
					row += sizeX;
					for( int x = 0; x < sizeX; x++ ) {
						int pixel = row[x];
						if( !(pixel == fullWhite || pixel == fullBlack) ) {
							return true;
						}
					}
				}
			}
			return false;
		}
		
		DateTime lastModelChange = new DateTime( 1, 1, 1 );
		public void SetModel( string modelName ) {
			ModelName = modelName;
			Model = game.ModelCache.GetModel( ModelName );
			lastModelChange = DateTime.UtcNow;
			MobTextureId = -1;
		}
	}
}