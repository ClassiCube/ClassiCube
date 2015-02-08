using System;

namespace ClassicalSharp {
	
	public partial class BlockInfo {
		
		int[] optimTextures = new int[blocksCount * 6];	
		const int Row1 = 0, Row2 = 16, Row3 = 32, Row4 = 48,
		Row5 = 64, Row6 = 80, Row7 = 96, Row8 = 112, Row9 = 128,
		Row10 = 144;
		
		// Although there are 16 rows, the optimised texture
		// only uses the first 10.
		void SetupOptimTextures() {
			// TODO: Snowy grass
			// TODO: Tinted grass
			SetOptimTopTexture( BlockId.Grass, Row1 );
			SetOptimAllTextures( BlockId.Stone, Row1 + 1 );
			SetOptimAllTextures( BlockId.Dirt, Row1 + 2 );
			SetOptimAllTextures( BlockId.Farmland, Row1 + 2 );
			SetOptimSideTextures( BlockId.Grass, Row1 + 3 );
			SetOptimBottomTexture( BlockId.Grass, Row1 + 3 );
			SetOptimAllTextures( BlockId.WoodenPlanks, Row1 + 4 );
			SetOptimAllTextures( BlockId.Fence, Row1 + 4 );
			SetOptimTopTexture( BlockId.Bookshelf, Row1 + 4 );
			SetOptimBottomTexture( BlockId.Bookshelf, Row1 + 4 );
			SetOptimSideTextures( BlockId.DoubleSlabs, Row1 + 5 );
			SetOptimSideTextures( BlockId.Slabs, Row1 + 5 );
			SetOptimTopTexture( BlockId.DoubleSlabs, Row1 + 6 );
			SetOptimBottomTexture( BlockId.DoubleSlabs, Row1 + 6 );
			SetOptimTopTexture( BlockId.Slabs, Row1 + 6 );
			SetOptimBottomTexture( BlockId.Slabs, Row1 + 6 );
			SetOptimAllTextures( BlockId.BrickBlock, Row1 + 7 );
			SetOptimSideTextures( BlockId.TNT, Row1 + 8 );
			SetOptimTopTexture( BlockId.TNT, Row1 + 9 );
			SetOptimBottomTexture( BlockId.TNT, Row1 + 10 );
			SetOptimAllTextures( BlockId.Cobweb, Row1 + 11 );
			SetOptimAllTextures( BlockId.Rose, Row1 + 12 );
			SetOptimAllTextures( BlockId.Dandelion, Row1 + 13 );
			SetOptimAllTextures( BlockId.Sapling, Row1 + 14 );
			SetOptimAllTextures( BlockId.Cobblestone, Row1 + 15 );
			SetOptimAllTextures( BlockId.Bedrock, Row2 );
			SetOptimAllTextures( BlockId.Sand, Row2 + 1 );
			SetOptimAllTextures( BlockId.Gravel, Row2 + 2 );
			// TODO: Alternate forms
			SetOptimSideTextures( BlockId.Wood, Row2 + 3 );
			SetOptimTopTexture( BlockId.Wood, Row2 + 4 );
			SetOptimBottomTexture( BlockId.Wood, Row2 + 4 );
			SetOptimAllTextures( BlockId.BlockOfIron, Row2 + 5 );
			SetOptimAllTextures( BlockId.BlockOfGold, Row2 + 6 );
			SetOptimAllTextures( BlockId.BlockOfDiamond, Row2 + 7 );
			// TODO: Chest front, double chests
			SetOptimAllTextures( BlockId.Chest, Row2 + 8 );
			SetOptimSideTextures( BlockId.Chest, Row2 + 9 );
			SetOptimAllTextures( BlockId.RedMushroom, Row2 + 11 );
			SetOptimAllTextures( BlockId.BrownMushroom, Row2 + 12 );
			// TODO: Fire
			SetOptimAllTextures( BlockId.Fire, Row2 + 13 );
			SetOptimAllTextures( BlockId.GoldOre, Row2 + 14 );
			SetOptimAllTextures( BlockId.IronOre, Row2 + 15 );
			SetOptimAllTextures( BlockId.CoalOre, Row3 );
			SetOptimSideTextures( BlockId.Bookshelf, Row3 + 1 );
			SetOptimAllTextures( BlockId.MossStone, Row3 + 2 );
			SetOptimAllTextures( BlockId.Obsidian, Row3 + 3 );
			SetOptimAllTextures( BlockId.TallGrass, Row3 + 5 );
			// TODO: Crafting table bottom
			SetOptimAllTextures( BlockId.CraftingTable, Row3 + 8 );
			// TODO: Furnace front
			SetOptimSideTextures( BlockId.Furnace, Row3 + 9 );
			// TODO: Dispenser front
			SetOptimSideTextures( BlockId.Dispenser, Row3 + 11 );
			SetOptimAllTextures( BlockId.Sponge, Row3 + 12 );
			SetOptimAllTextures( BlockId.Glass, Row3 + 13 );
			SetOptimAllTextures( BlockId.DiamondOre, Row3 + 14 );
			SetOptimAllTextures( BlockId.RedstoneOre, Row3 + 15 );
			// TODO: Alternate forms
			SetOptimAllTextures( BlockId.Leaves, Row4 );
			SetOptimAllTextures( BlockId.DeadShrubs, Row4 + 2 );
			SetOptimSideTextures( BlockId.CraftingTable, Row4 + 7 );
			SetOptimSideTextures( BlockId.BurningFurnace, Row4 + 8 );
			SetOptimTopTexture( BlockId.Dispenser, Row4 + 9 );
			SetOptimTopTexture( BlockId.BurningFurnace, Row4 + 9 );
			SetOptimTopTexture( BlockId.Furnace, Row4 + 9 );
			SetOptimBottomTexture( BlockId.Dispenser, Row4 + 9 );
			SetOptimBottomTexture( BlockId.BurningFurnace, Row4 + 9 );
			SetOptimBottomTexture( BlockId.Furnace, Row4 + 9 );
			// TODO: Alternate forms
			SetOptimAllTextures( BlockId.Wool, Row4 + 11 );
			SetOptimAllTextures( BlockId.MonsterSpawner, Row4 + 12 );
			SetOptimAllTextures( BlockId.Snow, Row4 + 13 );
			SetOptimAllTextures( BlockId.SnowBlock, Row4 + 14 );
			SetOptimAllTextures( BlockId.Ice, Row4 + 14 );
			SetOptimTopTexture( BlockId.Cactus, Row5 );
			SetOptimSideTextures( BlockId.Cactus, Row5 + 1 );
			SetOptimBottomTexture( BlockId.Cactus, Row5 + 2 );
			SetOptimAllTextures( BlockId.ClayBlock, Row5 + 3 );
			SetOptimAllTextures( BlockId.SugarCane, Row5 + 4 );
			// TODO: what textures do noteblock and jukebox use?
			SetOptimAllTextures( BlockId.NoteBlock, Row5 + 5 );
			SetOptimAllTextures( BlockId.Jukebox, Row5 + 5 );
			SetOptimTopTexture( BlockId.Jukebox, Row5 + 6 );
			SetOptimAllTextures( BlockId.Torch, Row5 + 8 );
			// TODO: Doors
			SetOptimAllTextures( BlockId.WoodenDoor, Row5 + 9 );
			SetOptimAllTextures( BlockId.IronDoor, Row5 + 10 );
			SetOptimAllTextures( BlockId.Ladders, Row5 + 11 );
			SetOptimAllTextures( BlockId.Trapdoor, Row5 + 12 );
			// TODO: Wet farmland
			SetOptimTopTexture( BlockId.Farmland, Row5 + 13 );
			// TODO: Crop stages.
			SetOptimAllTextures( BlockId.Seeds, Row5 + 15 );
			SetOptimAllTextures( BlockId.Lever, Row6 + 7 );
			SetOptimAllTextures( BlockId.RedstoneTorchOn, Row6 + 10 );
			// TODO: Pumpkins and jack o lanterns properly
			SetOptimAllTextures( BlockId.Pumpkin, Row6 + 11 );
			SetOptimAllTextures( BlockId.JackOLantern, Row6 + 11 );
			SetOptimAllTextures( BlockId.Netherrack, Row6 + 12 );
			SetOptimAllTextures( BlockId.SoulSand, Row6 + 13 );
			SetOptimAllTextures( BlockId.GlowstoneBlock, Row6 + 14 );
			// TODO: Pistons properly
			SetOptimAllTextures( BlockId.StickyPiston, Row6 + 15 );
			SetOptimAllTextures( BlockId.Piston, Row7 );
			SetOptimAllTextures( BlockId.RedstoneTorchOff, Row7 + 7 );
			// TODO: Eaten cake
			SetOptimTopTexture( BlockId.CakeBlock, Row7 + 13 );
			SetOptimSideTextures( BlockId.CakeBlock, Row7 + 14 );
			SetOptimBottomTexture( BlockId.CakeBlock, Row8 );
			// TODO: Rails
			SetOptimAllTextures( BlockId.Rails, Row8 + 1 );
			// TODO: Repeaters off
			SetOptimAllTextures( BlockId.RedstoneRepeaterOff, Row8 + 4 );
			// TODO: Beds
			SetOptimAllTextures( BlockId.Bed, Row8 + 7 );
			SetOptimAllTextures( BlockId.LapisLazuliBlock, Row8 + 9 );
			// TODO: Repeaters on
			SetOptimAllTextures( BlockId.RedstoneRepeaterOn, Row8 + 12 );
			SetOptimAllTextures( BlockId.LapisLazuliOre, Row9 + 1 );
			// TODO: Powered rail on
			SetOptimAllTextures( BlockId.PoweredRail, Row9 + 4 );
			// TODO: Redstone wire
			SetOptimAllTextures( BlockId.RedstoneWire, Row9 + 5 );
			SetOptimTopTexture( BlockId.Sandstone, Row9 + 7 );
			SetOptimSideTextures( BlockId.Sandstone, Row9 + 11 );
			// TODO: Detector rail
			SetOptimAllTextures( BlockId.DetectorRail, Row9 + 14 );
			// TODO: Water
			SetOptimAllTextures( BlockId.Water, Row9 + 15 );
			SetOptimAllTextures( BlockId.StillWater, Row9 + 15 );
			SetOptimBottomTexture( BlockId.Sandstone, Row10 );
			// TODO: Lava
			SetOptimAllTextures( BlockId.Lava, Row10 + 4 );
			SetOptimAllTextures( BlockId.StillLava, Row10 + 4 );
		}

		void SetOptimAllTextures( BlockId blockId, int textureId ) {
			int index = (byte)blockId * 6;
			for( int i = index; i < index + 6; i++ ) {
				optimTextures[i] = textureId;
			}
		}
		
		void SetOptimSideTextures( BlockId blockId, int textureId ) {
			int index = (byte)blockId * 6;
			for( int i = index; i < index + TileSide.Bottom; i++ ) {
				optimTextures[i] = textureId;
			}
		}
		
		void SetOptimTopTexture( BlockId blockId, int textureId ) {
			optimTextures[(byte)blockId * 6 + TileSide.Top] = textureId;
		}
		
		void SetOptimBottomTexture( BlockId blockId, int textureId ) {
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