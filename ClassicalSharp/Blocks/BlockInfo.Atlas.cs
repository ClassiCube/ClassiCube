using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		int[] textures = new int[BlocksCount * TileSide.Sides];	

		void SetupOptimTextures() {
			// Row 1
			SetAll( Block.Grass, Block.Stone, Block.Dirt );
			SetSide( Block.Grass );
			SetBottom( 0 + 2, Block.Grass );
			SetAll( Block.WoodenPlanks );
			SetSide( Block.DoubleSlab );
			SetSide( 0 + 5, Block.Slab );
			SetTopAndBottom( Block.DoubleSlab );
			SetTopAndBottom( 0 + 6, Block.Slab );
			SetAll( Block.Brick );
			SetSide( Block.TNT );
			SetTop( Block.TNT );
			SetBottom( Block.TNT );
			SetAll( Block.Rope, Block.Rose, Block.Dandelion, Block.Water, Block.Sapling );
			SetAll( 0 + 14, Block.StillWater );
			// Row 2
			SetAll( Block.Cobblestone, Block.Bedrock, Block.Sand, Block.Gravel );
			SetAll( 16 + 0, Block.CobblestoneSlab );
			SetSide( Block.Wood );
			SetTopAndBottom( Block.Wood );
			SetAll( Block.Leaves );
			SetTop( Block.IronBlock, Block.GoldBlock, Block.Sandstone, Block.Pillar );
			texId += 1;
			SetAll( Block.RedMushroom, Block.BrownMushroom, Block.Lava );
			SetAll( 16 + 14, Block.StillLava );
			texId += 1;
			// Row 3
			SetAll( Block.GoldOre, Block.IronOre, Block.CoalOre, Block.Bookshelf, 
			       Block.MossyCobblestone, Block.Obsidian, Block.Fire );
			SetTopAndBottom( 0 + 4, Block.Bookshelf );
			SetSide( Block.IronBlock, Block.GoldBlock, Block.Sandstone, Block.Pillar );
			texId += 5;
			// Row 4
			SetAll( Block.Sponge, Block.Glass, Block.Snow, Block.Ice, Block.StoneBrick, 
			       Block.Crate, Block.CeramicTile );
			SetBottom( Block.IronBlock, Block.GoldBlock, Block.Sandstone, Block.Pillar );
			texId += 5;
			// Row 5
			SetAll( Block.RedCloth, Block.OrangeCloth, Block.YellowCloth, Block.LimeCloth, 
			       Block.GreenCloth, Block.AquaCloth, Block.CyanCloth, Block.BlueCloth, 
			       Block.PurpleCloth, Block.IndigoCloth, Block.VioletCloth, Block.MagentaCloth,
			       Block.PinkCloth, Block.BlackCloth, Block.GrayCloth, Block.WhiteCloth );
			// Row 6
			SetAll( Block.LightPinkWool, Block.ForestGreenWool, Block.BrownWool,
			       Block.DeepBlueWool, Block.TurquoiseWool );
			texId += 1;
			SetAll( Block.Magma );
		}
		
		void SetAll( int textureId, Block blockId ) {
			int index = (byte)blockId * TileSide.Sides;
			for( int i = index; i < index + TileSide.Sides; i++ ) {
				textures[i] = textureId;
			}
		}
		
		void SetSide( int textureId, Block blockId ) {
			int index = (byte)blockId * TileSide.Sides;
			for( int i = index; i < index + TileSide.Bottom; i++ ) {
				textures[i] = textureId;
			}
		}		
		
		void SetTopAndBottom( int textureId, Block blockId ) {
			textures[(byte)blockId * TileSide.Sides + TileSide.Bottom] = textureId;
			textures[(byte)blockId * TileSide.Sides + TileSide.Top] = textureId;
		}
		
		void SetTop( int textureId, Block blockId ) {
			textures[(byte)blockId * TileSide.Sides + TileSide.Top] = textureId;
		}
		
		void SetBottom( int textureId, Block blockId ) {
			textures[(byte)blockId * TileSide.Sides + TileSide.Bottom] = textureId;
		}

		int texId;
		void SetAll( params Block[] blocks ) {
			for( int i = 0; i < blocks.Length; i++ ) {
				SetAll( texId, blocks[i] ); texId++;
			}
		}
		
		void SetSide( params Block[] blocks ) {
			for( int i = 0; i < blocks.Length; i++ ) {
				SetSide( texId, blocks[i] ); texId++;
			}
		}
		
		void SetTopAndBottom( params Block[] blocks ) {
			for( int i = 0; i < blocks.Length; i++ ) {
				SetTopAndBottom( texId, blocks[i] ); texId++;
			}
		}
		
		void SetTop( params Block[] blocks ) {
			for( int i = 0; i < blocks.Length; i++ ) {
				SetTop( texId, blocks[i] ); texId++;
			}
		}
		
		void SetBottom( params Block[] blocks ) {
			for( int i = 0; i < blocks.Length; i++ ) {
				SetBottom( texId, blocks[i] ); texId++;
			}
		}
		
		/// <summary> Gets the index in the terrain atlas for the texture of the face of the given block. </summary>
		/// <param name="face"> Face of the given block, see TileSide constants. </param>
		public int GetTextureLoc( byte block, int face ) {
			return textures[block * TileSide.Sides + face];
		}
	}
}