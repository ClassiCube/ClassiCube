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
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		/// <summary> Gets whether the given block is a liquid. (water and lava) </summary>
		public bool IsLiquid(BlockID block) { return block >= Block.Water && block <= Block.StillLava; }
		
		/// <summary> Gets whether the given block stops sunlight. </summary>
		public bool[] BlocksLight = new bool[Block.Count];
		
		/// <summary> Gets whether the given block should draw all its faces in a full white colour. </summary>
		public bool[] FullBright = new bool[Block.Count];
		
		/// <summary> Gets the name of the given block, or 'Invalid' if the block is not defined. </summary>
		public string[] Name = new string[Block.Count];
		
		/// <summary> Gets the custom fog colour that should be used when the player is standing within this block.
		/// Note that this is only used for exponential fog mode. </summary>
		public FastColour[] FogColour = new FastColour[Block.Count];

		/// <summary> Gets the fog density for the given block. </summary>
		/// <remarks> A value of 0 means this block does not apply fog. </remarks>
		public float[] FogDensity = new float[Block.Count];
		
		public byte[] Collide = new byte[Block.Count];
		
		public byte[] ExtendedCollide = new byte[Block.Count];
		
		public float[] SpeedMultiplier = new float[Block.Count];
		
		public byte[] LightOffset = new byte[Block.Count];

		/// <summary> Gets the DrawType for the given block. </summary>
		public byte[] Draw = new byte[Block.Count];
		
		/// <summary> Gets whether the given block has an opaque draw type and is also a full tile block. </summary>
		/// <remarks> Full tile block means Min of (0, 0, 0) and max of (1, 1, 1). </remarks>
		public bool[] FullOpaque = new bool[Block.Count];
		
		public uint[] DefinedCustomBlocks = new uint[Block.Count >> 5];
		
		public SoundType[] DigSounds = new SoundType[Block.Count];
		
		public SoundType[] StepSounds = new SoundType[Block.Count];

		/// <summary> Gets whether the given block has a tinting colour applied to it when rendered. </summary>
		/// <remarks> The tinting colour used is the block's fog colour. </remarks>
		public bool[] Tinted = new bool[Block.Count];
		
		/// <summary> Recalculates the initial properties and culling states for all blocks. </summary>
		public void Reset(Game game) {
			Init();
			// TODO: Make this part of TerrainAtlas2D maybe?
			using (FastBitmap fastBmp = new FastBitmap(game.TerrainAtlas.AtlasBitmap, true, true))
				RecalculateSpriteBB(fastBmp);
		}
		
		/// <summary> Calculates the initial properties and culling states for all blocks. </summary>
		public void Init() {
			for (int i = 0; i < DefinedCustomBlocks.Length; i++)
				DefinedCustomBlocks[i] = 0;
			for (int block = 0; block < Block.Count; block++)
				ResetBlockProps((BlockID)block);
			UpdateCulling();
		}

		/// <summary> Initialises the default blocks the player is allowed to place and delete. </summary>
		public void SetDefaultPerms(InventoryPermissions place, InventoryPermissions delete) {
			for (int block = Block.Stone; block <= Block.MaxDefinedBlock; block++) {
				place[block] = true;
				delete[block] = true;
			}
			
			place[Block.Lava]       = false; delete[Block.Lava]       = false;
			place[Block.Water]      = false; delete[Block.Water]      = false;
			place[Block.StillLava]  = false; delete[Block.StillLava]  = false;
			place[Block.StillWater] = false; delete[Block.StillWater] = false;
			place[Block.Bedrock]    = false; delete[Block.Bedrock]    = false;
		}
		
		public void SetCollide(BlockID block, byte collide) {
			// necessary for cases where servers redefined core blocks before extended types were introduced
			collide = DefaultSet.MapOldCollide(block, collide);
			ExtendedCollide[block] = collide;
			
			// Reduce extended collision types to their simpler forms
			if (collide == CollideType.Ice) collide = CollideType.Solid;
			if (collide == CollideType.SlipperyIce) collide = CollideType.Solid;
			
			if (collide == CollideType.LiquidWater) collide = CollideType.Liquid;
			if (collide == CollideType.LiquidLava) collide = CollideType.Liquid;
			Collide[block] = collide;
		}
		
		public void SetBlockDraw(BlockID block, byte draw) {
			if (draw == DrawType.Opaque && Collide[block] != CollideType.Solid)
				draw = DrawType.Transparent;
			Draw[block] = draw;
			
			FullOpaque[block] = draw == DrawType.Opaque
				&& MinBB[block] == Vector3.Zero && MaxBB[block] == Vector3.One;
		}
		
		/// <summary> Resets the properties for the given block to their defaults. </summary>
		public void ResetBlockProps(BlockID block) {
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
		
		/// <summary> Finds the ID of the block whose name caselessly matches the input, -1 otherwise. </summary>
		public int FindID(string name) {
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
				start = Block.Names.IndexOf(' ', start) + 1;
			int end = Block.Names.IndexOf(' ', start);
			if (end == -1) end = Block.Names.Length;
			
			buffer.Clear();
			SplitUppercase(buffer, start, end);
			return buffer.ToString();
		}
		
		static void SplitUppercase(StringBuffer buffer, int start, int end) {
			int index = 0;
			for (int i = start; i < end; i++) {
				char c = Block.Names[i];
				bool upper = Char.IsUpper(c) && i > start;
				bool nextLower = i < end - 1 && !Char.IsUpper(Block.Names[i + 1]);
				
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