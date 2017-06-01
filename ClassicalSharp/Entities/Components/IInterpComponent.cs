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
		internal Vector3 curPos;
		internal float curRotX, curRotZ, curHeadX, curHeadY;
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			Vector3 lastPos = curPos;
			float lastRotX = curRotX, lastRotZ = curRotZ;
			float lastHeadX = curHeadX, lastHeadY = curHeadY;
			
			if (update.IncludesPosition) {
				curPos = update.RelativePosition ? curPos + update.Pos : update.Pos;
			}
			curRotX =  Next(update.RotX,  curRotX);
			curRotZ =  Next(update.RotZ,  curRotZ);
			curHeadX = Next(update.HeadX, curHeadX);
			curHeadY = Next(update.RotY,  curHeadY);
			
			if (!interpolate) {
				stateCount = 0;
				next = prev = new State(curPos, curHeadX, curHeadY, curRotX, curRotZ);
				rotYStateCount = 0;
				nextRotY = prevRotY = curHeadY;
			} else {
				// Smoother interpolation by also adding midpoint.
				Vector3 midPos = Vector3.Lerp(lastPos, curPos, 0.5f);
				float midRotX =  Utils.LerpAngle(lastRotX,  curRotX,  0.5f);
				float midRotZ =  Utils.LerpAngle(lastRotZ,  curRotZ,  0.5f);				
				float midHeadX = Utils.LerpAngle(lastHeadX, curHeadX, 0.5f);
				float midHeadY = Utils.LerpAngle(lastHeadY, curHeadY, 0.5f);
				
				AddState(new State(midPos, midHeadX, midHeadY, midRotX, midRotZ));
				AddState(new State(curPos, curHeadX, curHeadY, curRotX, curRotZ));
				for (int i = 0; i < 3; i++)
					AddRotY(Utils.LerpAngle(lastHeadY, curHeadY, (i + 1) / 3f));
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
		
		static float Next(float next, float cur) {
			if (float.IsNaN(next)) return cur;
			return next;
		}
		
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
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			if (update.IncludesPosition) {
				next.Pos = update.RelativePosition ? next.Pos + update.Pos : update.Pos;
				float blockOffset = next.Pos.Y - Utils.Floor(next.Pos.Y);
				if (blockOffset < Entity.Adjustment)
					next.Pos.Y += Entity.Adjustment;
				
				if (!interpolate) {
					prev.Pos = entity.Position = next.Pos;
				}
			}
			
			next.RotX =  Next(update.RotX,  next.RotX,  ref prev.RotX,  interpolate);
			next.RotZ =  Next(update.RotZ,  next.RotZ,  ref prev.RotZ,  interpolate);
			next.HeadX = Next(update.HeadX, next.HeadX, ref prev.HeadX, interpolate);
			next.HeadY = Next(update.RotY,  next.HeadY, ref prev.HeadY, interpolate);
			
			if (!float.IsNaN(update.RotY)) {
				// Body Y rotation lags slightly behind
				if (!interpolate) {
					nextRotY = update.RotY; entity.RotY = update.RotY;
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
		
		static float Next(float next, float cur, ref float last, bool interpolate) {
			if (float.IsNaN(next)) return cur;
			
			if (!interpolate) last = next;
			return next;
		}
	}
}