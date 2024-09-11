| SEGA 32X support code for the 68000
| by Chilly Willy
| Third part of rom header

        .text

| Standard Mars startup code at 0x3F0 - this is included as binary as SEGA
| uses this as a security key. US law allows us to include this as-is because
| it's used for security. The interoperability clauses in the law state this
| trumps copyright... and the Supreme Court agrees. :P

        .word   0x287C,0xFFFF,0xFFC0,0x23FC,0x0000,0x0000,0x00A1,0x5128
        .word   0x46FC,0x2700,0x4BF9,0x00A1,0x0000,0x7001,0x0CAD,0x4D41
        .word   0x5253,0x30EC,0x6600,0x03E6,0x082D,0x0007,0x5101,0x67F8
        .word   0x4AAD,0x0008,0x6710,0x4A6D,0x000C,0x670A,0x082D,0x0000
        .word   0x5101,0x6600,0x03B8,0x102D,0x0001,0x0200,0x000F,0x6706
        .word   0x2B78,0x055A,0x4000,0x7200,0x2C41,0x4E66,0x41F9,0x0000
        .word   0x04D4,0x6100,0x0152,0x6100,0x0176,0x47F9,0x0000,0x04E8
        .word   0x43F9,0x00A0,0x0000,0x45F9,0x00C0,0x0011,0x3E3C,0x0100
        .word   0x7000,0x3B47,0x1100,0x3B47,0x1200,0x012D,0x1100,0x66FA
        .word   0x7425,0x12DB,0x51CA,0xFFFC,0x3B40,0x1200,0x3B40,0x1100
        .word   0x3B47,0x1200,0x149B,0x149B,0x149B,0x149B,0x41F9,0x0000
        .word   0x04C0,0x43F9,0x00FF,0x0000,0x22D8,0x22D8,0x22D8,0x22D8
        .word   0x22D8,0x22D8,0x22D8,0x22D8,0x41F9,0x00FF,0x0000,0x4ED0
        .word   0x1B7C,0x0001,0x5101,0x41F9,0x0000,0x06BC,0xD1FC,0x0088
        .word   0x0000,0x4ED0,0x0404,0x303C,0x076C,0x0000,0x0000,0xFF00
        .word   0x8137,0x0002,0x0100,0x0000,0xAF01,0xD91F,0x1127,0x0021
        .word   0x2600,0xF977,0xEDB0,0xDDE1,0xFDE1,0xED47,0xED4F,0xD1E1
        .word   0xF108,0xD9C1,0xD1E1,0xF1F9,0xF3ED,0x5636,0xE9E9,0x9FBF
        .word   0xDFFF,0x4D41,0x5253,0x2049,0x6E69,0x7469,0x616C,0x2026
        .word   0x2053,0x6563,0x7572,0x6974,0x7920,0x5072,0x6F67,0x7261
        .word   0x6D20,0x2020,0x2020,0x2020,0x2020,0x2043,0x6172,0x7472
        .word   0x6964,0x6765,0x2056,0x6572,0x7369,0x6F6E,0x2020,0x2020
        .word   0x436F,0x7079,0x7269,0x6768,0x7420,0x5345,0x4741,0x2045
        .word   0x4E54,0x4552,0x5052,0x4953,0x4553,0x2C4C,0x5444,0x2E20
        .word   0x3139,0x3934,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020
        .word   0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020
        .word   0x2020,0x2020,0x2020,0x524F,0x4D20,0x5665,0x7273,0x696F
        .word   0x6E20,0x312E,0x3000,0x48E7,0xC040,0x43F9,0x00C0,0x0004
        .word   0x3011,0x303C,0x8000,0x323C,0x0100,0x3E3C,0x0012,0x1018
        .word   0x3280,0xD041,0x51CF,0xFFF8,0x4CDF,0x0203,0x4E75,0x48E7
        .word   0x81C0,0x41F9,0x0000,0x063E,0x43F9,0x00C0,0x0004,0x3298
        .word   0x3298,0x3298,0x3298,0x3298,0x3298,0x3298,0x2298,0x3341
        .word   0xFFFC,0x3011,0x0800,0x0001,0x66F8,0x3298,0x3298,0x7000
        .word   0x22BC,0xC000,0x0000,0x7E0F,0x3340,0xFFFC,0x3340,0xFFFC
        .word   0x3340,0xFFFC,0x3340,0xFFFC,0x51CF,0xFFEE,0x22BC,0x4000
        .word   0x0010,0x7E09,0x3340,0xFFFC,0x3340,0xFFFC,0x3340,0xFFFC
        .word   0x3340,0xFFFC,0x51CF,0xFFEE,0x4CDF,0x0381,0x4E75,0x8114
        .word   0x8F01,0x93FF,0x94FF,0x9500,0x9600,0x9780,0x4000,0x0080
        .word   0x8104,0x8F02,0x48E7,0xC140,0x43F9,0x00A1,0x5180,0x08A9
        .word   0x0007,0xFF80,0x66F8,0x3E3C,0x00FF,0x7000,0x7200,0x337C
        .word   0x00FF,0x0004,0x3341,0x0006,0x3340,0x0008,0x4E71,0x0829
        .word   0x0001,0x000B,0x66F8,0x0641,0x0100,0x51CF,0xFFE8,0x4CDF
        .word   0x0283,0x4E75,0x48E7,0x8180,0x41F9,0x00A1,0x5200,0x08A8
        .word   0x0007,0xFF00,0x66F8,0x3E3C,0x001F,0x20C0,0x20C0,0x20C0
        .word   0x20C0,0x51CF,0xFFF6,0x4CDF,0x0181,0x4E75,0x41F9,0x00FF
        .word   0x0000,0x3E3C,0x07FF,0x7000,0x20C0,0x20C0,0x20C0,0x20C0
        .word   0x20C0,0x20C0,0x20C0,0x20C0,0x51CF,0xFFEE,0x3B7C,0x0000
        .word   0x1200,0x7E0A,0x51CF,0xFFFE,0x43F9,0x00A1,0x5100,0x7000
        .word   0x2340,0x0020,0x2340,0x0024,0x1B7C,0x0003,0x5101,0x2E79
        .word   0x0088,0x0000,0x0891,0x0007,0x66FA,0x7000,0x3340,0x0002
        .word   0x3340,0x0004,0x3340,0x0006,0x2340,0x0008,0x2340,0x000C
        .word   0x3340,0x0010,0x3340,0x0030,0x3340,0x0032,0x3340,0x0038
        .word   0x3340,0x0080,0x3340,0x0082,0x08A9,0x0000,0x008B,0x66F8
        .word   0x6100,0xFF12,0x08E9,0x0000,0x008B,0x67F8,0x6100,0xFF06
        .word   0x08A9,0x0000,0x008B,0x6100,0xFF3C,0x303C,0x0040,0x2229
        .word   0x0020,0x0C81,0x5351,0x4552,0x6700,0x0092,0x303C,0x0080
        .word   0x2229,0x0020,0x0C81,0x5344,0x4552,0x6700,0x0080,0x21FC
        .word   0x0088,0x02A2,0x0070,0x303C,0x0002,0x7200,0x122D,0x0001
        .word   0x1429,0x0080,0xE14A,0x8242,0x0801,0x000F,0x660A,0x0801
        .word   0x0006,0x6700,0x0058,0x6008,0x0801,0x0006,0x6600,0x004E
        .word   0x7020,0x41F9,0x0088,0x0000,0x3C28,0x018E,0x4A46,0x6700
        .word   0x0010,0x3429,0x0028,0x0C42,0x0000,0x67F6,0xB446,0x662C
        .word   0x7000,0x2340,0x0028,0x2340,0x002C,0x3E14,0x2C7C,0xFFFF
        .word   0xFFC0,0x4CD6,0x7FF9,0x44FC,0x0000,0x6014,0x43F9,0x00A1
        .word   0x5100,0x3340,0x0006,0x303C,0x8000,0x6004,0x44FC,0x0001

