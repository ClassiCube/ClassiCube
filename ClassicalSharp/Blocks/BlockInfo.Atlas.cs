// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		internal byte[] textures = new byte[BlocksCount * Side.Sides];

		void SetupTextures() {
			// Row 1
			SetAll( Block.Grass ); SetAll( Block.Stone );
			SetAll( Block.Dirt ); SetSide( Block.Grass );
			SetTex( 0 + 2, Side.Bottom, Block.Grass ); SetAll( Block.Wood );
			SetSide( Block.DoubleSlab ); SetSide( 0 + 5, Block.Slab );
			SetTopAndBottom( Block.DoubleSlab ); SetTopAndBottom( 0 + 6, Block.Slab );
			SetAll( Block.Brick ); SetSide( Block.TNT );
			SetTop( Block.TNT ); SetBottom( Block.TNT );
			SetAll( Block.Rope ); SetAll( Block.Rose ); SetAll( Block.Dandelion );
			SetAll( Block.Water ); SetAll( Block.Sapling );
			SetAll( 0 + 14, Block.StillWater );
			// Row 2
			SetAll( Block.Cobblestone ); SetAll( Block.Bedrock );
			SetAll( Block.Sand ); SetAll( Block.Gravel );
			SetAll( 16 + 0, Block.CobblestoneSlab ); SetSide( Block.Log );
			SetTopAndBottom( Block.Log ); SetAll( Block.Leaves );
			SetTop( Block.Iron ); SetTop( Block.Gold );
			SetTop( Block.Sandstone ); SetTop( Block.Pillar );
			texId += 1;
			SetAll( Block.RedMushroom ); SetAll( Block.BrownMushroom );
			SetAll( Block.Lava ); SetAll( 16 + 14, Block.StillLava );
			texId += 1;
			// Row 3
			SetAll( Block.GoldOre ); SetAll( Block.IronOre );
			SetAll( Block.CoalOre ); SetAll( Block.Bookshelf );
			SetAll(  Block.MossyRocks ); SetAll( Block.Obsidian );
			SetAll(	Block.Fire ); SetTopAndBottom( 0 + 4, Block.Bookshelf );
			SetSide( Block.Iron ); SetSide( Block.Gold );
			SetSide( Block.Sandstone ); SetSide( Block.Pillar );
			texId += 5;
			// Row 4
			SetAll( Block.Sponge ); SetAll( Block.Glass ); 
			SetAll( Block.Snow ); SetAll( Block.Ice ); 
			SetAll( Block.StoneBrick ); SetAll( Block.Crate ); 
			SetAll( Block.CeramicTile ); SetBottom( Block.Iron ); 
			SetBottom( Block.Gold ); SetBottom( Block.Sandstone ); 
			SetBottom( Block.Pillar );
			texId += 5;
			// Row 5
			SetAll( Block.Red ); SetAll( Block.Orange ); 
			SetAll( Block.Yellow ); SetAll( Block.Lime ); 
			SetAll( Block.Green ); SetAll( Block.Teal ); 
			SetAll( Block.Aqua ); SetAll( Block.Cyan ); 
			SetAll( Block.Blue ); SetAll( Block.Indigo ); 
			SetAll( Block.Violet ); SetAll( Block.Magenta ); 
			SetAll( Block.Pink ); SetAll( Block.Black );
			SetAll( Block.Gray ); SetAll( Block.White );
			// Row 6
			SetAll( Block.LightPink ); SetAll( Block.ForestGreen );
			SetAll( Block.Brown ); SetAll( Block.DeepBlue );
			SetAll( Block.Turquoise );
			texId += 1;
			SetAll( Block.Magma );
		}
		
		void SetAll( int textureId, byte blockId ) {
			int index = blockId * Side.Sides;
			for( int i = index; i < index + Side.Sides; i++ ) {
				textures[i] = (byte)textureId;
			}
		}
		
		internal void SetSide( int textureId, byte blockId ) {
			int index = blockId * Side.Sides;
			for( int i = index; i < index + Side.Bottom; i++ )
				textures[i] = (byte)textureId;
		}
		
		internal void SetTopAndBottom( int textureId, byte blockId ) {
			textures[blockId * Side.Sides + Side.Bottom] = (byte)textureId;
			textures[blockId * Side.Sides + Side.Top] = (byte)textureId;
		}
		
		internal void SetTex( int textureId, int face, byte blockId ) {
			textures[blockId * Side.Sides + face] = (byte)textureId;
		}

		int texId;
		void SetAll( byte blockId ) {
			SetAll( texId, blockId ); texId++;
		}
		
		void SetSide( byte blockId ) {
			SetSide( texId, blockId ); texId++;
		}
		
		void SetTopAndBottom( byte blockId ) {
			SetTopAndBottom( texId, blockId ); texId++;
		}
		
		void SetTop( byte blockId ) {
			SetTex( texId, Side.Top, blockId ); texId++;
		}
		
		void SetBottom( byte blockId ) {
			SetTex( texId, Side.Bottom, blockId ); texId++;
		}
		
		/// <summary> Gets the index in the terrain atlas for the texture of the face of the given block. </summary>
		/// <param name="face"> Face of the given block, see TileSide constants. </param>
		public int GetTextureLoc( byte block, int face ) {
			return textures[block * Side.Sides + face];
		}
		
		void GetTextureRegion( byte block, int side, out Vector2 min, out Vector2 max ) {
			min = Vector2.Zero; max = Vector2.One;
			Vector3 bbMin = MinBB[block], bbMax = MaxBB[block];		
			switch( side ) {
				case Side.Left:
				case Side.Right:
					min = new Vector2( bbMin.Z, bbMin.Y );
					max = new Vector2( bbMax.Z, bbMax.Y );
					if( IsLiquid[block] ) { min.Y -= 1.5f/16; max.Y -= 1.5f/16; }
					break;
				case Side.Front:
				case Side.Back: 
					min = new Vector2( bbMin.X, bbMin.Y );
					max = new Vector2( bbMax.X, bbMax.Y );
					if( IsLiquid[block] ) { min.Y -= 1.5f/16; max.Y -= 1.5f/16; }
					break;
				case Side.Top:
				case Side.Bottom:
					min = new Vector2( bbMin.X, bbMin.Z );
					max = new Vector2( bbMax.X, bbMax.Z ); 
					break;
			}
		}
		
		bool FaceOccluded( byte block, byte other, int side ) {
			Vector2 bMin, bMax, oMin, oMax;
			GetTextureRegion( block, side, out bMin, out bMax );
			GetTextureRegion( other, side, out oMin, out oMax );
			return bMin.X >= oMin.X && bMin.Y >= oMin.Y
				&& bMax.X <= oMax.X && bMax.Y <= oMax.Y;
		}
	}
}