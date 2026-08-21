#!/usr/bin/env python3
# Sets the NaN2008 flag (EF_MIPS_NAN2008, 0x400) in a MIPS ELF's e_flags.
# zig cc omits it even with -mnan=2008, but the Snowsky's kernel requires it.
import sys
from pathlib import Path

p = Path(sys.argv[1])
d = bytearray(p.read_bytes())
if d[:4] != b"\x7fELF" or d[4] != 1 or d[5] != 1:
    raise SystemExit("not a 32-bit little-endian ELF")
old = int.from_bytes(d[0x24:0x28], "little")
new = old | 0x400
d[0x24:0x28] = new.to_bytes(4, "little")
p.write_bytes(d)
print(f"{p.name}: e_flags 0x{old:08x} -> 0x{new:08x}")
