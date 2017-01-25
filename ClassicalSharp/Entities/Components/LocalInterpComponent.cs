// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs interpolation of position and model head yaw over time. </summary>
	public sealed class LocalInterpComponent {
		
		Entity entity;
		public LocalInterpComponent(Game game, Entity entity) {
			this.entity = entity;
		}

		internal Vector3 lastPos, nextPos;
		internal float lastHeadY, lastHeadX, lastRotX, lastRotY, lastRotZ;
		internal float nextHeadY, nextHeadX, nextRotX, nextRotY, nextRotZ;
		int rotYStateCount;
		float[] rotYStates = new float[15];
		
		public void SetLocation(LocationUpdate update, bool interpolate) {
			if (update.IncludesPosition) {
				nextPos = update.RelativePosition ? nextPos + update.Pos : update.Pos;
				double blockOffset = nextPos.Y - Math.Floor(nextPos.Y);
				if (blockOffset < Entity.Adjustment)
					nextPos.Y += Entity.Adjustment;
				if (!interpolate) {
					lastPos = entity.Position = nextPos;
				}
			}
			
			nextRotX =  Next(update.RotX,  nextRotX,  ref lastRotX,  interpolate);
			nextRotZ =  Next(update.RotZ,  nextRotZ,  ref lastRotZ,  interpolate);
			nextHeadX = Next(update.HeadX, nextHeadX, ref lastHeadX, interpolate);
			
			if (float.IsNaN(update.RotY)) return;
			nextHeadY = update.RotY;
			
			if (!interpolate) {
				lastHeadY = update.RotY; entity.HeadY = update.RotY; entity.RotY = update.RotY;
				rotYStateCount = 0;
			} else {
				for (int i = 0; i < 3; i++)
					AddRotY(Utils.LerpAngle(lastHeadY, nextHeadY, (i + 1) / 3f));
			}
		}	
		
		public void AdvanceState() {
			lastPos = entity.Position = nextPos;
			lastHeadY = nextHeadY; lastHeadX = nextHeadX;
			lastRotX = nextRotX; lastRotY = nextRotY; lastRotZ = nextRotZ;
			
			if (rotYStateCount > 0) {
				nextRotY = rotYStates[0];
				RemoveOldest(rotYStates, ref rotYStateCount);
			}
		}
		
		public void LerpAngles(float t) {
			entity.HeadX = Utils.LerpAngle(lastHeadX, nextHeadX, t);
			entity.HeadY = Utils.LerpAngle(lastHeadY, nextHeadY, t);
			entity.RotX =  Utils.LerpAngle(lastRotX,  nextRotX, t);
			entity.RotY =  Utils.LerpAngle(lastRotY,  nextRotY, t);
			entity.RotZ =  Utils.LerpAngle(lastRotZ,  nextRotZ, t);
		}
		
		
		static float Next(float next, float cur, ref float last, bool interpolate) {
			if (float.IsNaN(next)) return cur;
			
			if (!interpolate) last = next;
			return next;
		}
		
		void AddRotY(float state) {
			if (rotYStateCount == rotYStates.Length)
				RemoveOldest(rotYStates, ref rotYStateCount);
			rotYStates[rotYStateCount++] = state;
		}
		
		void RemoveOldest<T>(T[] array, ref int count) {
			for (int i = 0; i < array.Length - 1; i++)
				array[i] = array[i + 1];
			count--;
		}
	}
}