| At this point (0x800), the Work RAM is clear, the VDP initialized, the
| VRAM/VSRAM/CRAM cleared, the Z80 initialized, the 32X initialized,
| both 32X framebuffers cleared, the 32X palette cleared, the SH2s
| checked for a startup error, the adapter TV mode matches the MD TV
| mode, and the ROM checksum checked. If any error is detected, the
| carry is set, otherwise it is cleared. The 68000 main code is now
| entered.

        jmp     __start+0x00880000+0x3F0

| 68000 General exception handler at 0x806

        jmp     __except+0x00880000+0x3F0

| 68000 Level 4 interrupt handler at 0x80C - HBlank IRQ

        jmp     __hblank+0x00880000+0x3F0

| 68000 Level 6 interrupt handler at 0x812 - VBlank IRQ

        jmp     __vblank+0x00880000+0x3F0

__except:
        move.l  d0,-(sp)
        move.l  4(sp),d0            /* jump table return address */
        sub.w   #0x206,d0           /* 0 = BusError, 6 = AddrError, etc */

| handle exception

        move.l  (sp)+,d0
        addq.l  #4,sp               /* pop jump table return address */
        rte

__hblank:
        rte

__vblank:
        move.l  d0,-(sp)
        move.l  0xFF0FFC,d0
        beq.b   1f
        move.l  a0,-(sp)
        movea.l d0,a0
        jmp     (a0)
1:
        move.l  (sp)+,d0
        rte


__start:
        move.b  #0,0xA15107             /* clear RV - allow SH2 to access ROM */
0:
        cmp.l   #0x4D5F4F4B,0xA15120    /* M_OK */
        bne.b   0b                      /* wait for master ok */
1:
        cmp.l   #0x535F4F4B,0xA15124    /* S_OK */
        bne.b   1b                      /* wait for slave ok */

        bsr     init_hardware           /* initialize the console hardware */

| Copy 68000 main loop to Work RAM to keep contention for the ROM with
| SH2s to a minimum.
        lea     __m68k_start(pc),a0
        lea     0x00FF1000,a1
        move.w  #__m68k_end-__m68k_start-1,d0
