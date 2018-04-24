// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs interpolation of position and orientation over time. </summary>
	public abstract class IInterpComponent {
		
		public abstract void SetLocation(LocationUpdate update, bool interpolate);

		public virtual void AdvanceState() {
			prevRotY = nextRotY;
			if (rotYStateCount == 0) return;
			
			nextRotY = rotYStates[0];
			RemoveOldest(rotYStates, ref rotYStateCount);
		}
		

		public State prev, next;
		public float prevRotY, nextRotY;
		
		public struct State {
			public Vector3 Pos;
			public float HeadX, HeadY, RotX, RotZ;
			
			public State(Vector3 pos, float headX, float headY, float rotX, float rotZ) {
				this.Pos = pos;
				this.HeadX = headX; this.HeadY = headY;
				this.RotX = rotX; this.RotZ = rotZ;
			}
		}
		
		public void LerpAngles(float t) {
			entity.HeadX = Utils.LerpAngle(prev.HeadX, next.HeadX, t);
			entity.HeadY = Utils.LerpAngle(prev.HeadY, next.HeadY, t);
			entity.RotX =  Utils.LerpAngle(prev.RotX,  next.RotX,  t);
			entity.RotY =  Utils.LerpAngle(prevRotY,   nextRotY,   t);
			entity.RotZ =  Utils.LerpAngle(prev.RotZ,  next.RotZ,  t);
		}
		
		
		protected Entity entity;
		protected int rotYStateCount;
		protected float[] rotYStates = new float[15];
		
		protected void AddRotY(float state) {
			if (rotYStateCount == rotYStates.Length)
				RemoveOldest(rotYStates, ref rotYStateCount);
			rotYStates[rotYStateCount++] = state;
		}
		
		protected void RemoveOldest<T>(T[] array, ref int count) {
			for (int i = 0; i < array.Length - 1; i++)
				array[i] = array[i + 1];
			count--;
		}
	}
	
	
	public sealed class NetInterpComponent : IInterpComponent {

		public NetInterpComponent(Game game, Entity entity) {
			this.entity = entity;
		}
		
		// Last known position and orientation sent by the server.
		internal State cur;

		public override void SetLocation(LocationUpdate update, bool interpolate) {
			State last = cur;
			byte flags = update.Flags;
			if ((flags & LocationUpdateFlag.Pos) != 0) {
				cur.Pos = update.RelativePos ? cur.Pos + update.Pos : update.Pos;
			}

			if ((flags & LocationUpdateFlag.HeadX) != 0) cur.HeadX = update.HeadX;
			if ((flags & LocationUpdateFlag.HeadY) != 0) cur.HeadY = update.HeadY;
			if ((flags & LocationUpdateFlag.RotX) != 0)  cur.RotX  = update.RotX;
			if ((flags & LocationUpdateFlag.RotZ) != 0)  cur.RotZ  = update.RotZ;
			
			if (!interpolate) {
				stateCount = 0;
				next = cur; prev = next;
				rotYStateCount = 0;
				nextRotY = prevRotY = cur.HeadY;
			} else {
				// Smoother interpolation by also adding midpoint.
				State mid;
				mid.Pos   = Vector3.Lerp(last.Pos,      cur.Pos,   0.5f);
				mid.RotX  = Utils.LerpAngle(last.RotX,  cur.RotX,  0.5f);
				mid.RotZ  = Utils.LerpAngle(last.RotZ,  cur.RotZ,  0.5f);
				mid.HeadX = Utils.LerpAngle(last.HeadX, cur.HeadX, 0.5f);
				mid.HeadY = Utils.LerpAngle(last.HeadY, cur.HeadY, 0.5f);
				AddState(mid); AddState(cur);
				
				for (int i = 0; i < 3; i++) {
					AddRotY(Utils.LerpAngle(last.HeadY, cur.HeadY, (i + 1) / 3f));
				}
			}
		}

		public override void AdvanceState() {
			prev = next;
			if (stateCount > 0) {
				next = states[0];
				RemoveOldest(states, ref stateCount);
			}
			base.AdvanceState();
		}
		
		State[] states = new State[10];
		int stateCount;
		
		void AddState(State state) {
			if (stateCount == states.Length)
				RemoveOldest(states, ref stateCount);
			states[stateCount++] = state;
		}
	}
	
	
	/// <summary> Entity component that performs interpolation of position and orientation over time. </summary>
	public sealed class LocalInterpComponent : IInterpComponent {
		
		public LocalInterpComponent(Game game, Entity entity) {
			this.entity = entity;
		}
		
		void Angle(ref float prev, ref float next, float value, bool interpolate) {
			next = value;
			if (!interpolate) prev = value;
		}
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			byte flags = update.Flags;
			if ((flags & LocationUpdateFlag.Pos) != 0) {
				next.Pos = update.RelativePos ? next.Pos + update.Pos : update.Pos;
				// If server sets Y position exactly on ground, push up a tiny bit
				float yOffset = next.Pos.Y - Utils.Floor(next.Pos.Y);
				if (yOffset < Entity.Adjustment) { next.Pos.Y += Entity.Adjustment; }
				if (!interpolate) { prev.Pos = next.Pos; entity.Position = next.Pos; }
			}

			if ((flags & LocationUpdateFlag.HeadX) != 0) {
				Angle(ref prev.HeadX, ref next.HeadX, update.HeadX, interpolate);
			}
			if ((flags & LocationUpdateFlag.HeadY) != 0) {
				Angle(ref prev.HeadY, ref next.HeadY, update.HeadY, interpolate);
			}
			if ((flags & LocationUpdateFlag.RotX) != 0) {
				Angle(ref prev.RotX, ref next.RotX, update.RotX, interpolate);
			}
			if ((flags & LocationUpdateFlag.RotZ) != 0) {
				Angle(ref prev.RotZ, ref next.RotZ, update.RotZ, interpolate);
			}
			
			if ((flags & LocationUpdateFlag.HeadY) != 0) {
				// Body Y rotation lags slightly behind
				if (!interpolate) {
					nextRotY = update.HeadY; entity.RotY = update.HeadY;
					rotYStateCount = 0;
				} else {
					for (int i = 0; i < 3; i++)
						AddRotY(Utils.LerpAngle(prev.HeadY, next.HeadY, (i + 1) / 3f));
					nextRotY = rotYStates[0];
				}
			}
			
			LerpAngles(0);
		}
		
		public override void AdvanceState() {
			prev = next; entity.Position = next.Pos;
			base.AdvanceState();
		}
	}
}