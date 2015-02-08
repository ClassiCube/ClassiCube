using System;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp {

	public abstract class Player : LivingEntity {
		
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
		
		public string DisplayName, SkinName;
		protected PlayerRenderer renderer;
		public SkinType SkinType;
		
		public Player( Game window ) : base( window ) {
			SkinType = game.DefaultPlayerSkinType;
			SetModel( "humanoid" );
		}
		
		/// <summary> Gets the block just underneath the player's feet position. </summary>
		public BlockId BlockUnderFeet {
			get {
				if( map == null || map.IsNotLoaded ) return BlockId.Air;
				Vector3I blockCoords = Vector3I.Floor( Position.X, Position.Y - 0.01f, Position.Z );
				if( !map.IsValidPos( blockCoords ) ) return BlockId.Air;
				return (BlockId)map.GetBlock( blockCoords );
			}
		}
		
		/// <summary> Gets the block at player's eye position. </summary>
		public BlockId BlockAtHead {
			get {
				if( map == null || map.IsNotLoaded ) return BlockId.Air;
				Vector3I blockCoords = Vector3I.Floor( EyePosition );
				if( !map.IsValidPos( blockCoords ) ) return BlockId.Air;
				return (BlockId)map.GetBlock( blockCoords );
			}
		}
	}
}