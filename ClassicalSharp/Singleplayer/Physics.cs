﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Map;
using ClassicalSharp.Events;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Singleplayer {
	
	public delegate void PhysicsAction(int index, BlockID block);

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
		
		public PhysicsAction[] OnActivate = new PhysicsAction[Block.Count];
		public PhysicsAction[] OnRandomTick = new PhysicsAction[Block.Count];
		public PhysicsAction[] OnPlace = new PhysicsAction[Block.Count];
		public PhysicsAction[] OnDelete = new PhysicsAction[Block.Count];
		
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
			if (!Enabled || game.World.blocks == null) return;
			
			if ((tickCount % 5) == 0) {
				liquid.TickWater();
				liquid.TickLava();
			}
			tickCount++;
			TickRandomBlocks();
		}
		
		void BlockChanged(object sender, BlockChangedEventArgs e) {
			if (!Enabled) return;
			Vector3I p = e.Coords;
			int index = (p.Y * length + p.Z) * width + p.X;
			BlockID block = e.Block;
			
			if (block == Block.Air && IsEdgeWater(p.X, p.Y, p.Z)) { 
				block = Block.StillWater; 
				game.UpdateBlock(p.X, p.Y, p.Z, Block.StillWater);
			}
			
			if (e.Block == 0) {
				PhysicsAction delete = OnDelete[e.OldBlock];
				if (delete != null) delete(index, e.OldBlock);
			} else {
				PhysicsAction place = OnPlace[block];
				if (place != null) place(index, block);
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
			BlockID block = map.blocks[index];
			PhysicsAction activate = OnActivate[block];
			if (activate != null) activate(index, block);
		}
		
		bool IsEdgeWater(int x, int y, int z) {
			WorldEnv env = map.Env;
			if (!(env.EdgeBlock == Block.Water || env.EdgeBlock == Block.StillWater))
				return false;
			
			return y >= env.SidesHeight && y < env.EdgeHeight 
				&& (x == 0 || z == 0 || x == (map.Width - 1) || z == (map.Length - 1));
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
				int index = 0;
				for (int t = 0; t <= 3; t++) {
					index = rnd.Next(lo, hi);
					BlockID block = map.blocks[index];
					PhysicsAction tick = OnRandomTick[block];
					if (tick != null) tick(index, block);
				}
			}
		}
	}
}
