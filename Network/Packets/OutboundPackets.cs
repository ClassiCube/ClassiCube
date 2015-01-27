using System;
using System.Text;
using ClassicalSharp.Window;
using ClassicalSharp.Util;
using OpenTK;

namespace ClassicalSharp.Network.Packets {
	
	public sealed class KeepAliveOutbound : OutboundPacket {
		
		public KeepAliveOutbound() {
		}
		
		public override void WriteData( NetWriter writer ) {
		}
	}
	
	public sealed class LoginRequestOutbound : OutboundPacket {
		string username;
		
		public LoginRequestOutbound( string username ) {
			this.username = username;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( 14 );
			writer.WriteString( username );
			writer.WriteInt64( 0 );
			writer.WriteUInt8( 0 );
		}
	}
	
	public sealed class HandshakeOutbound : OutboundPacket {
		string username;
		
		public HandshakeOutbound( string username ) {
			this.username = username;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteString( username );
		}
	}
	
	public sealed class ChatOutbound : OutboundPacket {
		string value;
		
		public ChatOutbound( string value ) {
			this.value = value;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteString( value );
		}
	}
	
	public sealed class UseEntityOutbound : OutboundPacket {
		int entityId; // ignored?
		int targetId;
		bool leftClick;
		
		public UseEntityOutbound( int entityId, int targetId, bool leftClick ) {
			this.entityId = entityId;
			this.targetId = targetId;
			this.leftClick = leftClick;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( entityId );
			writer.WriteInt32( targetId );
			writer.WriteBoolean( leftClick );
		}
	}
	
	public sealed class RespawnOutbound : OutboundPacket {
		byte dimension;
		
		public RespawnOutbound( byte dimension ) {
			this.dimension = dimension;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteUInt8( dimension );
		}
	}	
	
	public sealed class PlayerOutbound : OutboundPacket {
		bool onGround;
		
		public PlayerOutbound( bool onGround ) {
			this.onGround = onGround;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteBoolean( onGround );
		}
	}
	
	public sealed class PlayerPosOutbound : OutboundPacket {
		bool onGround;
		double x, y, z, stanceY;
		
		public PlayerPosOutbound( bool onGround, double x, double y, double z, double stanceY ) {
			this.onGround = onGround;
			this.x = x;
			this.y = y;
			this.z = z;
			this.stanceY = stanceY;
		}
		
		public PlayerPosOutbound( bool onGround, Vector3 pos, float stanceY )
			: this( onGround, pos.X, pos.Y, pos.Z, stanceY ) {
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteFloat64( x );
			writer.WriteFloat64( y );
			writer.WriteFloat64( stanceY );
			writer.WriteFloat64( z );
			writer.WriteBoolean( onGround );
		}
	}
	
	public sealed class PlayerLookOutbound : OutboundPacket {
		bool onGround;
		float yaw, pitch;
		
		public PlayerLookOutbound( bool onGround, float yaw, float pitch ) {
			this.onGround = onGround;
			this.yaw = yaw;
			this.pitch = pitch;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteFloat32( yaw );
			writer.WriteFloat32( pitch );
			writer.WriteBoolean( onGround );
		}
	}
	
	public sealed class PlayerPosAndLookOutbound : OutboundPacket {
		bool onGround;
		double x, y, z, stanceY;
		float yaw, pitch;
		
		public PlayerPosAndLookOutbound( bool onGround, double x, double y, double z, double stanceY,
		                                float yaw, float pitch ) {
			this.onGround = onGround;
			this.x = x;
			this.y = y;
			this.z = z;
			this.yaw = yaw;
			this.pitch = pitch;
			this.stanceY = stanceY;
		}
		
		public PlayerPosAndLookOutbound( bool onGround, Vector3 pos, double stanceY, float yaw, float pitch ) :
			this( onGround, pos.X, pos.Y, pos.Z, stanceY, yaw, pitch ) {
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteFloat64( x );
			writer.WriteFloat64( y );
			writer.WriteFloat64( stanceY );
			writer.WriteFloat64( z );
			writer.WriteFloat32( yaw );
			writer.WriteFloat32( pitch );
			writer.WriteBoolean( onGround );
		}
	}
	
	public sealed class PlayerDiggingOutbound : OutboundPacket {
		DigStatus status;
		Vector3I loc;
		byte face;
		
		public PlayerDiggingOutbound( DigStatus status, Vector3I loc, byte face ) {
			this.status = status;
			this.face = face;
			this.loc = loc;
		}
				
