// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.Gui;
using ClassicalSharp.Network;
using ClassicalSharp.Textures;
using ClassicalSharp.Network.Protocols;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor : IServerConnection {
		
		public NetworkProcessor(Game window) {
			game = window;
			cpeData = new CPESupport(); game.Components.Add(cpeData);
		}
		
		public override bool IsSinglePlayer { get { return false; } }
		
		Socket socket;
		DateTime lastPacket;
		byte lastOpcode;
		public NetReader reader;
		public NetWriter writer;
		
		internal ClassicProtocol classic;
		internal CPEProtocol cpe;
		internal CPEProtocolBlockDefs cpeBlockDefs;
		internal WoMProtocol wom;

		internal CPESupport cpeData;
		internal bool receivedFirstPosition;
		internal byte[] needRemoveNames = new byte[256 >> 3];
		int netTicks, pingTicks;
		
		public override void Connect(IPAddress address, int port) {
			socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			try {
				socket.Connect(address, port);
			} catch (SocketException ex) {
				ErrorHandler.LogError("connecting to server", ex);
				game.Disconnect("Failed to connect to " + address + ":" + port,
				                "You failed to connect to the server. It's probably down!");
				Dispose();
				return;
			}
			
			reader = new NetReader(socket);
			writer = new NetWriter(socket);
			
			classic = new ClassicProtocol(game);
			classic.Init();
			cpe = new CPEProtocol(game);
			cpe.Init();
			cpeBlockDefs = new CPEProtocolBlockDefs(game);
			cpeBlockDefs.Init();
			wom = new WoMProtocol(game);
			wom.Init();
			
			Disconnected = false;
			receivedFirstPosition = false;
			lastPacket = DateTime.UtcNow;
			game.WorldEvents.OnNewMap += OnNewMap;
			game.UserEvents.BlockChanged += BlockChanged;

			classic.WriteLogin(game.Username, game.Mppass);
			SendPacket();
			lastPacket = DateTime.UtcNow;
		}
		
		public override void Dispose() {
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.UserEvents.BlockChanged -= BlockChanged;
			socket.Close();
			Disconnected = true;
		}
		
		public override void Tick(ScheduledTask task) {
			if (Disconnected) return;
			if ((DateTime.UtcNow - lastPacket).TotalSeconds >= 30) {
				CheckDisconnection(task.Interval);
			}
			if (Disconnected) return;
			
			try {
				reader.ReadPendingData();
			} catch (SocketException ex) {
				ErrorHandler.LogError("reading packets", ex);
				game.Disconnect("&eLost connection to the server", "I/O error when reading packets");
				Dispose();
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
				
				if (opcode > maxHandledPacket) {
					ErrorHandler.LogError("NetworkProcessor.Tick", "received invalid opcode: " + opcode);
					reader.Skip(1);
					continue;
				}
				
				if ((reader.size - reader.index) < packetSizes[opcode]) break;
				ReadPacket(opcode);
			}
			
			reader.RemoveProcessed();
			// Network is ticked 60 times a second. We only send position updates 20 times a second.
			if ((netTicks % 3) == 0) CoreTick();
			netTicks++;
		}
		
		void CoreTick() {
			CheckAsyncResources();
			wom.Tick();			
			if (!receivedFirstPosition) return;
			
			LocalPlayer player = game.LocalPlayer;
			classic.WritePosition(player.Position, player.HeadY, player.HeadX);
			pingTicks++;
			
			if (pingTicks >= 20 && cpeData.twoWayPing) {
				cpe.WriteTwoWayPing(false, PingList.NextTwoWayPingData());
				pingTicks = 0;
			}
			SendPacket();
		}
		
		/// <summary> Sets the incoming packet handler for the given packet id. </summary>
		public void Set(byte opcode, Action handler, int packetSize) {
			handlers[opcode] = handler;
			packetSizes[opcode] = (ushort)packetSize;
			maxHandledPacket = Math.Max(opcode, maxHandledPacket);
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
		
		void ReadPacket(byte opcode) {
			reader.Skip(1); // remove opcode
			lastOpcode = opcode;
			Action handler = handlers[opcode];
			lastPacket = DateTime.UtcNow;
			
			if (handler == null)
				throw new NotImplementedException("Unsupported packet:" + opcode);
			handler();
		}
		
		internal void SkipPacketData(byte opcode) {
			reader.Skip(packetSizes[opcode] - 1);
		}
		
		internal void Reset() {
			UsingExtPlayerList = false;
			UsingPlayerClick = false;
			SupportsPartialMessages = false;
			SupportsFullCP437 = false;
			addEntityHack = true;
			
			for (int i = 0; i < handlers.Length; i++) {
				handlers[i] = null;
				packetSizes[i] = 0;
			}
			if (classic == null) return; // null if no successful connection ever made before
			
			classic.Reset();
			cpe.Reset();
			cpeBlockDefs.Reset();
			
			reader.ExtendedPositions = false;
			writer.ExtendedPositions = false;
		}
		
		internal Action[] handlers = new Action[256];
		internal ushort[] packetSizes = new ushort[256];
		int maxHandledPacket = 0;
		
		void BlockChanged(object sender, BlockChangedEventArgs e) {
			Vector3I p = e.Coords;
			BlockID block = game.Inventory.Selected;
			
			if (e.Block == 0) {
				classic.WriteSetBlock(p.X, p.Y, p.Z, false, block);
			} else {
				classic.WriteSetBlock(p.X, p.Y, p.Z, true, e.Block);
			}
			SendPacket();
		}
		
		void OnNewMap(object sender, EventArgs e) {
			// wipe all existing entity states
			for (int i = 0; i < EntityList.MaxCount; i++)
				RemoveEntity((byte)i);
		}
		
		double testAcc = 0;
		void CheckDisconnection(double delta) {
			testAcc += delta;
			if (testAcc < 1) return;
			testAcc = 0;
			
			if (!socket.Connected || (socket.Poll(1000, SelectMode.SelectRead) && socket.Available == 0)) {
				game.Disconnect("Disconnected!", "You've lost connection to the server");
				Dispose();
			}
		}
	}
}