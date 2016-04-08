// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp {
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		internal int[] textures = new int[BlocksCount * TileSide.Sides];

		void SetupTextures() {
			// Row 1
			SetAll( Block.Grass ); SetAll( Block.Stone );
			SetAll( Block.Dirt ); SetSide( Block.Grass );
			SetTex( 0 + 2, TileSide.Bottom, Block.Grass ); SetAll( Block.Wood );
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
		
		void SetAll( int textureId, Block blockId ) {
			int index = (byte)blockId * TileSide.Sides;
			for( int i = index; i < index + TileSide.Sides; i++ ) {
				textures[i] = textureId;
			}
		}
		
		internal void SetSide( int textureId, Block blockId ) {
			int index = (byte)blockId * TileSide.Sides;
			for( int i = index; i < index + TileSide.Bottom; i++ )
				textures[i] = textureId;
		}
		
		internal void SetTopAndBottom( int textureId, Block blockId ) {
			textures[(byte)blockId * TileSide.Sides + TileSide.Bottom] = textureId;
			textures[(byte)blockId * TileSide.Sides + TileSide.Top] = textureId;
		}
		
		internal void SetTex( int textureId, int face, Block blockId ) {
			textures[(byte)blockId * TileSide.Sides + face] = textureId;
		}

		int texId;
		void SetAll( Block blockId ) {
			SetAll( texId, blockId ); texId++;
		}
		
		void SetSide( Block blockId ) {
			SetSide( texId, blockId ); texId++;
		}
		
		void SetTopAndBottom( Block blockId ) {
			SetTopAndBottom( texId, blockId ); texId++;
		}
		
		void SetTop( Block blockId ) {
			SetTex( texId, TileSide.Top, blockId ); texId++;
		}
		
		void SetBottom( Block blockId ) {
			SetTex( texId, TileSide.Bottom, blockId ); texId++;
		}
		
		/// <summary> Gets the index in the terrain atlas for the texture of the face of the given block. </summary>
		/// <param name="face"> Face of the given block, see TileSide constants. </param>
		public int GetTextureLoc( byte block, int face ) {
			return textures[block * TileSide.Sides + face];
		}
	}
}