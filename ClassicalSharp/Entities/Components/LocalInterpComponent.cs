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
		internal float lastHeadYaw, nextHeadYaw, lastPitch, nextPitch, lastYaw, nextYaw;
		int yawStateCount;
		float[] yawStates = new float[15];
		
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
			
			if (update.IncludesOrientation) {
				nextHeadYaw = update.Yaw;
				nextPitch = update.Pitch;
				if (!interpolate) {
					lastHeadYaw = entity.YawDegrees = nextHeadYaw;
					lastPitch = entity.PitchDegrees = nextPitch;
					entity.HeadYawDegrees = entity.YawDegrees;
					yawStateCount = 0;
				} else {
					for (int i = 0; i < 3; i++)
						AddYaw(Utils.LerpAngle(lastHeadYaw, nextHeadYaw, (i + 1) / 3f));
				}
			}
		}
		
		public void AdvanceState() {
			lastPos = entity.Position = nextPos;
			lastHeadYaw = nextHeadYaw;
			lastYaw = nextYaw;
			lastPitch = nextPitch;
			
			if (yawStateCount > 0) {
				nextYaw = yawStates[0];
				RemoveOldest(yawStates, ref yawStateCount);
			}
		}
		
		void AddYaw(float state) {
			if (yawStateCount == yawStates.Length)
				RemoveOldest(yawStates, ref yawStateCount);
			yawStates[yawStateCount++] = state;
		}
		
		void RemoveOldest<T>(T[] array, ref int count) {
			for (int i = 0; i < array.Length - 1; i++)
				array[i] = array[i + 1];
			count--;
		}
	}
}