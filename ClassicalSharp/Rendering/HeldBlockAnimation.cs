// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.Model;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Renderers {
	
	internal class HeldBlockAnimation {
		
		internal bool doAnim, digAnim, swingAnim;
		internal float angleY = 0;
		internal Vector3 pos;
		
		double time, period = 0.25, speed = Math.PI / 0.25;
		BlockID lastType;
		Game game;
		HeldBlockRenderer held;
		
		internal void Init(Game game, HeldBlockRenderer held) {
			this.game = game;
			this.held = held;
			lastType = game.Inventory.Selected;
			
			game.Events.HeldBlockChanged += DoSwitchBlockAnim;
			game.UserEvents.BlockChanged += BlockChanged;
		}
		
		internal void Dispose() {
			game.Events.HeldBlockChanged -= DoSwitchBlockAnim;
			game.UserEvents.BlockChanged -= BlockChanged;
		}
		
		/// <summary> Sets the current animation state of the held block.<br/>
		/// true = left mouse pressed, false = right mouse pressed. </summary>
		internal void SetClickAnim(bool dig) {
			// TODO: timing still not quite right, rotate2 still not quite right
			ResetAnimationState(true, dig ? 0.35 : 0.25);
			swingAnim = false;
			digAnim = dig;
			doAnim = true;
			// Start place animation at bottom of cycle
			if (!dig) time = period / 2;
		}
		
		void DoSwitchBlockAnim(object sender, EventArgs e) {
			if (swingAnim) {
				// Like graph -sin(x) : x=0.5 and x=2.5 have same y values
				// but increasing x causes y to change in opposite directions
				if (time > period * 0.5)
					time = period - time;
			} else {
				ResetAnimationState(false, 0.25);
				doAnim = true;
				swingAnim = true;
			}
		}
		
		void BlockChanged(object sender, BlockChangedEventArgs e) {
			if (e.Block == 0) return;
			SetClickAnim(false);
		}
		
		internal void Update(double delta, Vector3 last) {
			if (swingAnim || !digAnim) {
				pos.Y = -0.4f * (float)Math.Sin(time * speed);
				if (swingAnim) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if (pos.Y > last.Y)
						lastType = held.type;
					held.type = lastType;
				}
			} else {
				if (time >= period * 0.25) DigSecondCycle();
				else DigFirstCycle();
			}
			time += delta;
			if (time > period)
				ResetAnimationState(true, 0.25);
		}
		
		// Based off incredible gifs from (Thanks goodlyay!)
		// https://dl.dropboxusercontent.com/u/12694594/slowBreakTranslate.gif
		// https://dl.dropboxusercontent.com/u/12694594/slowBreakRotate1.gif
		// https://dl.dropboxusercontent.com/u/12694594/slowBreakRotate2.gif
		// https://dl.dropboxusercontent.com/u/12694594/slowBreakFull.gif
		void DigFirstCycle() {			
			double angle = time * speed;
			pos.X = -0.325f * (float)Math.Sin(angle * 2);
			pos.Y = 0.20f * (float)Math.Sin(angle * 2 * 2);
			pos.Z = -0.325f * (float)Math.Sin(angle * 2);
			
			angleY = -90 * (float)Math.Sin(angle * 2);
			held.block.SwitchOrder = angleY <= -30;
		}
		
		void DigSecondCycle() {
			// Need to adjust angle so it starts at same point and of first cycle
			// And finishes cycle at original position
			double endFirst = period * 0.25;
			double angle = (endFirst * 2 + (time - endFirst) * 0.66667) * speed;
			
			pos.X = -0.325f * (float)Math.Sin(angle);
			pos.Y = 0.25f * (float)Math.Sin(angle * 2);
			pos.Z = -0.325f * (float)Math.Sin(angle);
			
			// For second cycle, rotate the block from 0-->15 then back to 15-->0.
			float rotX = Math.Max(0, (float)angle - 90 * Utils.Deg2Rad);
			if (rotX >= 45 * Utils.Deg2Rad) rotX = 90 * Utils.Deg2Rad - rotX;
			held.block.CosX = (float)Math.Cos(rotX * 0.33333);
			held.block.SinX = (float)Math.Sin(rotX * 0.33333);
			
			angleY = -90 * (float)Math.Sin(angle);
			held.block.SwitchOrder = angleY <= -30;
		}
		
		void ResetAnimationState(bool updateLastType, double period) {
			time = 0;
			doAnim = false;
			swingAnim = false;
			pos = Vector3.Zero;
			angleY = 0;
			this.period = period;
			speed = Math.PI / period;
			
			if (updateLastType)
				lastType = game.Inventory.Selected;
			pos = Vector3.Zero;
		}
	}
}