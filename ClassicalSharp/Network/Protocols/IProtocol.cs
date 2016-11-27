// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
		
		/// <summary> Initalises variables for this protocol. </summary>
		/// <remarks> Typically used to hook the packet handlers. </remarks>
		public virtual void Init() { }
		
		/// <summary> Resets the state of the variables for this protocol. </summary>
		public virtual void Reset() { }
		
		/// <summary> Performs a tick (if required) for this protocol. </summary>
		public virtual void Tick() { }
	}
}
