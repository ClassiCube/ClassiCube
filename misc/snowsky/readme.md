# ClassiCube on the Snowsky Echo Disc

A port to the **Snowsky Echo Disc** (a FiiO music player): an Ingenic **X2000** SoC (MIPS32r2,
little-endian) running embedded Linux, with a **360x360 round LCD** and a capacitive touchscreen.

It reuses the POSIX platform layer and the software rasteriser (no GPU / OpenGL on this device).
The device-specific part is a single windowing backend, `src/Window_Fbdev.c`, selected by the
`PLAT_SNOWSKY` platform in `src/Core.h`:

- **Display** - takes over the raw Linux framebuffer `/dev/fb0` and converts ClassiCube's 32bpp
  bitmap to the panel's pixel format (read from the driver). The panel is mounted upside-down, so
  output and input are rotated 180 by default (`CC_FB_ROTATE=0` disables it).
- **Input** - standard Linux evdev, matched by device name: the physical buttons (`x2000_key`)
  and the capacitive touchscreen (`cst816`).

## Controls

| Input | Action |
|-------|--------|
| Vol+ | Move forward (`W`) |
| Vol- | Move back (`S`) |
| Play/Pause | Action / place / break (left click) |
| Power (short tap) | Jump (space) |
| Touch drag (in game) | Look around |
| Touch (in menus) | Move + click the pointer |

## Building

Requires [zig](https://ziglang.org) (used as the MIPS cross-compiler). From the repo root:

```sh
make -f misc/snowsky/Makefile               # SoftGPU rasteriser (default)
make -f misc/snowsky/Makefile GFX=SOFTFP    # fixed-point rasteriser (if the FPU is a bottleneck)
```

This produces `ClassiCube-snowsky` and applies the NaN2008 ELF flag (the kernel requires it).
Also build the watchdog decoy used by the launcher:

```sh
zig cc -target mipsel-linux-musleabi -static -march=mips32r2 -mabi=32 -mnan=2008 \
       -o mq_decoy misc/snowsky/mq_decoy.c
python3 misc/snowsky/set_nan2008.py mq_decoy
```

## Assets

The device is offline-only here, so pre-stage the texture pack instead of letting the launcher
download it:

```sh
curl -L http://static.classicube.net/default.zip -o default.zip
```

## Deploying and running

Copy the binary, `default.zip` (into a `texpacks/` subfolder), `mq_decoy`, and `misc/snowsky/run.sh`
onto the device, e.g. under `/usr/data/classicube`:

```
/usr/data/classicube/ClassiCube
/usr/data/classicube/texpacks/default.zip
/usr/data/classicube/mq_decoy
/usr/data/classicube/run.sh
```

Then, from an interactive SSH session on the device:

```sh
sh /usr/data/classicube/run.sh
```

`run.sh` takes the device over from the stock firmware and is fully reversible - **Ctrl-C (or a
reboot) restores the stock music-player UI**. It handles three things the firmware does to keep
its UI running, which otherwise make a takeover impossible:

1. **Hardware watchdog** - `mq_player` feeds a ~10s watchdog on `/dev/jz_watchdog`; killing it
   makes the SoC hard-reset in a few seconds. `run.sh` feeds and stops it via the stock
   `cmd_watchdog` tool.
2. **Supervisor** - `/usr/project/fiio_init.sh` relaunches `mq_ui`/`mq_player` (and kills wifi and
   blanks the backlight) whenever `pgrep -x mq_ui`/`mq_player` fails. `run.sh` keeps those checks
   passing with harmless decoys named `mq_ui`/`mq_player` (see `mq_decoy.c`).
3. **Framebuffer/input owner** - `mq_ui` owns `/dev/fb0` and the input devices, so it is stopped.

## Notes

- `Window_DrawFramebuffer` handles both 16bpp (RGB565) and 32bpp panels; the format is read from
  the driver, so the same binary works regardless of how the panel is configured.
- The evdev button codes in `Window_Fbdev.c` were found by reverse-engineering the stock
  `mq_player`. If a different unit reports different codes, run with `CC_KEY_DEBUG=1` and it logs
  every button's `code`/`down` state.
- Only a *long* hardware hold of Power force-offs the device (handled by the PMU below Linux);
  a short tap is just an evdev event, which is why it can be used to jump.
