// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Net.Sockets;
using ClassicalSharp.Entities;
using ClassicalSharp.Network.Protocols;
using OpenTK;
using OpenTK.Input;
using BlockID = System.UInt16;

namespace ClassicalSharp.Network {

	public sealed class NetworkProcessor : IServerConnection {
		
		public NetworkProcessor(Game game) {
			this.game = game;
			cpeData = new CPESupport();
			cpeData.game = game;
			IsSinglePlayer = false;
		}
		
		Socket socket;
		DateTime lastPacket;
		byte lastOpcode;
		public NetReader reader;
		public NetWriter writer;
		
		bool connecting = false;
		DateTime connectTimeout;
		const int timeoutSecs = 15;
		
		internal ClassicProtocol classic;
		internal CPEProtocol cpe;
		internal CPEProtocolBlockDefs cpeBlockDefs;
		internal WoMProtocol wom;
		internal CPESupport cpeData;
		
		public override void BeginConnect() {
			socket = new Socket(game.IPAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			Events.BlockChanged += BlockChanged;
			Disconnected = false;
			
			socket.Blocking = false;
			connecting = true;
			connectTimeout = DateTime.UtcNow.AddSeconds(timeoutSecs);
			
			try {
				socket.Connect(game.IPAddress, game.Port);
			} catch (SocketException ex) {
				SocketError err = ex.SocketErrorCode;
				if (err == SocketError.InProgress || err == SocketError.WouldBlock) return;
				FailConnect(ex);
			}
		}
		
		void TickConnect() {
			if (socket.Poll(0, SelectMode.SelectError)) {
				int err = (int)socket.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Error);
				FailConnect(new SocketException(err));
				return;
			}
			
			DateTime now = DateTime.UtcNow;
			// socket.Connected doesn't work here on some mono versions (e.g. 5.0.1.1)
			if (socket.Poll(0, SelectMode.SelectWrite)) {
				socket.Blocking = true;
				FinishConnect();
			} else if (now > connectTimeout) {
				FailConnect(null);
			} else {
				double leftSecs = (connectTimeout - now).TotalSeconds;
				Events.RaiseLoading((float)leftSecs / timeoutSecs);
			}
		}
		
		void FinishConnect() {
			connecting = false;
			Events.RaiseLoading(0);
			reader = new NetReader(socket);
			writer = new NetWriter(socket);
			
			classic = new ClassicProtocol(game);
			cpe = new CPEProtocol(game);
			cpeBlockDefs = new CPEProtocolBlockDefs(game);
			wom = new WoMProtocol(game);
			ResetState();
			
			classic.WriteLogin(game.Username, game.Mppass);
			SendPacket();
			lastPacket = DateTime.UtcNow;
		}
		
		void FailConnect(SocketException ex) {
			connecting = false;
			if (ex != null) {
				ErrorHandler.LogError("connecting to server", ex);
			}
			
			game.Disconnect("Failed to connect to " + game.IPAddress + ":" + game.Port,
			                "You failed to connect to the server. It's probably down!");
			Dispose();
		}
		
		public override void SendChat(string text) {
			if (String.IsNullOrEmpty(text) || connecting) return;
			
			while (text.Length > Utils.StringLength) {
				classic.WriteChat(text.Substring(0, Utils.StringLength), true);
				SendPacket();
				text = text.Substring(Utils.StringLength);
			}
			classic.WriteChat(text, false);
			SendPacket();
		}
		
		public override void SendPosition(Vector3 pos, float rotY, float headX) {
			classic.WritePosition(pos, rotY, headX);
			SendPacket();
		}
		
		public override void SendPlayerClick(MouseButton button, bool buttonDown, byte targetId, PickedPos pos) {
			cpe.WritePlayerClick(button, buttonDown, targetId, pos);
			SendPacket();
		}
		
		public override void Dispose() {
			if (Disconnected) return;
			Events.BlockChanged -= BlockChanged;
			socket.Close();
			Disconnected = true;
		}
		