cpyloop:
        move.b  (a0)+,(a1)+
        dbra    d0,cpyloop

        move.w  #0,0xA15128         /* controller 1 */
        move.w  #0,0xA1512A         /* controller 2 */
| look for mouse
        lea     0xA10003,a0
0:
        bsr     get_mky
        cmpi.l  #-2,d0
        beq.b   0b                  /* timeout */
        cmpi.l  #-1,d0
        beq.b   1f                  /* no mouse */
        move.w  #0xF001,0xA15128    /* mouse in port 1 */
1:
        lea     2(a0),a0
2:
        bsr     get_mky
        cmpi.l  #-2,d0
        beq.b   2b                  /* timeout */
        cmpi.l  #-1,d0
        beq.b   3f                  /* no mouse */
        move.w  #0xF001,0xA1512A    /* mouse in port 2 */
3:
        move.l  #0,0xA1512C         /* clear the vblank count */

| jump to main loop in Work RAM
        jmp     0xFF1000.l

| this block of code must be pc relative as it's copied into Work RAM

__m68k_start:
        move.b  #1,0xA15107         /* set RV */
        move.b  #2,0xA130F1         /* SRAM disabled, write protected */
        move.b  #0,0xA15107         /* clear RV */

        move.w  0xA15100,d0
        or.w    #0x8000,d0
        move.w  d0,0xA15100         /* set FM - allow SH2 access to MARS hw */
        move.l  #0,0xA15120         /* let Master SH2 run */

        lea     vert_blank(pc),a0
        move.l  a0,0xFF0FFC         /* set vertical blank interrupt handler */
        move.w  #0x2000,sr          /* enable interrupts */

main_loop:
        move.w  0xA15120,d0         /* get COMM0 */
        bne.b   handle_req

| any other 68000 tasks here

        bra.b   main_loop

| process request from Master SH2
handle_req:
        cmpi.w  #0x01FF,d0
        bls     read_sram
        cmpi.w  #0x02FF,d0
        bls     write_sram
        cmpi.w  #0x03FF,d0
        bls     read_mouse
        cmpi.w  #0x04FF,d0
        bls     clear_screen
        cmpi.w  #0x05FF,d0
        bls     set_offset
        cmpi.w  #0x06FF,d0
        bls     set_ntable
        cmpi.w  #0x07FF,d0
        bls     set_vram

| unknown command
        move.w  #0,0xA15120         /* done */
        bra.b   main_loop

read_sram:
        move.w  #0x2700,sr          /* disable ints */
        moveq   #0,d1
        moveq   #0,d0
        move.w  0xA15122,d0         /* COMM2 holds offset */
        add.l   d0,d0               /* sram is every other byte */
        lea     0x200000,a0
        move.b  #1,0xA15107         /* set RV */
        move.b  #3,0xA130F1         /* SRAM enabled, write protected */
        move.b  1(a0,d0.l),d1       /* read SRAM */
        move.b  #2,0xA130F1         /* SRAM disabled, write protected */
        move.b  #0,0xA15107         /* clear RV */
        move.w  d1,0xA15122         /* COMM2 holds return byte */
        move.w  #0,0xA15120         /* done */
        move.w  #0x2000,sr          /* enable ints */
        bra     main_loop

write_sram:
        move.w  #0x2700,sr          /* disable ints */
        moveq   #0,d1
        move.w  0xA15122,d1         /* COMM2 holds offset */
        add.l   d1,d1               /* sram is every other byte */
        lea     0x200000,a0
        move.b  #1,0xA15107         /* set RV */
        move.b  #1,0xA130F1         /* SRAM enabled, write enabled */
        move.b  d0,1(a0,d1.l)       /* write SRAM */
        move.b  #2,0xA130F1         /* SRAM disabled, write protected */
        move.b  #0,0xA15107         /* clear RV */
        move.w  #0,0xA15120         /* done */
        move.w  #0x2000,sr          /* enable ints */
        bra     main_loop

read_mouse:
        tst.b   d0
        bne.b   1f                  /* skip port 1 */

        move.w  0xA15128,d0
        andi.w  #0xF001,d0
        cmpi.w  #0xF001,d0
        bne.b   1f                  /* no mouse in port 1 */
        lea     0xA10003,a0
        bsr     get_mky
        bset    #31,d0
        move.w  d0,0xA15122
        swap    d0
        move.w  d0,0xA15120
0:
        move.w  0xA15120,d0
        bne.b   0b                  /* wait for SH2 to read mouse value */
        bra     main_loop
1:
        move.w  0xA1512A,d0
        andi.w  #0xF001,d0
        cmpi.w  #0xF001,d0
        bne.b   3f                  /* no mouse in port 2 */
        lea     0xA10005,a0
        bsr     get_mky
        bset    #31,d0
        move.w  d0,0xA15122
        swap    d0
        move.w  d0,0xA15120
2:
        move.w  0xA15120,d0
        bne.b   2b                  /* wait for SH2 to read mouse value */
        bra     main_loop
3:
        move.l  #-1,d0              /* no mouse */
        move.w  d0,0xA15122
        swap    d0
        move.w  d0,0xA15120
