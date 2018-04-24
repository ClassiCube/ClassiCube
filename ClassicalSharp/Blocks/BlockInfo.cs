// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Blocks;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp {

	public static class SoundType {
		public const byte None = 0;   public const byte Wood = 1;
		public const byte Gravel = 2; public const byte Grass = 3;
		public const byte Stone = 4;  public const byte Metal = 5;
		public const byte Glass = 6;  public const byte Cloth = 7;
		public const byte Sand = 8;   public const byte Snow = 9;
		
		public static string[] Names = new string[10] {
			"none", "wood", "gravel", "grass", "stone",
			"metal", "glass", "cloth", "sand", "snow",
		};
	}

	/// <summary> Describes how a block is rendered in the world. </summary>
	public static class DrawType {
		/// <summary> Completely covers blocks behind (e.g. dirt). </summary>
		public const byte Opaque = 0;
		/// <summary> Blocks behind show (e.g. glass). Pixels are either fully visible or invisible. </summary>
		public const byte Transparent = 1;
		/// <summary> Same as Transparent, but all neighbour faces show. (e.g. leaves) </summary>
		public const byte TransparentThick = 2;
		/// <summary> Blocks behind show (e.g. water). Pixels blend with other blocks behind. </summary>
		public const byte Translucent = 3;
		/// <summary> Does not show (e.g. air). Can still be collided with. </summary>
		public const byte Gas = 4;
		/// <summary> Block renders as an X sprite (e.g. sapling). Pixels are either fully visible or invisible. </summary>
		public const byte Sprite = 5;
	}
	
	/// <summary> Describes the interaction a block has with a player when they collide with it. </summary>
	public static class CollideType {
		/// <summary> No interaction when player collides. </summary>
		public const byte Gas = 0;
		/// <summary> 'swimming'/'bobbing' interaction when player collides. </summary>
		public const byte Liquid = 1;
		/// <summary> Block completely stops the player when they are moving. </summary>
		public const byte Solid = 2;
		/// <summary> Block is solid and partially slidable on. </summary>
		public const byte Ice = 3;
		/// <summary> Block is solid and fully slidable on. </summary>
		public const byte SlipperyIce = 4;
		/// <summary> Water style 'swimming'/'bobbing' interaction when player collides. </summary>
		public const byte LiquidWater = 5;
		/// <summary> Lava style 'swimming'/'bobbing' interaction when player collides. </summary>
		public const byte LiquidLava = 6;
		/// <summary> Rope/Ladder style climbing interaction when player collides. </summary>
		public const byte ClimbRope = 7;
	}
	
	/// <summary> Stores various properties about the blocks. </summary>
	/// <remarks> e.g. blocks light, height, texture IDs, etc. </remarks>
	public static partial class BlockInfo {
		
		public static bool[] IsLiquid, BlocksLight, FullBright;
		public static bool[] CanPlace, CanDelete, Tinted, FullOpaque;
		public static byte[] Collide, ExtendedCollide, textures, hidden;
		public static byte[] LightOffset, Draw, SpriteOffset, CanStretch;
		public static byte[] DigSounds, StepSounds;
		public static string[] Name;
		public static float[] FogDensity, SpeedMultiplier;
		public static FastColour[] FogColour;
		public static Vector3[] MinBB, MaxBB, RenderMinBB, RenderMaxBB;
		static uint[] DefinedCustomBlocks;
		public static int MaxUsed, Count, IDMask;
		
		public static void Allocate(int count) {
			IsLiquid = new bool[count];
			BlocksLight = new bool[count];
			FullBright = new bool[count];
			CanPlace = new bool[count];
			CanDelete = new bool[count];
			Tinted = new bool[count];
			FullOpaque = new bool[count];
			Collide = new byte[count];
			ExtendedCollide = new byte[count];
			textures = new byte[count * Side.Sides];
			hidden = new byte[count * count];
			LightOffset = new byte[count];
			Draw = new byte[count];
			SpriteOffset = new byte[count];
			CanStretch = new byte[count];
			DigSounds = new byte[count];
			StepSounds = new byte[count];
			Name = new string[count];
			FogDensity = new float[count];
			SpeedMultiplier = new float[count];
			FogColour = new FastColour[count];
			MinBB = new Vector3[count];
			MaxBB = new Vector3[count];
			RenderMinBB = new Vector3[count];
			RenderMaxBB = new Vector3[count];
			
			DefinedCustomBlocks = new uint[count >> 5];
			Count = count;
			SetMaxUsed(count - 1);
		}
		
		public static void SetMaxUsed(int max) {
			MaxUsed = max;
			IDMask  = Utils.NextPowerOf2(max + 1) - 1;
		}
		
		public static void Reset() {
			Init();
			RecalculateSpriteBB();
		}
		
		public static void Init() {
			for (int i = 0; i < DefinedCustomBlocks.Length; i++) {
				DefinedCustomBlocks[i] = 0;
			}
			for (int b = 0; b < Count; b++) {
				ResetBlockProps((BlockID)b);
			}
			UpdateCulling();
		}

		public static void SetDefaultPerms() {
			for (int b = Block.Air; b < Count; b++) {
				CanPlace[b] = true;
				CanDelete[b] = true;
			}
			
			CanPlace[Block.Air]        = false; CanDelete[Block.Air]        = false;
			CanPlace[Block.Lava]       = false; CanDelete[Block.Lava]       = false;
			CanPlace[Block.Water]      = false; CanDelete[Block.Water]      = false;
			CanPlace[Block.StillLava]  = false; CanDelete[Block.StillLava]  = false;
			CanPlace[Block.StillWater] = false; CanDelete[Block.StillWater] = false;
			CanPlace[Block.Bedrock]    = false; CanDelete[Block.Bedrock]    = false;
		}
		
		public static bool IsCustomDefined(BlockID block) {
			return (DefinedCustomBlocks[block >> 5] & (1u << (block & 0x1F))) != 0;
		}
		
		public static void SetCustomDefined(BlockID block, bool defined) {
			if (defined) {
				DefinedCustomBlocks[block >> 5] |= (1u << (block & 0x1F));
			} else {
				DefinedCustomBlocks[block >> 5] &= ~(1u << (block & 0x1F));
			}
		}
		
		public static void DefineCustom(Game game, BlockID block) {
			SetBlockDraw(block, Draw[block]);
			CalcRenderBounds(block);
			UpdateCulling(block);
			
			Tinted[block] = FogColour[block] != FastColour.Black && Name[block].IndexOf('#') >= 0;
			CalcLightOffset(block);
			
			game.Inventory.AddDefault(block);
			SetCustomDefined(block, true);
			game.Events.RaiseBlockDefinitionChanged();
		}
		
		static void RecalcIsLiquid(BlockID block) {
			byte collide = ExtendedCollide[block];
			IsLiquid[block] =
				(collide == CollideType.LiquidWater && Draw[block] == DrawType.Translucent) ||
				(collide == CollideType.LiquidLava  && Draw[block] == DrawType.Transparent);
		}
		
		public static void SetCollide(BlockID block, byte collide) {
			// necessary for cases where servers redefined core blocks before extended types were introduced
			collide = DefaultSet.MapOldCollide(block, collide);
			ExtendedCollide[block] = collide;
			RecalcIsLiquid(block);
			
			// Reduce extended collision types to their simpler forms
			if (collide == CollideType.Ice) collide = CollideType.Solid;
			if (collide == CollideType.SlipperyIce) collide = CollideType.Solid;
			
			if (collide == CollideType.LiquidWater) collide = CollideType.Liquid;
			if (collide == CollideType.LiquidLava) collide = CollideType.Liquid;
			Collide[block] = collide;
		}
		
		public static void SetBlockDraw(BlockID block, byte draw) {
			if (draw == DrawType.Opaque && Collide[block] != CollideType.Solid)
				draw = DrawType.Transparent;
			Draw[block] = draw;
			RecalcIsLiquid(block);
			
			FullOpaque[block] = draw == DrawType.Opaque
				&& MinBB[block] == Vector3.Zero && MaxBB[block] == Vector3.One;
		}
		
		public static void ResetBlockProps(BlockID block) {
			BlocksLight[block] = DefaultSet.BlocksLight(block);
			FullBright[block] = DefaultSet.FullBright(block);
			FogColour[block] = DefaultSet.FogColour(block);
			FogDensity[block] = DefaultSet.FogDensity(block);
			SetCollide(block, DefaultSet.Collide(block));
			DigSounds[block] = DefaultSet.DigSound(block);
			StepSounds[block] = DefaultSet.StepSound(block);
			SpeedMultiplier[block] = 1;
			Name[block] = DefaultName(block);
			Tinted[block] = false;
			SpriteOffset[block] = 0;
			
			Draw[block] = DefaultSet.Draw(block);
			if (Draw[block] == DrawType.Sprite) {
				MinBB[block] = new Vector3(2.50f/16f, 0, 2.50f/16f);
				MaxBB[block] = new Vector3(13.5f/16f, 1, 13.5f/16f);
			} else {
				MinBB[block] = Vector3.Zero;
				MaxBB[block] = Vector3.One;
				MaxBB[block].Y = DefaultSet.Height(block);
			}
			
			SetBlockDraw(block, Draw[block]);
			CalcRenderBounds(block);
			CalcLightOffset(block);
			
			if (block >= Block.CpeCount) {
				SetTex(0, Side.Top, block);
				SetTex(0, Side.Bottom, block);
				SetSide(0, block);
			} else {
				SetTex(topTex[block], Side.Top, block);
				SetTex(bottomTex[block], Side.Bottom, block);
				SetSide(sideTex[block], block);
			}
		}

		public static int FindID(string name) {
			for (int b = 0; b < Count; b++) {
				if (Utils.CaselessEquals(Name[b], name)) return b;
			}
			return -1;
		}
		
		
		const string RawNames = "Air_Stone_Grass_Dirt_Cobblestone_Wood_Sapling_Bedrock_Water_Still water_Lava" +
			"_Still lava_Sand_Gravel_Gold ore_Iron ore_Coal ore_Log_Leaves_Sponge_Glass_Red_Orange_Yellow_Lime_Green" +
			"_Teal_Aqua_Cyan_Blue_Indigo_Violet_Magenta_Pink_Black_Gray_White_Dandelion_Rose_Brown mushroom_Red mushroom" +
			"_Gold_Iron_Double slab_Slab_Brick_TNT_Bookshelf_Mossy rocks_Obsidian_Cobblestone slab_Rope_Sandstone" +
			"_Snow_Fire_Light pink_Forest green_Brown_Deep blue_Turquoise_Ice_Ceramic tile_Magma_Pillar_Crate_Stone brick";	
		
		static StringBuffer buffer = new StringBuffer(64);
		static string DefaultName(BlockID block) {
			if (block >= Block.CpeCount) return "Invalid";
			
			// Find start and end of this particular block name
			int start = 0;
			for (int i = 0; i < block; i++)
				start = RawNames.IndexOf('_', start) + 1;
			int end = RawNames.IndexOf('_', start);
			if (end == -1) end = RawNames.Length;
			
			buffer.Clear();
			for (int i = start; i < end; i++) {
				buffer.Append(RawNames[i]);
			}
			return buffer.ToString();
		}
		
		
		internal static void SetSide(byte textureId, BlockID blockId) {
			textures[blockId * Side.Sides + Side.Left]  = textureId;
			textures[blockId * Side.Sides + Side.Right] = textureId;
			textures[blockId * Side.Sides + Side.Front] = textureId;
			textures[blockId * Side.Sides + Side.Back]  = textureId;
		}
		
		internal static void SetTex(byte textureId, int face, BlockID blockId) {
			textures[blockId * Side.Sides + face] = textureId;
		}

		public static int GetTextureLoc(BlockID block, int face) {
			return textures[block * Side.Sides + face];
		}
		
		static byte[] topTex = new byte[] { 0,  1,  0,  2, 16,  4, 15, 17, 14, 14,
			30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
			72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 24, 23,  6,  6,  7,  9,  4,
			36, 37, 16, 11, 25, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 26, 53, 52, };
		static byte[] sideTex = new byte[] { 0,  1,  3,  2, 16,  4, 15, 17, 14, 14,
			30, 30, 18, 19, 32, 33, 34, 20, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
			72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 40, 39,  5,  5,  7,  8, 35,
			36, 37, 16, 11, 41, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 42, 53, 52, };
		static byte[] bottomTex = new byte[] { 0,  1,  2,  2, 16,  4, 15, 17, 14, 14,
			30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
			72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 56, 55,  6,  6,  7, 10,  4,
			36, 37, 16, 11, 57, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 58, 53, 52 };
		

		internal static void UpdateCulling() {
			for (int block = 0; block < Count; block++) {
				CalcStretch((BlockID)block);
				for (int neighbour = 0; neighbour < Count; neighbour++) {
					CalcCulling((BlockID)block, (BlockID)neighbour);
				}
			}
		}
		
		internal static void UpdateCulling(BlockID block) {
			CalcStretch(block);
			for (int other = 0; other < Count; other++) {
				CalcCulling(block, (BlockID)other);
				CalcCulling((BlockID)other, block);
			}
		}
		
		static void CalcStretch(BlockID block) {
			// faces which can be stretched on X axis
			if (MinBB[block].X == 0 && MaxBB[block].X == 1) {
				CanStretch[block] |= 0x3C;
			} else {
				CanStretch[block] &= 0xC3; // ~0x3C
			}
			
			// faces which can be stretched on Z axis
			if (MinBB[block].Z == 0 && MaxBB[block].Z == 1) {
				CanStretch[block] |= 0x03;
			} else {
				CanStretch[block] &= 0xFC; // ~0x03
			}
		}
		
		static void CalcCulling(BlockID block, BlockID other) {
			if (!IsHidden(block, other)) {
				// Block is not hidden at all, so we can just entirely skip per-face check
				BlockInfo.hidden[(block * Count) + other] = 0;
			} else {
				Vector3 bMin = MinBB[block], bMax = MaxBB[block];
				Vector3 oMin = MinBB[other], oMax = MaxBB[other];
				if (IsLiquid[block]) bMax.Y -= 1.5f/16;
				if (IsLiquid[other]) oMax.Y -= 1.5f/16;
				
				// Don't need to care about sprites here since they never cull faces
				bool bothLiquid = IsLiquid[block] && IsLiquid[other];
				int f = 0; // mark all faces initially 'not hidden'
				
				// Whether the 'texture region' of a face on block fits inside corresponding region on other block
				bool occludedX = (bMin.Z >= oMin.Z && bMax.Z <= oMax.Z) && (bMin.Y >= oMin.Y && bMax.Y <= oMax.Y);
				bool occludedY = (bMin.X >= oMin.X && bMax.X <= oMax.X) && (bMin.Z >= oMin.Z && bMax.Z <= oMax.Z);
				bool occludedZ = (bMin.X >= oMin.X && bMax.X <= oMax.X) && (bMin.Y >= oMin.Y && bMax.Y <= oMax.Y);
				
				f |= occludedX && oMax.X == 1 && bMin.X == 0 ? (1 << Side.Left)   : 0;
				f |= occludedX && oMin.X == 0 && bMax.X == 1 ? (1 << Side.Right)  : 0;
				f |= occludedZ && oMax.Z == 1 && bMin.Z == 0 ? (1 << Side.Front)  : 0;
				f |= occludedZ && oMin.Z == 0 && bMax.Z == 1 ? (1 << Side.Back)   : 0;
				f |= occludedY && (bothLiquid || (oMax.Y == 1 && bMin.Y == 0)) ? (1 << Side.Bottom) : 0;
				f |= occludedY && (bothLiquid || (oMin.Y == 0 && bMax.Y == 1)) ? (1 << Side.Top)    : 0;
				BlockInfo.hidden[(block * Count) + other] = (byte)f;
			}
		}
		
		static bool IsHidden(BlockID block, BlockID other) {
			// Sprite blocks can never hide faces.
			if (Draw[block] == DrawType.Sprite) return false;
			
			// NOTE: Water is always culled by lava
			if ((block == Block.Water || block == Block.StillWater) && (other == Block.Lava || other == Block.StillLava))
				return true;
			
			// All blocks (except for say leaves) cull with themselves.
			if (block == other) return Draw[block] != DrawType.TransparentThick;
			
			// An opaque neighbour (asides from lava) culls the face.
			if (Draw[other] == DrawType.Opaque && !IsLiquid[other]) return true;
			if (Draw[block] != DrawType.Translucent || Draw[other] != DrawType.Translucent) return false;
			
			// e.g. for water / ice, don't need to draw water.
			byte bType = Collide[block], oType = Collide[other];
			bool canSkip = (bType == CollideType.Solid && oType == CollideType.Solid) || bType != CollideType.Solid;
			return canSkip;
		}
		
		/// <summary> Returns whether the face at the given face of the block
		/// should be drawn with the neighbour 'other' present on the other side of the face. </summary>
		public static bool IsFaceHidden(BlockID block, BlockID other, int tileSide) {
			return (hidden[(block * Count) + other] & (1 << tileSide)) != 0;
		}
	}
}
