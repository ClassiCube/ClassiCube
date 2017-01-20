// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs interpolation of position and model head yaw over time. </summary>
	public sealed class NetInterpComponent {
		
		Entity entity;
		public NetInterpComponent(Game game, Entity entity) {
			this.entity = entity;
		}
		
		// Last known position and orientation sent by the server.
		internal Vector3 serverPos;
		internal float serverYaw, serverPitch;
		
		public void SetLocation(LocationUpdate update, bool interpolate) {
			Vector3 lastPos = serverPos;
			float lastYaw = serverYaw, lastPitch = serverPitch;
			if (update.IncludesPosition) {
				serverPos = update.RelativePosition ? serverPos + update.Pos : update.Pos;
			}
			if (update.IncludesOrientation) {
				serverYaw = update.Yaw; serverPitch = update.Pitch;
			}
			
			if (!interpolate) {
				stateCount = 0;
				newState = oldState = new State(entity.tickCount, serverPos, serverYaw, serverPitch);
				yawStateCount = 0;
				newYaw = oldYaw = serverYaw;
			} else {
				// Smoother interpolation by also adding midpoint.
				Vector3 midPos = Vector3.Lerp(lastPos, serverPos, 0.5f);
				float midYaw = Utils.LerpAngle(lastYaw, serverYaw, 0.5f);
				float midPitch = Utils.LerpAngle(lastPitch, serverPitch, 0.5f);
				AddState(new State(entity.tickCount, midPos, midYaw, midPitch));
				AddState(new State(entity.tickCount, serverPos, serverYaw, serverPitch));
				for (int i = 0; i < 3; i++)
					AddYaw(Utils.LerpAngle(lastYaw, serverYaw, (i + 1) / 3f));
			}
		}
		
		public struct State {
			public int tick;
			public Vector3 pos;
			public float headYaw, pitch;
			
			public State(int tick, Vector3 pos, float headYaw, float pitch) {
				this.tick = tick;
				this.pos = pos;
				this.headYaw = headYaw;
				this.pitch = pitch;
			}
		}
		
		State[] states = new State[10];
		float[] yawStates = new float[15];
		public State newState, oldState;
		public float newYaw, oldYaw;
		int stateCount, yawStateCount;
		
		void AddState(State state) {
			if (stateCount == states.Length)
				RemoveOldest(states, ref stateCount);
			states[stateCount++] = state;
		}
		
		void AddYaw(float state) {
			if (yawStateCount == yawStates.Length)
				RemoveOldest(yawStates, ref yawStateCount);
			yawStates[yawStateCount++] = state;
		}
		
		public void UpdateCurrentState() {
			oldState = newState;
			oldYaw = newYaw;
			if (stateCount > 0) {
				//if (states[0].tick > tickCount - 2) return; // 100 ms delay
				newState = states[0];
				RemoveOldest(states, ref stateCount);
			}
			if (yawStateCount > 0) {
				newYaw = yawStates[0];
				RemoveOldest(yawStates, ref yawStateCount);
			}
		}
		
		void RemoveOldest<T>(T[] array, ref int count) {
			for (int i = 0; i < array.Length - 1; i++)
				array[i] = array[i + 1];
			count--;
		}
	}
}