4:
        move.w  0xA15120,d0
        bne.b   4b                  /* wait for SH2 to read mouse value */
        bra     main_loop

clear_screen:
        moveq   #0,d0
        lea     0xC00000,a0
        move.w  #0x8F02,4(a0)       /* set INC to 2 */
        move.l  #0x60000003,d1      /* VDP write VRAM at 0xE000 (scroll plane B) */
        move.l  d1,4(a0)            /* write VRAM at plane B start */
        move.w  #64*32-1,d1
1:
        move.w  d0,(a0)             /* clear name pattern */
        dbra    d1,1b
        move.w  #0,0xA15120         /* done */
        bra     main_loop

set_offset:
        moveq   #0,d7
        move.w  0xA15122,d7         /* offset */
        move.w  #0,0xA15120         /* done */
        bra     main_loop

set_ntable:
        lea     0xC00000,a1
        move.w  #0x8F02,4(a1)       /* set INC to 2 */
        move.l  d7,d2               /* name table offset */
        swap    d2
        ori.l   #0x60000003,d2      /* OR cursor with VDP write VRAM at 0xE000 (scroll plane B) */
        move.l  d2,4(a1)            /* write VRAM at location of cursor in plane B */
        move.w  0xA15122,d0         /* pattern name */
        move.w  d0,(a1)             /* set pattern name for character */
        addq.l  #2,d7               /* increment offset */
        move.w  #0,0xA15120         /* done */
        bra     main_loop

set_vram:
        lea     0xC00000,a1
        move.w  #0x8F02,4(a1)       /* set INC to 2 */
        move.l  d7,d2               /* vram offset */
        swap    d2
        rol.l   #2,d2
        lsl.w   #2,d2
        lsr.l   #2,d2
        ori.l   #0x40000000,d2      /* VDP write VRAM */
        move.l  d2,4(a1)            /* write VRAM at location of cursor in plane B */
        move.w  0xA15122,d0         /* data word */
        move.w  d0,(a1)             /* set vram word */
        addq.l  #2,d7               /* increment offset */
        move.w  #0,0xA15120         /* done */
        bra     main_loop


vert_blank:
        move.l  d1,-(sp)
        move.l  d2,-(sp)

        /* read controllers */
        move.w  0xA15128,d0
        andi.w  #0xF000,d0
        cmpi.w  #0xF000,d0
        beq.b   0f                  /* no pad in port 1 (or mouse) */
        lea     0xA10003,a0
        bsr.b   get_pad
        move.w  d2,0xA15128         /* controller 1 current value */
0:
        move.w  0xA1512A,d0
        andi.w  #0xF000,d0
        cmpi.w  #0xF000,d0
        beq.b   1f                  /* no pad in port 2 (or mouse) */
        lea     0xA10005,a0
        bsr.b   get_pad
        move.w  d2,0xA1512A         /* controller 2 current value */
1:
        move.l  0xA1512C,d0
        addq.l  #1,d0
        move.l  d0,0xA1512C         /* increment the vblank count */

        move.l  (sp)+,d2
        move.l  (sp)+,d1
        movea.l (sp)+,a0
        move.l  (sp)+,d0
        rte

| get current pad value
| entry: a0 = pad control port
| exit:  d2 = pad value (0 0 0 1 M X Y Z S A C B R L D U) or (0 0 0 0 0 0 0 0 S A C B R L D U)
get_pad:
        bsr.b   get_input       /* - 0 s a 0 0 d u - 1 c b r l d u */
        move.w  d0,d1
        andi.w  #0x0C00,d0
        bne.b   no_pad
        bsr.b   get_input       /* - 0 s a 0 0 d u - 1 c b r l d u */
        bsr.b   get_input       /* - 0 s a 0 0 0 0 - 1 c b m x y z */
        move.w  d0,d2
        bsr.b   get_input       /* - 0 s a 1 1 1 1 - 1 c b r l d u */
        andi.w  #0x0F00,d0      /* 0 0 0 0 1 1 1 1 0 0 0 0 0 0 0 0 */
        cmpi.w  #0x0F00,d0
        beq.b   common          /* six button pad */
        move.w  #0x010F,d2      /* three button pad */
common:
        lsl.b   #4,d2           /* - 0 s a 0 0 0 0 m x y z 0 0 0 0 */
        lsl.w   #4,d2           /* 0 0 0 0 m x y z 0 0 0 0 0 0 0 0 */
        andi.w  #0x303F,d1      /* 0 0 s a 0 0 0 0 0 0 c b r l d u */
        move.b  d1,d2           /* 0 0 0 0 m x y z 0 0 c b r l d u */
        lsr.w   #6,d1           /* 0 0 0 0 0 0 0 0 s a 0 0 0 0 0 0 */
        or.w    d1,d2           /* 0 0 0 0 m x y z s a c b r l d u */
        eori.w  #0x1FFF,d2      /* 0 0 0 1 M X Y Z S A C B R L D U */
        rts