		public override void WriteData( NetWriter writer ) {
			writer.WriteUInt8( (byte)status );
			writer.WriteInt32( loc.X );
			writer.WriteUInt8( (byte)loc.Y );
			writer.WriteInt32( loc.Z );
			writer.WriteUInt8( face );
		}
	}
	
	public enum DigStatus {
		Start = 0,
		Finish = 2,
		DropItem = 4,
	}
	
	public sealed class PlayerPlaceBlockOutbound : OutboundPacket {
		Vector3I loc;
		byte direction;
		Slot heldItem;
		
		public PlayerPlaceBlockOutbound( Vector3I loc, byte dir, Slot held ) {
			this.loc = loc;
			this.direction = dir;
			this.heldItem = held;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( loc.X );
			writer.WriteUInt8( (byte)loc.Y );
			writer.WriteInt32( loc.Z );
			writer.WriteUInt8( direction );
			writer.WriteSlot( heldItem );
		}
	}
	
	public sealed class HeldItemChangeOutbound : OutboundPacket {
		short index;
		
		public HeldItemChangeOutbound( short index  ) {
			this.index = index;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt16( index );
		}
	}
	
	public sealed class AnimationOutbound : OutboundPacket {
		int entityId;
		byte anim;
		
		public AnimationOutbound( int entityId, byte anim ) {
			this.entityId = entityId;
			this.anim = anim;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( entityId );
			writer.WriteUInt8( anim );
		}
	}
	
	public sealed class EntityActionOutbound : OutboundPacket {
		int entityId;
		byte action;
		
		public EntityActionOutbound( int entityId, byte action ) {
			this.entityId = entityId;
			this.action = action;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( entityId );
			writer.WriteUInt8( (byte)action );
		}
	}
	
	public sealed class AttachEntityOutbound : OutboundPacket {
		int entityId;
		int targetId;
		
		public AttachEntityOutbound( int entityId, int targetId ) {
			this.entityId = entityId;
			this.targetId = targetId;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( entityId );
			writer.WriteInt32( targetId );
		}
	}
	
	public sealed class CloseWindowOutbound : OutboundPacket {
		byte windowId;
		
		public CloseWindowOutbound( byte windowId ) {
			this.windowId = windowId;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteUInt8( windowId );
		}
	}
	
	public sealed class ClickWindowOutbound : OutboundPacket {
		byte windowId;
		short slotId;
		bool rightClick;
		short actionNumber;
		bool shift;
		Slot clickedItem;
		
		public ClickWindowOutbound( byte windowId, short slotId, bool rightClick,
		                           short actionNum, bool shift, Slot clicked ) {
			this.windowId = windowId;
			this.slotId = slotId;
			this.rightClick = rightClick;
			this.actionNumber = actionNum;
			this.shift = shift;
			this.clickedItem = clicked;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteUInt8( windowId );
			writer.WriteInt16( slotId );
			writer.WriteBoolean( rightClick );
			writer.WriteInt16( actionNumber );
			writer.WriteBoolean( shift );
			writer.WriteSlot( clickedItem );
		}
	}
	
	public sealed class ConfirmTransactionOutbound : OutboundPacket {
		byte windowId;
		short actionNumber;
		bool accepted;
		
		public ConfirmTransactionOutbound( byte windowId, short actNum, bool accepted ) {
			this.windowId = windowId;
			this.actionNumber = actNum;
			this.accepted = accepted;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteUInt8( windowId );
			writer.WriteInt16( actionNumber );
			writer.WriteBoolean( accepted );
		}
	}
	
	public sealed class UpdateSignOutbound : OutboundPacket {
		Vector3I loc;
		string[] text;
		
		public UpdateSignOutbound( Vector3I loc, string[] lines ) {
			this.loc = loc;
			this.text = lines;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteInt32( loc.X );
			writer.WriteInt16( (short)loc.Y );
			writer.WriteInt32( loc.Z );
			writer.WriteString( text[0] );
			writer.WriteString( text[1] );
			writer.WriteString( text[2] );
			writer.WriteString( text[3] );
		}
	}
	
	public sealed class DisconnectOutbound : OutboundPacket {
		string text;
		
		public DisconnectOutbound( string text ) {
			this.text = text;
		}
		
		public override void WriteData( NetWriter writer ) {
			writer.WriteString( text );
		}
	}
}