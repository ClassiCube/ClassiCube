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

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor : IServerConnection {
		
		public NetworkProcessor(Game window) {
			game = window;
			cpeData = game.AddComponent(new CPESupport());
		}
		
		public override bool IsSinglePlayer { get { return false; } }
		
		Socket socket;
		DateTime lastPacket;
		Opcode lastOpcode;
		internal NetReader reader;
		internal NetWriter writer;
		
		internal ClassicProtocol classic;
		internal CPEProtocol cpe;
		internal CPEProtocolBlockDefs cpeBlockDefs;
		internal WoMProtocol wom;

		internal CPESupport cpeData;
		internal ScheduledTask task;
		internal bool receivedFirstPosition;
		internal byte[] needRemoveNames = new byte[256 >> 3];
		
		public override void Connect(IPAddress address, int port) {
			socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			try {
				socket.Connect(address, port);
			} catch (SocketException ex) {
				ErrorHandler.LogError("connecting to server", ex);
				game.Disconnect("&eUnable to reach " + address + ":" + port,
				                "Unable to establish an underlying connection");
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

			classic.SendLogin(game.Username, game.Mppass);
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
			
			LocalPlayer player = game.LocalPlayer;
			this.task = task;
			
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
					player.physics.jumpVel = 0.42f; // assume default jump height
					player.physics.serverJumpVel = player.physics.jumpVel;
					continue;
				}
				
				if (opcode > maxHandledPacket) {
					ErrorHandler.LogError("NetworkProcessor.Tick",
					                      "received an invalid opcode of " + opcode);
					reader.Skip(1);
					continue;
				}
				
				if ((reader.size - reader.index) < packetSizes[opcode]) break;
				ReadPacket(opcode);
			}
			
			reader.RemoveProcessed();
			if (receivedFirstPosition) {
				SendPosition(player.Position, player.HeadYawDegrees, player.PitchDegrees);
			}
			CheckAsyncResources();
			wom.Tick();
		}
		
		/// <summary> Sets the incoming packet handler for the given packet id. </summary>
		public void Set(Opcode opcode, Action handler, int packetSize) {
			handlers[(byte)opcode] = handler;
			packetSizes[(byte)opcode] = (ushort)packetSize;
			maxHandledPacket = Math.Max((byte)opcode, maxHandledPacket);
		}
		
		internal void SendPacket() {
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
			lastOpcode = (Opcode)opcode;
			Action handler = handlers[opcode];
			lastPacket = DateTime.UtcNow;
			
			if (handler == null)
				throw new NotImplementedException("Unsupported packet:" + (Opcode)opcode);
			handler();
		}
		
		internal void SkipPacketData(Opcode opcode) {
			reader.Skip(packetSizes[(byte)opcode] - 1);
		}
		
		internal void ResetProtocols() {
			UsingExtPlayerList = false;
			UsingPlayerClick = false;
			SupportsPartialMessages = false;
			SupportsFullCP437 = false;
			
			for (int i = 0; i < handlers.Length; i++)
				handlers[i] = null;
			
			packetSizes[(byte)Opcode.CpeEnvSetMapApperance] = 69;
			packetSizes[(byte)Opcode.CpeDefineBlockExt] = 85;
		}
		
		internal Action[] handlers = new Action[256];
		internal ushort[] packetSizes = new ushort[256];
		int maxHandledPacket = 0;
		
		void BlockChanged(object sender, BlockChangedEventArgs e) {
			Vector3I p = e.Coords;
			byte block = game.Inventory.HeldBlock;
			
			if (e.Block == 0) {
				classic.SendSetBlock(p.X, p.Y, p.Z, false, block);
			} else {
				classic.SendSetBlock(p.X, p.Y, p.Z, true, e.Block);
			}
		}
		
		void OnNewMap(object sender, EventArgs e) {
			// wipe all existing entity states
			for (int i = 0; i < 256; i++)
				RemoveEntity((byte)i);
		}
		
		double testAcc = 0;
		void CheckDisconnection(double delta) {
			testAcc += delta;
			if (testAcc < 1) return;
			testAcc = 0;
			
			if (!socket.Connected || (socket.Poll(1000, SelectMode.SelectRead) && socket.Available == 0)) {
				game.Disconnect("&eDisconnected from the server", "Connection timed out");
				Dispose();
			}
		}
	}
}