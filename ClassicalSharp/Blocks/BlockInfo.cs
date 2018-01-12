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

	public enum SoundType : byte {
		None, Wood, Gravel, Grass, Stone,
		Metal, Glass, Cloth, Sand, Snow,
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
	}
	
	/// <summary> Stores various properties about the blocks. </summary>
	/// <remarks> e.g. blocks light, height, texture IDs, etc. </remarks>
	public static partial class BlockInfo {
		
		public static bool[] IsLiquid = new bool[Block.Count];
		public static bool[] BlocksLight = new bool[Block.Count];
		public static bool[] FullBright = new bool[Block.Count];
		public static string[] Name = new string[Block.Count];
		public static FastColour[] FogColour = new FastColour[Block.Count];
		public static float[] FogDensity = new float[Block.Count];
		public static byte[] Collide = new byte[Block.Count];
		public static byte[] ExtendedCollide = new byte[Block.Count];
		public static float[] SpeedMultiplier = new float[Block.Count];
		public static byte[] LightOffset = new byte[Block.Count];
		public static byte[] Draw = new byte[Block.Count];
		public static uint[] DefinedCustomBlocks = new uint[Block.Count >> 5];
		public static SoundType[] DigSounds = new SoundType[Block.Count];
		public static SoundType[] StepSounds = new SoundType[Block.Count];
		public static bool[] CanPlace = new bool[Block.Count];
		public static bool[] CanDelete = new bool[Block.Count];
		public static bool[] Tinted = new bool[Block.Count];
		public static byte[] SpriteOffset = new byte[Block.Count];
		
		/// <summary> Gets whether the given block has an opaque draw type and is also a full tile block. </summary>
		/// <remarks> Full tile block means Min of (0, 0, 0) and max of (1, 1, 1). </remarks>
		public static bool[] FullOpaque = new bool[Block.Count];
		
		public static void Reset(Game game) {
			Init();
			// TODO: Make this part of TerrainAtlas2D maybe?
			using (FastBitmap fastBmp = new FastBitmap(game.TerrainAtlas.AtlasBitmap, true, true))
				RecalculateSpriteBB(fastBmp);
		}
		
		public static void Init() {
			for (int i = 0; i < DefinedCustomBlocks.Length; i++)
				DefinedCustomBlocks[i] = 0;
			for (int block = 0; block < Block.Count; block++)
				ResetBlockProps((BlockID)block);
			UpdateCulling();
		}

		public static void SetDefaultPerms() {
			for (int block = Block.Stone; block <= Block.MaxDefinedBlock; block++) {
				CanPlace[block] = true;
				CanDelete[block] = true;
			}
			
			CanPlace[Block.Lava]       = false; CanDelete[Block.Lava]       = false;
			CanPlace[Block.Water]      = false; CanDelete[Block.Water]      = false;
			CanPlace[Block.StillLava]  = false; CanDelete[Block.StillLava]  = false;
			CanPlace[Block.StillWater] = false; CanDelete[Block.StillWater] = false;
			CanPlace[Block.Bedrock]    = false; CanDelete[Block.Bedrock]    = false;
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
			for (int i = 0; i < Block.Count; i++) {
				if (Utils.CaselessEquals(Name[i], name)) return i;
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
			int index = 0;
			for (int i = start; i < end; i++) {
				char c = Block.RawNames[i];
				bool upper = Char.IsUpper(c) && i > start;
				bool nextLower = i < end - 1 && !Char.IsUpper(Block.RawNames[i + 1]);
				
				if (upper && nextLower) {
					buffer.Append(ref index, ' ');
					buffer.Append(ref index, Char.ToLower(c));
				} else {
					buffer.Append(ref index, c);
				}
			}
		}
	}
}