no_pad:
        .ifdef  HAS_SMS_PAD
        move.b  (a0),d0         /* - 1 c b r l d u */
        andi.w  #0x003F,d0      /* 0 0 0 0 0 0 0 0 0 0 c b r l d u */
        eori.w  #0x003F,d0      /* 0 0 0 0 0 0 0 0 0 0 C B R L D U */
        .else
        move.w  #0xF000,d0      /* SEGA_CTRL_NONE */
        .endif
        rts

| read single phase from controller
get_input:
        move.b  #0x00,(a0)
        nop
        nop
        move.b  (a0),d0
        move.b  #0x40,(a0)
        lsl.w   #8,d0
        move.b  (a0),d0
        rts

| get current mouse value
| entry: a0 = mouse control port
| exit:  d0 = mouse value (0  0  0  0  0  0  0  0  YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0) or -2 (timeout) or -1 (no mouse)
get_mky:
        move.w  sr,d2
        move.w  #0x2700,sr      /* disable ints */

        move.b	#0x60,6(a0)     /* set direction bits */
        nop
        nop
        move.b  #0x60,(a0)      /* first phase of mouse packet */
        nop
        nop
0:
        btst    #4,(a0)
        beq.b   0b              /* wait on handshake */
        move.b  (a0),d0
        andi.b  #15,d0
        bne     mky_err         /* not 0 means not mouse */

        move.b  #0x20,(a0)      /* next phase */
        move.w  #254,d1         /* number retries before timeout */
1:
        btst    #4,(a0)
        bne.b   2f              /* handshake */
        dbra    d1,1b
        bra     timeout_err
2:
        move.b  (a0),d0
        andi.b  #15,d0
        move.b  #0,(a0)         /* next phase */
        cmpi.b  #11,d0
        bne     mky_err         /* not 11 means not mouse */
3:
        btst    #4,(a0)
        beq.b   4f              /* handshake */
        dbra    d1,3b
        bra     timeout_err
4:
        move.b  (a0),d0         /* specs say should be 15 */
        nop
        nop
        move.b  #0x20,(a0)      /* next phase */
        nop
        nop
5:
        btst    #4,(a0)
        bne.b   6f
        dbra    d1,5b
        bra     timeout_err
6:
        move.b  (a0),d0         /* specs say should be 15 */
        nop
        nop
        move.b  #0,(a0)         /* next phase */
        moveq   #0,d0           /* clear reg to hold packet */
        nop
7:
        btst    #4,(a0)
        beq.b   8f              /* handshake */
        dbra    d1,7b
        bra     timeout_err
8:
        move.b  (a0),d0         /* YO XO YS XS */
        move.b  #0x20,(a0)      /* next phase */
        lsl.w   #8,d0           /* save nibble */
9:
        btst    #4,(a0)
        bne.b   10f             /* handshake */
        dbra    d1,9b
        bra     timeout_err
10:
        move.b  (a0),d0         /* S  M  R  L */
        move.b  #0,(a0)         /* next phase */
        lsl.b   #4,d0           /* YO XO YS XS S  M  R  L  0  0  0  0 */
        lsl.l   #4,d0           /* YO XO YS XS S  M  R  L  0  0  0  0  0  0  0  0 */
11:
        btst    #4,(a0)
        beq.b   12f             /* handshake */
        dbra    d1,11b
        bra     timeout_err
12:
        move.b  (a0),d0         /* X7 X6 X5 X4 */
        move.b  #0x20,(a0)      /* next phase */
        lsl.b   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 0  0  0  0 */
        lsl.l   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 0  0  0  0  0  0  0  0 */
13:
        btst    #4,(a0)
        bne.b   14f             /* handshake */
        dbra    d1,13b
        bra     timeout_err
14:
        move.b  (a0),d0         /* X3 X2 X1 X0 */
        move.b  #0,(a0)         /* next phase */
        lsl.b   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 0  0  0  0 */
        lsl.l   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 0  0  0  0  0  0  0  0 */
15:
        btst    #4,(a0)
        beq.b   16f             /* handshake */
        dbra    d1,15b
        bra     timeout_err
16:
        move.b  (a0),d0         /* Y7 Y6 Y5 Y4 */
        move.b  #0x20,(a0)      /* next phase */
        lsl.b   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 Y7 Y6 Y5 Y4 0  0  0  0 */
        lsl.l   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 Y7 Y6 Y5 Y4 0  0  0  0  0  0  0  0*/
17:
        btst    #4,(a0)
        beq.b   18f             /* handshake */
        dbra    d1,17b
        bra     timeout_err
18:
        move.b  (a0),d0         /* Y3 Y2 Y1 Y0 */
        move.b  #0x60,(a0)      /* first phase */
        lsl.b   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 0  0  0  0 */
        lsr.l   #4,d0           /* YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 */
19:
        btst    #4,(a0)
        beq.b   19b             /* wait on handshake */

        move.w  d2,sr           /* restore int status */
        rts

timeout_err:
        move.b  #0x60,(a0)      /* first phase */
        nop
        nop
0:
        btst    #4,(a0)
        beq.b   0b              /* wait on handshake */

        move.w  d2,sr           /* restore int status */
        moveq   #-2,d0
        rts