		public override void Tick(ScheduledTask task) {
			if (Disconnected) return;
			if (connecting) { TickConnect(); return; }
			
			if ((DateTime.UtcNow - lastPacket).TotalSeconds >= 30) {
				CheckDisconnection(task.Interval);
			}
			if (Disconnected) return;
			
			try {
				reader.ReadPendingData();
			} catch (SocketException ex) {
				ErrorHandler.LogError("reading packets", ex);
				game.Disconnect("&eLost connection to the server", "I/O error when reading packets");
				return;
			}
			
			while ((reader.size - reader.index) > 0) {
				byte opcode = reader.buffer[reader.index];
				// Workaround for older D3 servers which wrote one byte too many for HackControl packets.
				if (cpeData.needD3Fix && lastOpcode == Opcode.CpeHackControl && (opcode == 0x00 || opcode == 0xFF)) {
					Utils.LogDebug("Skipping invalid HackControl byte from D3 server.");
					reader.Skip(1);
					
					LocalPlayer player = game.LocalPlayer;
					player.physics.jumpVel = 0.42f; // assume default jump height
					player.physics.serverJumpVel = player.physics.jumpVel;
					continue;
				}
				
				if (opcode >= handlers.Length) {
					game.Disconnect("Disconnected!", "Server sent invalid packet " + opcode + "!"); return;
				}
				if ((reader.size - reader.index) < packetSizes[opcode]) break;
				
				reader.Skip(1); // remove opcode
				lastOpcode = opcode;			
				lastPacket = DateTime.UtcNow;
				
				Action handler = handlers[opcode];
				if (handler == null) {
					game.Disconnect("Disconnected!", "Server sent invalid packet " + opcode + "!"); return;
				}
				handler();
			}
			
			reader.RemoveProcessed();
			// Network is ticked 60 times a second. We only send position updates 20 times a second.
			if ((netTicks % 3) == 0) CoreTick();
			netTicks++;
		}
		
		void CoreTick() {
			CheckAsyncResources();
			wom.Tick();
			classic.Tick();
			cpe.Tick();
			if (writer.index > 0) SendPacket();
		}
		
		/// <summary> Sets the incoming packet handler for the given packet id. </summary>
		public void Set(byte opcode, Action handler, int packetSize) {
			handlers[opcode] = handler;
			packetSizes[opcode] = (ushort)packetSize;
		}
		
		public void SendPacket() {
			if (Disconnected) {
				writer.index = 0;
				return;
			}
			
			try {
				writer.Send();
			} catch (SocketException) {
				// NOTE: Not immediately disconnecting, as otherwise we sometimes miss out on kick messages
				writer.index = 0;
			}
		}
		
		
		public override void Reset(Game game) {
			UsingExtPlayerList = false;
			UsingPlayerClick = false;
			SupportsPartialMessages = false;
			SupportsFullCP437 = false;
			
			for (int i = 0; i < handlers.Length; i++) {
				handlers[i] = null;
				packetSizes[i] = 0;
			}
			
			BlockInfo.SetMaxUsed(255);
			ResetState();
			Dispose();
		}
		
		void ResetState() {
			if (classic == null) return; // null if no successful connection ever made before
			
			cpeData.Reset();
			classic.Reset();
			cpe.Reset();
			cpeBlockDefs.Reset();
			wom.Reset();
			
			reader.ExtendedPositions = false; reader.ExtendedBlocks = false;
			writer.ExtendedPositions = false; writer.ExtendedBlocks = false;
		}
		
		internal Action[] handlers = new Action[Opcode.Count];
		internal ushort[] packetSizes = new ushort[Opcode.Count];
		
		void BlockChanged(Vector3I p, BlockID old, BlockID now) {
			if (now == Block.Air) {
				now = game.Inventory.Selected;
				classic.WriteSetBlock(p.X, p.Y, p.Z, false, now);
			} else {
				classic.WriteSetBlock(p.X, p.Y, p.Z, true,  now);
			}
			SendPacket();
		}
		
		public override void OnNewMap(Game game) {
			// wipe all existing entity states
			if (classic == null) return;
			for (int i = 0; i < EntityList.MaxCount; i++) {
				classic.RemoveEntity((byte)i);
			}
		}
		
		double testAcc = 0;
		void CheckDisconnection(double delta) {
			testAcc += delta;
			if (testAcc < 1) return;
			testAcc = 0;
			
			if (!socket.Connected || (socket.Available == 0 && socket.Poll(0, SelectMode.SelectRead))) {
				game.Disconnect("Disconnected!", "You've lost connection to the server");
			}
		}
	}
}