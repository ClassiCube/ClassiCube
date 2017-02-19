// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Mode {

	public interface IGameMode : IGameComponent {
		
		bool HandlesKeyDown(Key key);
		void PickLeft(BlockID old);
		void PickMiddle(BlockID old);
		void PickRight(BlockID old, BlockID block);
		bool PickEntity(byte id);
		Widget MakeHotbar();
	}
}
