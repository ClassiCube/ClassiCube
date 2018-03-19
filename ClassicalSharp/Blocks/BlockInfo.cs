// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Blocks;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

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
		public static int MaxDefined;
		
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
			MaxDefined = count - 1;
		}
		
		public static void Reset() {
			Init();
			RecalculateSpriteBB();
		}
		
		public static void Init() {
			for (int i = 0; i < DefinedCustomBlocks.Length; i++) {
				DefinedCustomBlocks[i] = 0;
			}
			for (int b = 0; b <= MaxDefined; b++) {
				ResetBlockProps((BlockID)b);
			}
			UpdateCulling();
		}

		public static void SetDefaultPerms() {
			for (int b = Block.Air; b <= MaxDefined; b++) {
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
			LightOffset[block] = CalcLightOffset(block);
			
			if (block >= Block.CpeCount) {
				#if USE16_BIT
				// give some random texture ids
				SetTex((block * 10 + (block % 7) + 20) % 80, Side.Top, block);
				SetTex((block * 8  + (block & 5) + 5 ) % 80, Side.Bottom, block);
				SetSide((block * 4 + (block / 4) + 4 ) % 80, block);
				#else
				SetTex(0, Side.Top, block);
				SetTex(0, Side.Bottom, block);
				SetSide(0, block);
				#endif
			} else {
				SetTex(topTex[block], Side.Top, block);
				SetTex(bottomTex[block], Side.Bottom, block);
				SetSide(sideTex[block], block);
			}
		}

		public static int FindID(string name) {
			for (int b = 0; b <= MaxDefined; b++) {
				if (Utils.CaselessEquals(Name[b], name)) return b;
			}
			return -1;
		}
		
		
		static StringBuffer buffer = new StringBuffer(64);
		static string DefaultName(BlockID block) {
			#if USE16_BIT
			if (block >= 256) return "ID " + block;
			#endif
			if (block >= Block.CpeCount) return "Invalid";
			
			// Find start and end of this particular block name
			int start = 0;
			for (int i = 0; i < block; i++)
				start = Block.RawNames.IndexOf(' ', start) + 1;
			int end = Block.RawNames.IndexOf(' ', start);
			if (end == -1) end = Block.RawNames.Length;
			
			buffer.Clear();
			SplitUppercase(buffer, start, end);
			return buffer.ToString();
		}
		
		static void SplitUppercase(StringBuffer buffer, int start, int end) {
			for (int i = start; i < end; i++) {
				char c = Block.RawNames[i];
				bool upper = Char.IsUpper(c) && i > start;
				bool nextLower = i < end - 1 && !Char.IsUpper(Block.RawNames[i + 1]);
				
				if (upper && nextLower) {
					buffer.Append(' ');
					buffer.Append(Char.ToLower(c));
				} else {
					buffer.Append(c);
				}
			}
		}
	}
}