mky_err:
        move.b  #0x40,6(a0)     /* set direction bits */
        nop
        nop
        move.b  #0x40,(a0)

        move.w  d2,sr           /* restore int status */
        moveq   #-1,d0
        rts

__m68k_end:

        .align 4

| Initialize the hardware & load font tiles

init_hardware:
        movem.l d2-d7/a2-a6,-(sp)

| init joyports
        lea     0xA10000,a5
        move.b  #0x40,0x09(a5)
        move.b  #0x40,0x0B(a5)
        move.b  #0x40,0x03(a5)
        move.b  #0x40,0x05(a5)

        lea     0xC00000,a3             /* VDP data reg */
        lea     0xC00004,a4             /* VDP cmd/sts reg */

| wait on VDP DMA (in case we reset in the middle of DMA)
        move.w  #0x8114,(a4)            /* display off, dma enabled */
0:
        move.w  (a4),d0                 /* read VDP status */
        btst    #1,d0                   /* DMA busy? */
        bne.b   0b                      /* yes */

        moveq   #0,d0
        move.w  #0x8000,d5              /* set VDP register 0 */
        move.w  #0x0100,d7

| Set VDP registers
        lea     InitVDPRegs(pc),a5
        moveq   #18,d1
1:
        move.b  (a5)+,d5                /* lower byte = register data */
        move.w  d5,(a4)                 /* set VDP register */
        add.w   d7,d5                   /* + 0x0100 = next register */
        dbra    d1,1b

| clear VRAM
        move.w  #0x8F02,(a4)            /* set INC to 2 */
        move.l  #0x40000000,(a4)        /* write VRAM address 0 */
        move.w  #0x7FFF,d1              /* 32K - 1 words */
2:
        move.w  d0,(a3)                 /* clear VRAM */
        dbra    d1,2b

| The VDP state at this point is: Display disabled, ints disabled, Name Tbl A at 0xC000,
| Name Tbl B at 0xE000, Name Tbl W at 0xB000, Sprite Attr Tbl at 0xA800, HScroll Tbl at 0xAC00,
| H40 V28 mode, and Scroll size is 64x32.

| Clear CRAM
        lea     InitVDPRAM(pc),a5
        move.l  (a5)+,(a4)              /* set reg 1 and reg 15 */
        move.l  (a5)+,(a4)              /* write CRAM address 0 */
        moveq   #31,d3
3:
        move.l  d0,(a3)
        dbra    d3,3b

| Clear VSRAM
        move.l  (a5)+,(a4)              /* write VSRAM address 0 */
        moveq   #19,d4
4:
        move.l  d0,(a3)
        dbra    d4,4b

| halt Z80 and init FM chip
        /* Allow the 68k to access the FM chip */
        move.w  #0x100,0xA11100
        move.w  #0x100,0xA11200

| reset YM2612
        lea     FMReset(pc),a5
        lea     0xA00000,a0
        move.w  #0x4000,d1
        moveq   #26,d2
5:
        move.b  (a5)+,d1                /* FM reg */
        move.b  (a5)+,0(a0,d1.w)        /* FM data */
        nop
        nop
        dbra    d2,5b

        moveq   #0x30,d0
        moveq   #0x5F,d2
6:
        move.b  d0,0x4000(a0)           /* FM reg */
        nop
        nop
        move.b  #0xFF,0x4001(a0)        /* FM data */
        nop
        nop
        move.b  d0,0x4002(a0)           /* FM reg */
        nop
        nop
        move.b  #0xFF,0x4003(a0)        /* FM data */
        nop
        nop
        addq.b  #1,d0
        dbra    d2,6b

| reset PSG
        lea     PSGReset(pc),a5
        lea     0xC00000,a0
        move.b  (a5)+,0x0011(a0)
        move.b  (a5)+,0x0011(a0)
        move.b  (a5)+,0x0011(a0)
        move.b  (a5),0x0011(a0)

| load font tile data
        move.w  #0x8F02,(a4)            /* INC = 2 */
        move.l  #0x40000000,(a4)        /* write VRAM address 0 */
        lea     font_data(pc),a0
        move.w  #0x6B*8-1,d2
7:
        move.l  (a0)+,d0                /* font fg mask */
        move.l  d0,d1
        not.l   d1                      /* font bg mask */
        andi.l  #0x11111111,d0          /* set font fg color */
        andi.l  #0x00000000,d1          /* set font bg color */
        or.l    d1,d0
        move.l  d0,(a3)                 /* set tile line */
        dbra    d2,7b

| set the default palette for text
        move.l  #0xC0000000,(a4)        /* write CRAM address 0 */
        move.l  #0x00000CCC,(a3)        /* entry 0 (black) and 1 (lt gray) BGR */
        move.l  #0xC0200000,(a4)        /* write CRAM address 32 */
        move.l  #0x000000A0,(a3)        /* entry 16 (black) BGR and 17 (green) */
        move.l  #0xC0400000,(a4)        /* write CRAM address 64 */
        move.l  #0x0000000A,(a3)        /* entry 32 (black) BGR and 33 (red) */

        move.w  #0x8174,(a4)            /* display on, vblank enabled */

        movem.l (sp)+,d2-d7/a2-a6
        rts

