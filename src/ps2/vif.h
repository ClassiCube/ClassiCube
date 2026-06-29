// https://psi-rockin.github.io/ps2tek/#vifcommands
#define VIF_CMD_NOP      0x00 // no operation
#define VIF_CMD_STCYCL   0x01 // set CYCLE register
#define VIF_CMD_OFFSET   0x02 // set OFFSET register
#define VIF_CMD_BASE     0x03 // set BASE register
#define VIF_CMD_ITOP     0x04 // set ITOP register
#define VIF_CMD_STMOD    0x05 // set MODE register (used by unpack)
#define VIF_CMD_MSKPATH3 0x06 // sets path 3 masking on/off
#define VIF_CMD_MARK     0x07 // set MARK register
#define VIF_CMD_FLUSHE   0x10 // wait until current execution is finished
#define VIF_CMD_FLUSH    0x11 // wait for path 1+2 to be inactive
#define VIF_CMD_FLUSHA   0x13 // wait for path 1+2+3 to be inactive
#define VIF_CMD_MSCAL    0x14 // start execution at given address
#define VIF_CMD_MSCALF   0x15 // start execution at given address, after path 1+2 are inactive
#define VIF_CMD_MSCNT    0x17 // continue execution at current address
#define VIF_CMD_STMASK   0x20 // set MASK register
#define VIF_CMD_STROW    0x30 // set ROW register
#define VIF_CMD_STCOL    0x31 // set COL register
#define VIF_CMD_MPG      0x4A // load microprogram code
#define VIF_CMD_DIRECT   0x50 // transfer via path 2
#define VIF_CMD_DIRECTHL 0x51 // transfer via path 2, stalling for path3 if needed

// unpack command is more complicated
//  bits 0-1 = vector element type
//  bits 2-3 = vector element count
//  bit    4 = apply write masking
//  bits 5-6 = 11
// base value is therefore 1 1m ec et = 0x60
#define VIF_CMD_UNPACK_BASE 0x60
#define VIF_UNPACK_MASKED   0x10

#define VIF_UNPACK_S_32  0x00
#define VIF_UNPACK_S_16  0x01
#define VIF_UNPACK_S_8   0x02

#define VIF_UNPACK_V2_32  0x04
#define VIF_UNPACK_V2_16  0x05
#define VIF_UNPACK_V2_8   0x06

#define VIF_UNPACK_V3_32  0x08
#define VIF_UNPACK_V3_16  0x09
#define VIF_UNPACK_V3_8   0x0A

#define VIF_UNPACK_V4_32  0x0C
#define VIF_UNPACK_V4_16  0x0D
#define VIF_UNPACK_V4_8   0x0E
#define VIF_UNPACK_V4_5   0x0F


/*########################################################################################################################*
*---------------------------------------------------------VIF tags--------------------------------------------------------*
*#########################################################################################################################*/
#define VIFTAG(CMD, IMDT, NUM) (\
	(u32)((IMDT) & 0x0000FFFF) <<  0 | \
	(u32)((NUM)  & 0x000000FF) << 16 | \
 	(u32)((CMD)  & 0x000000FF) << 24)
// NOTE: bit 31 is IRQ flag, but not used currently here

// VIF no operation
#define VIF_NOP 0

// VIF load data into VU program memory
#define VIF_LOAD_CODE(addr, count) VIFTAG(VIF_CMD_MPG, addr, count)

// VIF transfer data to GIF (via path2)
#define VIF_XFER_GIF(qwords) VIFTAG(VIF_CMD_DIRECT, qwords, 0)

// VIF start execution at given VU program memory address
#define VIF_CALL(addr) VIFTAG(VIF_CMD_MSCAL, addr, 0)

// VIF wait for path1+2 transfers to finish, then start execution
#define VIF_FLUSH_CALL(addr) VIFTAG(VIF_CMD_MSCALF, addr, 0)

// VIF transfer data into VU data memory with automatic unpacking/conversion
#define VIF_UNPACK(type, addr, count) VIFTAG(VIF_CMD_UNPACK_BASE | type, addr, count)


// Packs 4 VIF tags into a qword, with T1 being first tag
#define PACK_VIFTAG_QWORD(Q, T1, T2, T3, T4) \
    (Q)->sw[0] = T1; \
    (Q)->sw[1] = T2; \
    (Q)->sw[2] = T3; \
    (Q)->sw[3] = T4;

#define PACK_VIFTAG_SINGLE(Q, TAG) PACK_VIFTAG_QWORD(Q, VIF_NOP, VIF_NOP, VIF_NOP, TAG)

