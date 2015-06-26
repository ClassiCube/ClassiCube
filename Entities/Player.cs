using System;
using System.Drawing;
using OpenTK;
using ClassicalSharp.Network;
using ClassicalSharp.Model;
using ClassicalSharp.Renderers;

namespace ClassicalSharp {

	public abstract class Player : Entity {
		
		public const float Width = 0.6f;
		public const float EyeHeight = 1.625f;
		public const float Height = 1.8f;
		public const float Depth = 0.6f;

		public override Vector3 Size {
			get { return new Vector3( Width, Height, Depth ); }
		}
		
		/// <summary> Gets the position of the player's eye in the world. </summary>
		public Vector3 EyePosition {
			get { return new Vector3( Position.X, Position.Y + EyeHeight, Position.Z ); }
		}
		
		public override float StepSize {
			get { return 0.5f; }
		}
		
		public Game Window;
		public byte ID;
		public string DisplayName, SkinName;
		public string ModelName;
		public IModel Model;
		protected PlayerRenderer renderer;
		public SkinType SkinType;
		
		public Player( byte id, Game window ) : base( window ) {
			ID = id;
			Window = window;
			SkinType = Window.DefaultPlayerSkinType;
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
			if( map == null || map.IsNotLoaded ) return Block.Air;
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
			bool moves = distance > 0.05;
			
			if( moves ) {
				walkTimeN += (float)distance * 2 * (float)( 20 * delta );
				swingN += (float)delta * 3;
				if( swingN > 1 ) swingN = 1;
			} else {
				swingN -= (float)delta * 3;
				if( swingN < 0 ) swingN = 0;
			}
		}
		
		const float armMax = (float)( 90 * Math.PI / 180.0 );
		const float legMax = (float)( 80 * Math.PI / 180.0 );
		const float idleMax = (float)( 3 * Math.PI / 180.0 );
		const float idleXPeriod = (float)( 2 * Math.PI / 5f );
		const float idleZPeriod = (float)( 2 * Math.PI / 3.5f );
		protected void SetCurrentAnimState( float t ) {
			float swing = Utils.Lerp( swingO, swingN, t );
			float walkTime = Utils.Lerp( walkTimeO, walkTimeN, t );
			float idleTime = (float)( Window.accumulator );
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
			Window.AsyncDownloader.TryGetItem( "skin_" + SkinName, out item );
			if( item != null && item.Bmp != null ) {
				Bitmap bmp = item.Bmp;
				Window.Graphics.DeleteTexture( ref renderer.PlayerTextureId );
				
				try {
					SkinType = Utils.GetSkinType( bmp );
					renderer.PlayerTextureId = Window.Graphics.LoadTexture( bmp );				
					renderer.MobTextureId = -1;
					
					// Custom mob textures.
					if( Utils.IsUrl( item.Url ) && item.TimeAdded > lastModelChange ) {
						renderer.MobTextureId = renderer.PlayerTextureId;
					}
				} catch( NotSupportedException ) {
					string formatString = "Skin {0} has unsupported dimensions({1}, {2}), reverting to default.";
					Utils.LogWarning( formatString, SkinName, bmp.Width, bmp.Height );
					renderer.MobTextureId = -1;
					renderer.PlayerTextureId = -1;
					SkinType = Window.DefaultPlayerSkinType;
				}
				bmp.Dispose();
			}
			
		}
		
		DateTime lastModelChange = new DateTime( 1, 1, 1 );
		public void SetModel( string modelName ) {
			ModelName = modelName;
			Model = Window.ModelCache.GetModel( ModelName );
			lastModelChange = DateTime.UtcNow;
			if( renderer != null ) {
				renderer.MobTextureId = -1;
			}
		}
	}
}