// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;
using BlockID = System.UInt16;

namespace ClassicalSharp.Mode {

	public interface IGameMode : IGameComponent {
		
		bool HandlesKeyDown(Key key);
		bool PickingLeft();
		bool PickingRight();
		void PickLeft(BlockID old);
		void PickMiddle(BlockID old);
		void PickRight(BlockID old, BlockID block);		
		Widget MakeHotbar();
		void BeginFrame(double delta);
		void EndFrame(double delta);
	}
}
