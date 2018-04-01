// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;
using ClassicalSharp.Events;
using BlockID = System.UInt16;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Singleplayer {
	
	public delegate void PhysicsAction(int index, BlockRaw block);

	public class PhysicsBase {
		Game game;
		World map;
		Random rnd = new Random();
		int width, length, height, oneY;

		FallingPhysics falling;
		TNTPhysics tnt;
		FoliagePhysics foliage;
		LiquidPhysics liquid;
		OtherPhysics other;
		
		bool enabled = true;
		public bool Enabled {
			get { return enabled; }
			set { enabled = value; liquid.Clear(); }
		}
		
		public PhysicsAction[] OnActivate = new PhysicsAction[Block.DefaultCount];
		public PhysicsAction[] OnRandomTick = new PhysicsAction[Block.DefaultCount];
		public PhysicsAction[] OnPlace = new PhysicsAction[Block.DefaultCount];
		public PhysicsAction[] OnDelete = new PhysicsAction[Block.DefaultCount];
		
		public PhysicsBase(Game game) {
			this.game = game;
			map = game.World;
			game.WorldEvents.OnNewMapLoaded += ResetMap;
			game.UserEvents.BlockChanged += BlockChanged;
			enabled = Options.GetBool(OptionsKey.BlockPhysics, true);
			
			falling = new FallingPhysics(game, this);
			tnt = new TNTPhysics(game, this);
			foliage = new FoliagePhysics(game, this);
			liquid = new LiquidPhysics(game, this);
			other = new OtherPhysics(game, this);
		}
		
		int tickCount = 0;
		public void Tick() {
			if (!Enabled || !game.World.HasBlocks) return;
			
			//if ((tickCount % 5) == 0) {
			liquid.TickLava();
			liquid.TickWater();
			//}
			tickCount++;
			TickRandomBlocks();
		}
		
		void BlockChanged(object sender, BlockChangedEventArgs e) {
			if (!Enabled) return;
			Vector3I p = e.Coords;
			int index = (p.Y * length + p.Z) * width + p.X;
			BlockRaw newB = (BlockRaw)e.Block, oldB = (BlockRaw)e.OldBlock;
			
			if (newB == Block.Air && IsEdgeWater(p.X, p.Y, p.Z)) { 
				newB = Block.StillWater; 
				game.UpdateBlock(p.X, p.Y, p.Z, Block.StillWater);
			}
			
			if (newB == Block.Air) {
				PhysicsAction delete = OnDelete[oldB];
				if (delete != null) delete(index, oldB);
			} else {
				PhysicsAction place = OnPlace[newB];
				if (place != null) place(index, newB);
			}
			ActivateNeighbours(p.X, p.Y, p.Z, index);
		}
		
		/// <summary> Activates the direct neighbouring blocks of the given coordinates. </summary>
		public void ActivateNeighbours(int x, int y, int z, int index) {
			if (x > 0) Activate(index - 1);
			if (x < map.MaxX) Activate(index + 1);
			if (z > 0) Activate(index - map.Width);
			if (z < map.MaxZ) Activate(index + map.Width);
			if (y > 0) Activate(index - oneY);
			if (y < map.MaxY) Activate(index + oneY);
		}
		
		/// <summary> Activates the block at the particular packed coordinates. </summary>
		public void Activate(int index) {
			BlockRaw block = map.blocks[index];
			PhysicsAction activate = OnActivate[block];
			if (activate != null) activate(index, block);
		}
		
		bool IsEdgeWater(int x, int y, int z) {
			WorldEnv env = map.Env;
			if (!(env.EdgeBlock == Block.Water || env.EdgeBlock == Block.StillWater))
				return false;
			
			return y >= env.SidesHeight && y < env.EdgeHeight 
				&& (x == 0 || z == 0 || x == map.MaxX || z == map.MaxZ);
		}
		
		void ResetMap(object sender, EventArgs e) {
			falling.ResetMap();
			liquid.ResetMap();
			width = map.Width;
			height = map.Height;
			length = map.Length;
			oneY = width * length;
		}
		
		public void Dispose() {
			game.WorldEvents.OnNewMapLoaded -= ResetMap;
			game.UserEvents.BlockChanged -= BlockChanged;
		}
		
		void TickRandomBlocks() {
			int xMax = width - 1, yMax = height - 1, zMax = length - 1;
			for (int y = 0; y < height; y += 16)
				for (int z = 0; z < length; z += 16)
					for (int x = 0; x < width; x += 16)
			{
				int lo = (y * length + z) * width + x;
				int hi = (Math.Min(yMax, y + 15) * length + Math.Min(zMax, z + 15))
					* width + Math.Min(xMax, x + 15);
				
				// Inlined 3 random ticks for this chunk
				int index = rnd.Next(lo, hi);
				BlockRaw block = map.blocks[index];
				PhysicsAction tick = OnRandomTick[block];
				if (tick != null) tick(index, block);
				
				index = rnd.Next(lo, hi);
				block = map.blocks[index];
				tick = OnRandomTick[block];
				if (tick != null) tick(index, block);
				
				index = rnd.Next(lo, hi);
				block = map.blocks[index];
				tick = OnRandomTick[block];
				if (tick != null) tick(index, block);
			}
		}
	}
}