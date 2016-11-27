// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Blocks;
using OpenTK;

namespace ClassicalSharp {

	public enum SoundType : byte {
		None, Wood, Gravel, Grass, Stone,
		Metal, Glass, Cloth, Sand, Snow,
	}
	
	public static class DrawType {
		public const byte Opaque = 0;
		public const byte Transparent = 1;
		public const byte TransparentThick = 2; // e.g. leaves render all neighbours
		public const byte Translucent = 3;
		public const byte Gas = 4;
		public const byte Sprite = 5;
	}
	
	public enum CollideType : byte {
		WalkThrough, // i.e. gas or sprite
		SwimThrough, // i.e. liquid
		Solid,       // i.e. solid
	}
	
	/// <summary> Stores various properties about the blocks in Minecraft Classic. </summary>
	public partial class BlockInfo {
		
		/// <summary> Gets whether the given block id is a liquid. (water and lava) </summary>
		public bool IsLiquid(byte block) { return block >= Block.Water && block <= Block.StillLava; }
		
		/// <summary> Gets whether the given block blocks sunlight. </summary>
		public bool[] BlocksLight = new bool[Block.Count];
		
		/// <summary> Gets whether the given block should draw all it faces with a full white colour component. </summary>
		public bool[] FullBright = new bool[Block.Count];
		
		public string[] Name = new string[Block.Count];
		
		/// <summary> Gets the custom fog colour that should be used when the player is standing within this block.
		/// Note that this is only used for exponential fog mode. </summary>
		public FastColour[] FogColour = new FastColour[Block.Count];
		
		public float[] FogDensity = new float[Block.Count];
		
		public CollideType[] Collide = new CollideType[Block.Count];
		
		public float[] SpeedMultiplier = new float[Block.Count];
		
		public byte[] LightOffset = new byte[Block.Count];

		public byte[] Draw = new byte[Block.Count];
		
		public uint[] DefinedCustomBlocks = new uint[Block.Count >> 5];
		
		public SoundType[] DigSounds = new SoundType[Block.Count];
		
		public SoundType[] StepSounds = new SoundType[Block.Count];
		
		public void Reset(Game game) {
			Init();
			// TODO: Make this part of TerrainAtlas2D maybe?
			using (FastBitmap fastBmp = new FastBitmap(game.TerrainAtlas.AtlasBitmap, true, true))
				RecalculateSpriteBB(fastBmp);
		}
		
		public void Init() {
			for (int i = 0; i < DefinedCustomBlocks.Length; i++)
				DefinedCustomBlocks[i] = 0;
			for (int block = 0; block < Block.Count; block++)
				ResetBlockProps((byte)block);
			UpdateCulling();
		}

		public void SetDefaultBlockPerms(InventoryPermissions place,
		                                 InventoryPermissions delete) {
			for (int block = Block.Stone; block <= Block.MaxDefinedBlock; block++) {
				place[block] = true;
				delete[block] = true;
			}
			place[Block.Lava] = false;
			place[Block.Water] = false;
			place[Block.StillLava] = false;
			place[Block.StillWater] = false;
			place[Block.Bedrock] = false;
			
			delete[Block.Bedrock] = false;
			delete[Block.Lava] = false;
			delete[Block.Water] = false;
			delete[Block.StillWater] = false;
			delete[Block.StillLava] = false;
		}
		
		public void SetBlockDraw(byte id, byte draw) {
			if (draw == DrawType.Opaque && Collide[id] != CollideType.Solid)
				draw = DrawType.Transparent;
			Draw[id] = draw;
		}
		
		public void ResetBlockProps(byte id) {
			BlocksLight[id] = DefaultSet.BlocksLight(id);
			FullBright[id] = DefaultSet.FullBright(id);
			FogColour[id] = DefaultSet.FogColour(id);
			FogDensity[id] = DefaultSet.FogDensity(id);
			Collide[id] = DefaultSet.Collide(id);
			DigSounds[id] = DefaultSet.DigSound(id);
			StepSounds[id] = DefaultSet.StepSound(id);
			SetBlockDraw(id, DefaultSet.Draw(id));
			SpeedMultiplier[id] = 1;
			Name[id] = DefaultName(id);
			
			if (Draw[id] == DrawType.Sprite) {
				MinBB[id] = new Vector3(2.50f/16f, 0, 2.50f/16f);
				MaxBB[id] = new Vector3(13.5f/16f, 1, 13.5f/16f);
			} else {
				MinBB[id] = Vector3.Zero;
				MaxBB[id] = Vector3.One;
				MaxBB[id].Y = DefaultSet.Height(id);
			}
			LightOffset[id] = CalcLightOffset(id);
			
			if (id >= Block.CpeCount) {
				SetTex(0, Side.Top, id);
				SetTex(0, Side.Bottom, id);
				SetSide(0, id);
			} else {
				SetTex(topTex[id], Side.Top, id);
				SetTex(bottomTex[id], Side.Bottom, id);
				SetSide(sideTex[id], id);
			}
		}
		
		static StringBuffer buffer = new StringBuffer(64);
		static string DefaultName(byte block) {
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