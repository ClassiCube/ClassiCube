// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Network.Protocols {

	public abstract class IProtocol {
		protected Game game;
		protected NetworkProcessor net;
		protected NetReader reader;
		protected NetWriter writer;
		
		public IProtocol(Game game) {
			this.game = game;
			net = (NetworkProcessor)game.Server;
			reader = net.reader;
			writer = net.writer;
		}

		public abstract void Reset();
		public abstract void Tick();
	}
}
