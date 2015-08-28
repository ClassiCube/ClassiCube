using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		int[] textures = new int[BlocksCount * TileSide.Sides];	
		const int Row1 = 0, Row2 = 16, Row3 = 32, Row4 = 48,
		Row5 = 64, Row6 = 80, Row7 = 96, Row8 = 112, Row9 = 128;

		void SetupOptimTextures() {
			SetOptimAll( Block.Stone, Row1 + 1 );
			SetOptimTop( Block.Grass, Row1 );
			SetOptimSide( Block.Grass, Row1 + 3 );
			SetOptimBottom( Block.Grass, Row1 + 2 );
			SetOptimAll( Block.Dirt, Row1 + 2 );
			SetOptimAll( Block.Cobblestone, Row2 );
			SetOptimAll( Block.WoodenPlanks, Row1 + 4 );
			SetOptimAll( Block.Sapling, Row1 + 15 );
			SetOptimAll( Block.Bedrock, Row2 + 1 );
			SetOptimAll( Block.Water, Row1 + 14 );
			SetOptimAll( Block.StillWater, Row1 + 14 );
			SetOptimAll( Block.Lava, Row2 + 13 );
			SetOptimAll( Block.StillLava, Row2 + 13 );
			SetOptimAll( Block.Sand, Row2 + 2 );
			SetOptimAll( Block.Gravel, Row2 + 3 );
			SetOptimAll( Block.GoldOre, Row2 + 14 );
			SetOptimAll( Block.IronOre, Row2 + 15 );
			SetOptimAll( Block.CoalOre, Row3 );
			SetOptimSide( Block.Wood, Row2 + 4 );
			SetOptimTop( Block.Wood, Row2 + 5 );
			SetOptimBottom( Block.Wood, Row2 + 5 );
			SetOptimAll( Block.Leaves, Row2 + 6 );
			SetOptimAll( Block.Sponge, Row3 + 9);
			SetOptimAll( Block.Glass, Row3 + 10 );
			SetOptimAll( Block.RedCloth, Row4 + 4 );
			SetOptimAll( Block.OrangeCloth, Row4 + 5 );
			SetOptimAll( Block.YellowCloth, Row4 + 6 );
			SetOptimAll( Block.LimeCloth, Row4 + 7 );
			SetOptimAll( Block.GreenCloth, Row4 + 8 );
			SetOptimAll( Block.AquaCloth, Row4 + 9 );
			SetOptimAll( Block.CyanCloth, Row4 + 10 );
			SetOptimAll( Block.BlueCloth, Row4 + 11 );
			SetOptimAll( Block.PurpleCloth, Row4 + 12 );
			SetOptimAll( Block.IndigoCloth, Row4 + 13 );
			SetOptimAll( Block.VioletCloth, Row4 + 14 );
			SetOptimAll( Block.MagentaCloth, Row4 + 15 );
			SetOptimAll( Block.PinkCloth, Row5 );
			SetOptimAll( Block.BlackCloth, Row5 + 1 );
			SetOptimAll( Block.GrayCloth, Row5 + 2 );
			SetOptimAll( Block.WhiteCloth, Row5 + 3 );
			SetOptimAll( Block.Dandelion, Row1 + 13 );
			SetOptimAll( Block.Rose, Row1 + 12 );
			SetOptimAll( Block.BrownMushroom, Row2 + 12 );
			SetOptimAll( Block.RedMushroom, Row2 + 11 );
			SetOptimTop( Block.GoldBlock, Row2 + 8 );
			SetOptimSide( Block.GoldBlock, Row3 + 6 );
			SetOptimBottom( Block.GoldBlock, Row4 + 1 );
			SetOptimTop( Block.IronBlock, Row2 + 7 );
			SetOptimSide( Block.IronBlock, Row3 + 5 );
			SetOptimBottom( Block.IronBlock, Row4 );
			SetOptimTop( Block.DoubleSlab, Row1 + 6 );
			SetOptimSide( Block.DoubleSlab, Row1 + 5 );
			SetOptimBottom( Block.DoubleSlab, Row1 + 6 );
			SetOptimTop( Block.Slab, Row1 + 6 );
			SetOptimSide( Block.Slab, Row1 + 5 );
			SetOptimBottom( Block.Slab, Row1 + 6 );
			SetOptimAll( Block.Brick, Row1 + 7 );
			SetOptimTop( Block.TNT, Row1 + 9 );
			SetOptimSide( Block.TNT, Row1 + 8 );
			SetOptimBottom( Block.TNT, Row1 + 10 );
			SetOptimTop( Block.Bookshelf, Row1 + 4 );
			SetOptimBottom( Block.Bookshelf, Row1 + 4 );
			SetOptimSide( Block.Bookshelf, Row3 + 1 );
			SetOptimAll( Block.MossyCobblestone, Row3 + 2 );
			SetOptimAll( Block.Obsidian, Row3 + 3 );
			
			// CPE blocks
			SetOptimAll( Block.CobblestoneSlab, Row2 + 0 );
			SetOptimAll( Block.Rope, Row1 + 11 );
			SetOptimTop( Block.Sandstone, Row2 + 9 );
			SetOptimSide( Block.Sandstone, Row3 + 7 );
			SetOptimBottom( Block.Sandstone, Row4 + 2 );
			SetOptimAll( Block.Snow, Row3 + 11 );
			SetOptimAll( Block.Fire, Row3 + 4 );
			SetOptimAll( Block.LightPinkWool, Row5 + 4 );
			SetOptimAll( Block.ForestGreenWool, Row5 + 5 );
			SetOptimAll( Block.BrownWool, Row5 + 6 );
			SetOptimAll( Block.DeepBlueWool, Row5 + 7 );
			SetOptimAll( Block.TurquoiseWool, Row5 + 8 );
			SetOptimAll( Block.Ice, Row3 + 12 );
			SetOptimAll( Block.CeramicTile, Row3 + 15 );
			SetOptimAll( Block.Magma, Row5 + 9 );
			SetOptimTop( Block.Pillar, Row2 + 10 );
			SetOptimSide( Block.Pillar, Row3 + 8 );
			SetOptimBottom( Block.Pillar, Row4 + 3 );
			SetOptimAll( Block.Crate, Row3 + 14 );
			SetOptimAll( Block.StoneBrick, Row3 + 13 );
		}

		void SetOptimAll( Block blockId, int textureId ) {
			int index = (byte)blockId * TileSide.Sides;
			for( int i = index; i < index + TileSide.Sides; i++ ) {
				textures[i] = textureId;
			}
		}
		
		void SetOptimSide( Block blockId, int textureId ) {
			int index = (byte)blockId * TileSide.Sides;
			for( int i = index; i < index + TileSide.Bottom; i++ ) {
				textures[i] = textureId;
			}
		}
		
		void SetOptimTop( Block blockId, int textureId ) {
			textures[(byte)blockId * TileSide.Sides + TileSide.Top] = textureId;
		}
		
		void SetOptimBottom( Block blockId, int textureId ) {
			textures[(byte)blockId * TileSide.Sides + TileSide.Bottom] = textureId;
		}
		
		/// <summary> Gets the index in the ***optimised*** 2D terrain atlas for the
		/// texture of the face of the given block. </summary>
		/// <param name="block"> Block ID. </param>
		/// <param name="face">Face of the given block, see TileSide constants. </param>
		/// <returns>The index of the texture within the terrain atlas.</returns>
		public int GetOptimTextureLoc( byte block, int face ) {
			return textures[block * TileSide.Sides + face];
		}
	}
}