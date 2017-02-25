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
	public enum CollideType : byte {
		/// <summary> No interaction when player collides. (typically gas or sprite blocks) </summary>
		WalkThrough,
		
		/// <summary> 'swimming'/'bobbing' interaction when player collides. (typically liquid blocks) </summary>
		SwimThrough,
		
		/// <summary> Block completely stops the player when they are moving. (typically most blocks) </summary>
		Solid,
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
		
		public CollideType[] Collide = new CollideType[Block.Count];
		
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
		
		public void SetBlockDraw(BlockID id, byte draw) {
			if (draw == DrawType.Opaque && Collide[id] != CollideType.Solid)
				draw = DrawType.Transparent;
			Draw[id] = draw;
			
			FullOpaque[id] = draw == DrawType.Opaque
				&& MinBB[id] == Vector3.Zero && MaxBB[id] == Vector3.One;
		}
		
		/// <summary> Resets the properties for the given block to their defaults. </summary>
		public void ResetBlockProps(BlockID id) {
			BlocksLight[id] = DefaultSet.BlocksLight(id);
			FullBright[id] = DefaultSet.FullBright(id);
			FogColour[id] = DefaultSet.FogColour(id);
			FogDensity[id] = DefaultSet.FogDensity(id);
			Collide[id] = DefaultSet.Collide(id);
			DigSounds[id] = DefaultSet.DigSound(id);
			StepSounds[id] = DefaultSet.StepSound(id);
			SpeedMultiplier[id] = 1;
			Name[id] = DefaultName(id);
			Tinted[id] = false;
			
			Draw[id] = DefaultSet.Draw(id);
			if (Draw[id] == DrawType.Sprite) {
				MinBB[id] = new Vector3(2.50f/16f, 0, 2.50f/16f);
				MaxBB[id] = new Vector3(13.5f/16f, 1, 13.5f/16f);
			} else {
				MinBB[id] = Vector3.Zero;
				MaxBB[id] = Vector3.One;
				MaxBB[id].Y = DefaultSet.Height(id);
			}
			
			SetBlockDraw(id, Draw[id]);
			CalcRenderBounds(id);			
			LightOffset[id] = CalcLightOffset(id);
			
			if (id >= Block.CpeCount) {
				#if USE16_BIT
				// give some random texture ids
				SetTex((id * 10 + (id % 7) + 20) % 80, Side.Top, id);
				SetTex((id * 8  + (id & 5) + 5 ) % 80, Side.Bottom, id);
				SetSide((id * 4 + (id / 4) + 4 ) % 80, id);
				#else
				SetTex(0, Side.Top, id);
				SetTex(0, Side.Bottom, id);
				SetSide(0, id);
				#endif
			} else {
				SetTex(topTex[id], Side.Top, id);
				SetTex(bottomTex[id], Side.Bottom, id);
				SetSide(sideTex[id], id);
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