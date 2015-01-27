using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		// The designation used is as follows:
		// 0 - left 1 - right 2 - front
		// 3 - back 4 - bottom 5 - top	
		int[] optimTextures = new int[blocksCount * 6];	
		const int Row1 = 0, Row2 = 16, Row3 = 32, Row4 = 48,
		Row5 = 64, Row6 = 80, Row7 = 96, Row8 = 112, Row9 = 128,
		Row10 = 144;
		
		// Although there are 16 rows, the optimised texture
		// only uses the first 10.
		void SetupOptimTextures() {
			// TODO: Snowy grass
			// TODO: Tinted grass
			SetOptimTopTexture( Block.Grass, Row1 );
			SetOptimAllTextures( Block.Stone, Row1 + 1 );
			SetOptimAllTextures( Block.Dirt, Row1 + 2 );
			SetOptimSideTextures( Block.Grass, Row1 + 3 );
			SetOptimAllTextures( Block.WoodenPlanks, Row1 + 4 );
			SetOptimTopTexture( Block.Bookshelf, Row1 + 4 );
			SetOptimBottomTexture( Block.Bookshelf, Row1 + 4 );
			SetOptimSideTextures( Block.DoubleSlabs, Row1 + 5 );
			SetOptimSideTextures( Block.Slabs, Row1 + 5 );
			SetOptimTopTexture( Block.DoubleSlabs, Row1 + 6 );
			SetOptimBottomTexture( Block.DoubleSlabs, Row1 + 6 );
			SetOptimTopTexture( Block.Slabs, Row1 + 6 );
			SetOptimBottomTexture( Block.Slabs, Row1 + 6 );
			SetOptimAllTextures( Block.BrickBlock, Row1 + 7 );
			SetOptimSideTextures( Block.TNT, Row1 + 8 );
			SetOptimTopTexture( Block.TNT, Row1 + 9 );
			SetOptimBottomTexture( Block.TNT, Row1 + 10 );
			SetOptimAllTextures( Block.Cobweb, Row1 + 11 );
			SetOptimAllTextures( Block.Rose, Row1 + 12 );
			SetOptimAllTextures( Block.Dandelion, Row1 + 13 );
			SetOptimAllTextures( Block.Sapling, Row1 + 14 );
			SetOptimAllTextures( Block.Cobblestone, Row1 + 15 );
			SetOptimAllTextures( Block.Bedrock, Row2 );
			SetOptimAllTextures( Block.Sand, Row2 + 1 );
			SetOptimAllTextures( Block.Gravel, Row2 + 2 );
			// TODO: Alternate forms
			SetOptimSideTextures( Block.Wood, Row2 + 3 );
			SetOptimTopTexture( Block.Wood, Row2 + 4 );
			SetOptimBottomTexture( Block.Wood, Row2 + 4 );
			SetOptimAllTextures( Block.BlockOfIron, Row2 + 5 );
			SetOptimAllTextures( Block.BlockOfGold, Row2 + 6 );
			SetOptimAllTextures( Block.BlockOfDiamond, Row2 + 7 );
			// TODO: Chest front, double chests
			SetOptimAllTextures( Block.Chest, Row2 + 8 );
			SetOptimSideTextures( Block.Chest, Row2 + 9 );
			SetOptimAllTextures( Block.RedMushroom, Row2 + 11 );
			SetOptimAllTextures( Block.BrownMushroom, Row2 + 12 );
			// TODO: Fire
			SetOptimAllTextures( Block.Fire, Row2 + 13 );
			SetOptimAllTextures( Block.GoldOre, Row2 + 14 );
			SetOptimAllTextures( Block.IronOre, Row2 + 15 );
			SetOptimAllTextures( Block.CoalOre, Row3 );
			SetOptimSideTextures( Block.Bookshelf, Row3 + 1 );
			SetOptimAllTextures( Block.MossStone, Row3 + 2 );
			SetOptimAllTextures( Block.Obsidian, Row3 + 3 );
			SetOptimAllTextures( Block.TallGrass, Row3 + 5 );
			// TODO: Crafting table bottom
			SetOptimAllTextures( Block.CraftingTable, Row3 + 8 );
			// TODO: Furnace front
			SetOptimSideTextures( Block.Furnace, Row3 + 9 );
			// TODO: Dispenser front
			SetOptimSideTextures( Block.Dispenser, Row3 + 11 );
			SetOptimAllTextures( Block.Sponge, Row3 + 12 );
			SetOptimAllTextures( Block.Glass, Row3 + 13 );
			SetOptimAllTextures( Block.DiamondOre, Row3 + 14 );
			SetOptimAllTextures( Block.RedstoneOre, Row3 + 15 );
			// TODO: Alternate forms
			SetOptimAllTextures( Block.Leaves, Row4 );
			SetOptimAllTextures( Block.DeadShrubs, Row4 + 2 );
			SetOptimSideTextures( Block.CraftingTable, Row4 + 7 );
			SetOptimSideTextures( Block.BurningFurnace, Row4 + 8 );
			SetOptimTopTexture( Block.Dispenser, Row4 + 9 );
			SetOptimTopTexture( Block.BurningFurnace, Row4 + 9 );
			SetOptimTopTexture( Block.Furnace, Row4 + 9 );
			SetOptimBottomTexture( Block.Dispenser, Row4 + 9 );
			SetOptimBottomTexture( Block.BurningFurnace, Row4 + 9 );
			SetOptimBottomTexture( Block.Furnace, Row4 + 9 );
			// TODO: Alternate forms
			SetOptimAllTextures( Block.Wool, Row4 + 11 );
			SetOptimAllTextures( Block.MonsterSpawner, Row4 + 12 );
			SetOptimAllTextures( Block.Snow, Row4 + 13 );
			SetOptimAllTextures( Block.SnowBlock, Row4 + 14 );
			SetOptimAllTextures( Block.Ice, Row4 + 14 );
			SetOptimTopTexture( Block.Cactus, Row5 );
			SetOptimSideTextures( Block.Cactus, Row5 + 1 );
			SetOptimBottomTexture( Block.Cactus, Row5 + 2 );
			SetOptimAllTextures( Block.ClayBlock, Row5 + 3 );
			SetOptimAllTextures( Block.SugarCane, Row5 + 4 );
			// TODO: what textures do noteblock and jukebox use?
			SetOptimAllTextures( Block.NoteBlock, Row5 + 5 );
			SetOptimAllTextures( Block.Jukebox, Row5 + 5 );
			SetOptimTopTexture( Block.Jukebox, Row5 + 6 );
			SetOptimAllTextures( Block.Torch, Row5 + 8 );
			// TODO: Doors
			SetOptimAllTextures( Block.WoodenDoor, Row5 + 9 );
			SetOptimAllTextures( Block.IronDoor, Row5 + 10 );
			SetOptimAllTextures( Block.Ladders, Row5 + 11 );
			SetOptimAllTextures( Block.Trapdoor, Row5 + 12 );
			// TODO: Wet farmland
			SetOptimAllTextures( Block.Farmland, Row5 + 13 );
			// TODO: Crop stages.
			SetOptimAllTextures( Block.Seeds, Row5 + 15 );
			SetOptimAllTextures( Block.Lever, Row6 + 7 );
			SetOptimAllTextures( Block.RedstoneTorchOn, Row6 + 10 );
			// TODO: Pumpkins and jack o lanterns properly
			SetOptimAllTextures( Block.Pumpkin, Row6 + 11 );
			SetOptimAllTextures( Block.JackOLantern, Row6 + 11 );
			SetOptimAllTextures( Block.Netherrack, Row6 + 12 );
			SetOptimAllTextures( Block.SoulSand, Row6 + 13 );
			SetOptimAllTextures( Block.GlowstoneBlock, Row6 + 14 );
			// TODO: Pistons properly
			SetOptimAllTextures( Block.StickyPiston, Row6 + 15 );
			SetOptimAllTextures( Block.Piston, Row7 );
			SetOptimAllTextures( Block.RedstoneTorchOff, Row7 + 7 );
			// TODO: Eaten cake
			SetOptimTopTexture( Block.CakeBlock, Row7 + 13 );
			SetOptimSideTextures( Block.CakeBlock, Row7 + 14 );
			SetOptimBottomTexture( Block.CakeBlock, Row8 );
			// TODO: Rails
			SetOptimAllTextures( Block.Rails, Row8 + 1 );
			// TODO: Repeaters off
			SetOptimAllTextures( Block.RedstoneRepeaterOff, Row8 + 4 );
			// TODO: Beds
			SetOptimAllTextures( Block.Bed, Row8 + 7 );
			SetOptimAllTextures( Block.LapisLazuliBlock, Row8 + 9 );
			// TODO: Repeaters on
			SetOptimAllTextures( Block.RedstoneRepeaterOn, Row8 + 12 );
			SetOptimAllTextures( Block.LapisLazuliOre, Row9 + 1 );
			// TODO: Powered rail on
			SetOptimAllTextures( Block.PoweredRail, Row9 + 4 );
			// TODO: Redstone wire
			SetOptimAllTextures( Block.RedstoneWire, Row9 + 5 );
			SetOptimTopTexture( Block.Sandstone, Row9 + 7 );
			SetOptimSideTextures( Block.Sandstone, Row9 + 11 );
			// TODO: Detector rail
			SetOptimAllTextures( Block.DetectorRail, Row9 + 14 );
			// TODO: Water
			SetOptimAllTextures( Block.Water, Row9 + 15 );
			SetOptimAllTextures( Block.StillWater, Row9 + 15 );
			SetOptimBottomTexture( Block.Sandstone, Row10 );
			// TODO: Lava
			SetOptimAllTextures( Block.Lava, Row10 + 4 );
			SetOptimAllTextures( Block.StillLava, Row10 + 4 );
		}

		void SetOptimAllTextures( Block blockId, int textureId ) {
			int index = (byte)blockId * 6;
			for( int i = index; i < index + 6; i++ ) {
				optimTextures[i] = textureId;
			}
		}
		
		void SetOptimSideTextures( Block blockId, int textureId ) {
			int index = (byte)blockId * 6;
			for( int i = index; i < index + TileSide.Top; i++ ) {
				optimTextures[i] = textureId;
			}
		}
		
		void SetOptimTopTexture( Block blockId, int textureId ) {
			optimTextures[(byte)blockId * 6 + TileSide.Top] = textureId;
		}
		
		void SetOptimBottomTexture( Block blockId, int textureId ) {
			optimTextures[(byte)blockId * 6 + TileSide.Bottom] = textureId;
		}
		
		/// <summary> Gets the index in the ***optimised*** 2D terrain atlas for the
		/// texture of the face of the given block. </summary>
		/// <param name="block"> Block ID. </param>
		/// <param name="face">Face of the given block, see TileSide constants. </param>
		/// <returns>The index of the texture within the terrain atlas.</returns>
		public int GetOptimTextureLoc( byte block, int face ) {
			return optimTextures[block * 6 + face];
		}
		
		static ushort[] RowFlags = new ushort[] {
			0xFFFD, // y y y y  y y y y  y y y y  y y n y (row 1)
			0xFFFD, // y y y y  y y y y  y y y y  y y n y (row 2)
			0xFF7E, // y y y y  y y y y  n y y y  y y y n (row 3)
			0xFDFF, // y y y y  y y n y  y y y y  y y y y (row 4)
			0xFFF1, // y y y y  y y y y  y y y y  n n n y (row 5)
			0xFBFF, // y y y y  y n y y  y y y y  y y y y (row 6)			
			0xF3FE, // y y y y  n n y y  y y y y  y y y n (row 7)
			0xFFF8, // y y y y  y y y y  y y y y  y n n n (row 8)			
			0xFF00, // y y y y  y y y y  n n n n  n n n n (row 9)
			0xF780, // y y y y  n y y y  y n n n  n n n n (row 10)			
			0xFC00, // y y y y  y y n n  n n n n  n n n n (row 11)
			0xF000, // y y y y  n n n n  n n n n  n n n n (row 12)
			0xF001, // y y y y  n n n n  n n n n  n n n y (row 13)
			0xE000, // y y y n  n n n n  n n n n  n n n n (row 14)
			0x4001, // n y n n  n n n n  n n n n  n n n y (row 15)
			0xFFC0, // y y y y  y y y y  y y n n  n n n n (row 16)
		};
		
		public static void MakeOptimisedTexture( FastBitmap atlas ) {
			int size = atlas.Width / 16;
			int srcIndex = 0, destIndex = 0;
			
			for( int y = 0; y < 16; y++ ) {
				int flags = RowFlags[y];
				for( int x = 0; x < 16; x++ ) {
					bool isUsed = ( flags & 1 << ( 15 - x ) ) != 0;
					if( isUsed && srcIndex != destIndex ) {
						int srcX = x * size;
						int srcY = y * size;
						int destX = ( destIndex & 0x0F ) * size;
						int destY = ( destIndex >> 4 ) * size;
						MovePortion( srcX, srcY, destX, destY, atlas, size );
					}
					
					srcIndex++;
					destIndex++;
					if( !isUsed ) destIndex--;
				}
			}
		}
		
		unsafe static void MovePortion( int srcX, int srcY, int dstX, int dstY, FastBitmap src, int size ) {
			for( int y = 0; y < size; y++ ) {
				int* srcRow = src.GetRowPtr( srcY + y );
				int* dstRow = src.GetRowPtr( dstY + y );
				for( int x = 0; x < size; x++ ) {					
					dstRow[dstX + x] = srcRow[srcX + x];
				}
			}
		}
	}
}