| VDP register initialization values
InitVDPRegs:
        .byte   0x04    /* 8004 => write reg 0 = /IE1 (no HBL INT), /M3 (enable read H/V cnt) */
        .byte   0x14    /* 8114 => write reg 1 = /DISP (display off), /IE0 (no VBL INT), M1 (DMA enabled), /M2 (V28 mode) */
        .byte   0x30    /* 8230 => write reg 2 = Name Tbl A = 0xC000 */
        .byte   0x2C    /* 832C => write reg 3 = Name Tbl W = 0xB000 */
        .byte   0x07    /* 8407 => write reg 4 = Name Tbl B = 0xE000 */
        .byte   0x54    /* 8554 => write reg 5 = Sprite Attr Tbl = 0xA800 */
        .byte   0x00    /* 8600 => write reg 6 = always 0 */
        .byte   0x00    /* 8700 => write reg 7 = BG color */
        .byte   0x00    /* 8800 => write reg 8 = always 0 */
        .byte   0x00    /* 8900 => write reg 9 = always 0 */
        .byte   0x00    /* 8A00 => write reg 10 = HINT = 0 */
        .byte   0x00    /* 8B00 => write reg 11 = /IE2 (no EXT INT), full scroll */
        .byte   0x81    /* 8C81 => write reg 12 = H40 mode, no lace, no shadow/hilite */
        .byte   0x2B    /* 8D2B => write reg 13 = HScroll Tbl = 0xAC00 */
        .byte   0x00    /* 8E00 => write reg 14 = always 0 */
        .byte   0x01    /* 8F01 => write reg 15 = data INC = 1 */
        .byte   0x01    /* 9001 => write reg 16 = Scroll Size = 64x32 */
        .byte   0x00    /* 9100 => write reg 17 = W Pos H = left */
        .byte   0x00    /* 9200 => write reg 18 = W Pos V = top */

        .align  2

| VDP Commands
InitVDPRAM:
        .word   0x8104, 0x8F01  /* set registers 1 (display off) and 15 (INC = 1) */
        .word   0xC000, 0x0000  /* write CRAM address 0 */
        .word   0x4000, 0x0010  /* write VSRAM address 0 */

FMReset:
        /* disable LFO */
        .byte   0,0x22
        .byte   1,0x00
        /* disable timer & set channel 6 to normal mode */
        .byte   0,0x27
        .byte   1,0x00
        /* all KEY_OFF */
        .byte   0,0x28
        .byte   1,0x00
        .byte   1,0x04
        .byte   1,0x01
        .byte   1,0x05
        .byte   1,0x02
        .byte   1,0x06
        /* disable DAC */
        .byte   0,0x2A
        .byte   1,0x80
        .byte   0,0x2B
        .byte   1,0x00
        /* turn off channels */
        .byte   0,0xB4
        .byte   1,0x00
        .byte   0,0xB5
        .byte   1,0x00
        .byte   0,0xB6
        .byte   1,0x00
        .byte   2,0xB4
        .byte   3,0x00
        .byte   2,0xB5
        .byte   3,0x00
        .byte   2,0xB6
        .byte   3,0x00

| PSG register initialization values
PSGReset:
        .byte   0x9f    /* set ch0 attenuation to max */
        .byte   0xbf    /* set ch1 attenuation to max */
        .byte   0xdf    /* set ch2 attenuation to max */
        .byte   0xff    /* set ch3 attenuation to max */

        .align  4

| ASCII font from Steph's Mini DevKit

