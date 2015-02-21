using System;

namespace Injector {
	
	public abstract class PacketC2S {
		
		public abstract void ReadData( StreamInjector reader );
	}
	
	public sealed class KeepAliveOutbound : PacketC2S {
		
		public override void ReadData( StreamInjector reader ) {
		}
	}
	
	public sealed class LoginRequestOutbound : PacketC2S {
		int protocolVersion;
		string username;
		ulong unused;
		byte unused2;
		
		public override void ReadData( StreamInjector reader ) {
			protocolVersion = reader.ReadInt32();
			username = reader.ReadString();
			unused = reader.ReadUInt64();
			unused2 = reader.ReadUInt8();
		}
	}
	
	public sealed class HandshakeOutbound : PacketC2S {
		string username;
		
		public override void ReadData( StreamInjector reader ) {
			username = reader.ReadString();
		}
	}
	
	public sealed class ChatOutbound : PacketC2S {
		string value;
		
		public override void ReadData( StreamInjector reader ) {
			value = reader.ReadString();
		}
	}
	
	public sealed class UseEntityOutbound : PacketC2S {
		int entityId; // ignored?
		int targetId;
		bool leftClick;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			targetId = reader.ReadInt32();
			leftClick = reader.ReadBool();
		}
	}
	
	public sealed class RespawnOutbound : PacketC2S {
		byte dimension;
		
		public override void ReadData( StreamInjector reader ) {
			dimension = reader.ReadUInt8();
		}
	}	
	
	public sealed class PlayerOutbound : PacketC2S {
		bool onGround;
		
		public override void ReadData( StreamInjector reader ) {
			onGround = reader.ReadBool();
		}
	}
	
	public sealed class PlayerPosOutbound : PacketC2S {
		bool onGround;
		double x, y, z, stanceY;
		
		public override void ReadData( StreamInjector reader ) {
			x = reader.ReadFloat64();
			y = reader.ReadFloat64();
			stanceY = reader.ReadFloat64();
			z = reader.ReadFloat64();
			onGround = reader.ReadBool();
		}
	}
	
	public sealed class PlayerLookOutbound : PacketC2S {
		bool onGround;
		float yaw, pitch;
		
		public override void ReadData( StreamInjector reader ) {
			yaw = reader.ReadFloat32();
			pitch = reader.ReadFloat32();
			onGround = reader.ReadBool();
		}
	}
	
	public sealed class PlayerPosAndLookOutbound : PacketC2S {
		bool onGround;
		double x, y, z, stanceY;
		float yaw, pitch;
		
		public override void ReadData( StreamInjector reader ) {
			x = reader.ReadFloat64();
			y = reader.ReadFloat64();
			stanceY = reader.ReadFloat64();
			z = reader.ReadFloat64();
			yaw = reader.ReadFloat32();
			pitch = reader.ReadFloat32();
			onGround = reader.ReadBool();
		}
	}
	
	public sealed class PlayerDiggingOutbound : PacketC2S {
		DigStatus status;
		int locX, locY, locZ;
		byte face;
				
		public override void ReadData( StreamInjector reader ) {
			status = (DigStatus)reader.ReadUInt8();
			locX = reader.ReadInt32();
			locY = reader.ReadUInt8();
			locZ = reader.ReadInt32();
			face = reader.ReadUInt8();
		}
	}
	
	public enum DigStatus {
		Start = 0,
		Finish = 2,
		DropItem = 4,
	}
	
	public sealed class PlayerPlaceBlockOutbound : PacketC2S {
		int locX, locY, locZ;
		byte direction;
		Slot heldItem;
		
		public override void ReadData( StreamInjector reader ) {
			locX = reader.ReadInt32();
			locY = reader.ReadUInt8();
			locZ = reader.ReadInt32();
			direction = reader.ReadUInt8();
			heldItem = reader.ReadSlot();
		}
	}
	
	public sealed class HeldItemChangeOutbound : PacketC2S {
		short index;
		
		public override void ReadData( StreamInjector reader ) {
			index = reader.ReadInt16();
		}
	}
	
	public sealed class AnimationOutbound : PacketC2S {
		int entityId;
		byte anim;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			anim = reader.ReadUInt8();
		}
	}
	
	public sealed class EntityActionOutbound : PacketC2S {
		int entityId;
		byte action;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			action = reader.ReadUInt8();
		}
	}
	
	public sealed class AttachEntityOutbound : PacketC2S {
		int entityId;
		int targetId;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			targetId = reader.ReadInt32();
		}
	}
	
	public sealed class CloseWindowOutbound : PacketC2S {
		byte windowId;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
		}
	}
	
	public sealed class ClickWindowOutbound : PacketC2S {
		byte windowId;
		short slotId;
		bool rightClick;
		short actionNumber;
		bool shift;
		Slot clickedItem;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			slotId = reader.ReadInt16();
			rightClick = reader.ReadBool();
			actionNumber = reader.ReadInt16();
			shift = reader.ReadBool();
			clickedItem = reader.ReadSlot();
		}
	}
	
	public sealed class ConfirmTransactionOutbound : PacketC2S {
		byte windowId;
		short actionNumber;
		bool accepted;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			actionNumber = reader.ReadInt16();
			accepted = reader.ReadBool();
		}
	}
	
	public sealed class UpdateSignOutbound : PacketC2S {
		int locX, locY, locZ;
		string line1, line2, line3, line4;
		
		public override void ReadData( StreamInjector reader ) {
			locX = reader.ReadInt32();
			locY = reader.ReadInt16();
			locZ = reader.ReadInt32();
			line1 = reader.ReadString();
			line2 = reader.ReadString();
			line3 = reader.ReadString();
			line4 = reader.ReadString();
		}
	}
	
	public sealed class DisconnectOutbound : PacketC2S {
		string text;
		
		public override void ReadData( StreamInjector reader ) {
			text = reader.ReadString();
		}
	}
}