font_data:
| 20
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0xFFFFFFFF
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0xFFFFFFFF
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x000FF000
        .long 0x00FFFFF0
        .long 0x0FF00000
        .long 0x00FFFF00
        .long 0x00000FF0
        .long 0x0FFFFF00
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF0FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x0FF00FF0
        .long 0x0F000FF0
        .long 0x00000000


        .long 0x000FFF00
        .long 0x00FF0FF0
        .long 0x000FFF00
        .long 0x00FFF000
        .long 0x0FF0FFFF
        .long 0x0FF00FF0
        .long 0x00FFF0FF
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x0000FFF0
        .long 0x000FFF00
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FFF00
        .long 0x0000FFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFF0000
        .long 0x00FFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00FFF000
        .long 0x0FFF0000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0xFFFFFFFF
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x0FFFFFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00FF0000


        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000FF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x0FF00000
        .long 0x0F000000
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF0FFF0
        .long 0x0FFF0FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x00FFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x0000FF00
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0000FF00
        .long 0x000FFF00
        .long 0x00FFFF00
        .long 0x0FF0FF00
        .long 0x0FFFFFF0
        .long 0x0000FF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x0FF00000
        .long 0x0FFFFF00
        .long 0x00000FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x00000FF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x00FF0000
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000FF0
        .long 0x0000FF00
        .long 0x00FFF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00FF0000


        .long 0x00000FF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x000FF000
        .long 0x0000FF00
        .long 0x00000FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x00000000
        .long 0x00000000


        .long 0x0FF00000
        .long 0x00FF0000
        .long 0x000FF000
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x0FF00000
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00000000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF0FFF0
        .long 0x0FF0FFF0
        .long 0x0FF00000
        .long 0x00FFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFFF0
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFF000
        .long 0x0FF0FF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF0FF00
        .long 0x0FFFF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x0FF00000
        .long 0x0FFFFF00
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x0FF00000
        .long 0x0FFFFF00
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFFF0
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF0FFF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFFF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF0FF00
        .long 0x0FFFF000
        .long 0x0FFFF000
        .long 0x0FF0FF00
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF000FF
        .long 0x0FFF0FFF
        .long 0x0FFFFFFF
        .long 0x0FF0F0FF
        .long 0x0FF000FF
        .long 0x0FF000FF
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FFF0FF0
        .long 0x0FFFFFF0
        .long 0x0FFFFFF0
        .long 0x0FF0FFF0
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFF00
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF0FF00
        .long 0x00FF0FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFF00
        .long 0x0FF0FF00
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00000
        .long 0x00FFFF00
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF000FF
        .long 0x0FF000FF
        .long 0x0FF0F0FF
        .long 0x0FFFFFFF
        .long 0x0FFF0FFF
        .long 0x0FF000FF
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x0FF00000
        .long 0x0FFFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x000FFFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0F000000
        .long 0x0FF00000
        .long 0x00FF0000
        .long 0x000FF000
        .long 0x0000FF00
        .long 0x00000FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FFFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x0FFFF000
        .long 0x00000000


        .long 0x00000000
        .long 0x0000F000
        .long 0x000FFF00
        .long 0x00FF0FF0
        .long 0x0FF000FF
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0xFFFFFFFF
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x00FFFF00
        .long 0x0FFFFFF0
        .long 0x0FFFFFF0
        .long 0x00FFFF00
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFF00
        .long 0x00000FF0
        .long 0x00FFFFF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00FFFFF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FFFFFF0
        .long 0x0FF00000
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x0000FFF0
        .long 0x000FF000
        .long 0x00FFFFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFFF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000FF0
        .long 0x0FFFFF00


        .long 0x00000000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x00000000
        .long 0x00FFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00000FF0
        .long 0x00000000
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00000FF0
        .long 0x00FFFF00


        .long 0x00000000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF0FF00
        .long 0x0FFFF000
        .long 0x0FF0FF00
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00FFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FFFFFFF
        .long 0x0FFFFFFF
        .long 0x0FF0F0FF
        .long 0x0FF000FF
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FFFFF00
        .long 0x0FF00000
        .long 0x0FF00000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFFF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000FF0
        .long 0x00000FF0


        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFF00
        .long 0x0FF00FF0
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x0FF00000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x00FFFFF0
        .long 0x0FF00000
        .long 0x00FFFF00
        .long 0x00000FF0
        .long 0x0FFFFF00
        .long 0x00000000


        .long 0x00000000
        .long 0x000FF000
        .long 0x0FFFFFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x0000FFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x000FF000
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FF000FF
        .long 0x0FF0F0FF
        .long 0x0FFFFFFF
        .long 0x00FFFFF0
        .long 0x00FF0FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x00FFFF00
        .long 0x000FF000
        .long 0x00FFFF00
        .long 0x0FF00FF0
        .long 0x00000000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x0FF00FF0
        .long 0x00FFFFF0
        .long 0x0000FF00
        .long 0x0FFFF000


        .long 0x00000000
        .long 0x00000000
        .long 0x0FFFFFF0
        .long 0x0000FF00
        .long 0x000FF000
        .long 0x00FF0000
        .long 0x0FFFFFF0
        .long 0x00000000

| 7B
        .long 0x00000000
        .long 0x0000FFF0
        .long 0x000FF000
        .long 0x000FF000
        .long 0x00FFF000
        .long 0x000FF000
        .long 0x0000FFF0
        .long 0x00000000

| 7C
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 7D
        .long 0x00000000
        .long 0x0FFF0000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FFF00
        .long 0x000FF000
        .long 0x0FFF0000
        .long 0x00000000

| 7E
        .long 0x00000000
        .long 0x00FFF0FF
        .long 0x0FF0FFF0
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000

| 7F
        .long 0x0000F000
        .long 0x000FF000
        .long 0x00FFF000
        .long 0x0FFFF000
        .long 0x00FFF000
        .long 0x000FF000
        .long 0x0000F000
        .long 0x00000000

| 80
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x000FFFFF
        .long 0x000FFFFF
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 81
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FFFFF
        .long 0x000FFFFF
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 82
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000

| 83
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0xFFFFF000
        .long 0xFFFFF000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000

| 84
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0xFFFFF000
        .long 0xFFFFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 85
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0xFFFFF000
        .long 0xFFFFF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 86
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FFFFF
        .long 0x000FFFFF
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000

| 87
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 88
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000

| 89
        .long 0x000FF000
        .long 0x000FF000
        .long 0x000FF000
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000

| 8A
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0x00000000
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
        .long 0xFFFFFFFF
