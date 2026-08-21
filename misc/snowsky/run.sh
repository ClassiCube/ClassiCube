#!/bin/sh
# Launch ClassiCube on the Snowsky Echo Disc, taking the device over from the stock firmware.
# Run it from an INTERACTIVE ssh session on the device. It is fully reversible: Ctrl-C (or a
# reboot) restores the stock music-player UI.
#
# Getting past the stock firmware needs three things handled at once:
#   1. HARDWARE WATCHDOG - mq_player feeds a 10s watchdog on /dev/jz_watchdog; if we just kill it
#      the SoC hard-resets. So we feed+stop it ourselves via the stock `cmd_watchdog` tool.
#   2. SUPERVISOR - /usr/project/fiio_init.sh relaunches mq_ui/mq_player (and kills wifi + blanks
#      the backlight) whenever `pgrep -x mq_ui`/`mq_player` fails. We keep those checks passing
#      with harmless decoys named mq_ui/mq_player (busybox pgrep -x matches argv[0], so we launch
#      the decoy binary via a PATH lookup of the bare name).
#   3. FRAMEBUFFER/INPUT - mq_ui owns /dev/fb0 and the input devices, so it must be stopped.
#
# Layout expected on the device (adjust CC_DIR if you put it elsewhere):
#   $CC_DIR/ClassiCube      the binary built by misc/snowsky/Makefile
#   $CC_DIR/texpacks/default.zip   the texture pack (http://static.classicube.net/default.zip)
#   $CC_DIR/mq_decoy        the decoy built from misc/snowsky/mq_decoy.c
CC_DIR="${CC_DIR:-/usr/data/classicube}"
DECOY_DIR=/usr/data/ccdecoy
WD=/usr/bin/cmd_watchdog
export LD_LIBRARY_PATH=/usr/lib:/usr/lib/pulseaudio:/usr/data/lib   # for cmd_watchdog (libhardware2.so)

DU=""; DP=""; REAPER=""

cleanup() {
	echo "[cc] restoring stock UI ..."
	[ -n "$REAPER" ] && kill -9 "$REAPER" 2>/dev/null
	[ -n "$DU" ]     && kill -9 "$DU" 2>/dev/null
	[ -n "$DP" ]     && kill -9 "$DP" 2>/dev/null
	rm -rf "$DECOY_DIR" 2>/dev/null
	sleep 1
	/usr/project/fiio_init.sh >/dev/null 2>&1 &   # brings back the UI and re-arms the watchdog feed
}
trap cleanup INT TERM EXIT

[ -s "$CC_DIR/mq_decoy" ] || { echo "[cc] ERROR: $CC_DIR/mq_decoy missing"; exit 1; }

# 1. decoys: run the do-nothing binary as "mq_ui" / "mq_player" (argv[0] via PATH lookup)
mkdir -p "$DECOY_DIR"
cp "$CC_DIR/mq_decoy" "$DECOY_DIR/mq_ui"
cp "$CC_DIR/mq_decoy" "$DECOY_DIR/mq_player"
chmod 755 "$DECOY_DIR/mq_ui" "$DECOY_DIR/mq_player"
PATH="$DECOY_DIR:$PATH" mq_ui &
DU=$!
PATH="$DECOY_DIR:$PATH" mq_player &
DP=$!
sleep 1

# 2+3. reaper: every second, kill the supervisor watchdog + the real mq_ui/mq_player (never the
#      decoys), and keep the hardware watchdog handled.
( while :; do
	for d in /proc/[0-9]*; do
		pid=${d#/proc/}
		[ "$pid" = "$DU" ] && continue
		[ "$pid" = "$DP" ] && continue
		c=$(cat "$d/cmdline" 2>/dev/null | tr '\0' ' ')
		case "$c" in *fiio_init.sh*) kill -9 "$pid" 2>/dev/null; continue ;; esac
		case "$(cat "$d/comm" 2>/dev/null)" in mq_ui|mq_player) kill -9 "$pid" 2>/dev/null ;; esac
	done
	"$WD" feed >/dev/null 2>&1     # keep the hardware watchdog alive this cycle ...
	"$WD" stop >/dev/null 2>&1     # ... and disarm it if the driver allows
	sleep 1
  done ) &
REAPER=$!
sleep 2   # let the reaper clear mq_ui and neutralise the watchdog before we take the screen

# backlight on (the supervisor may have blanked it)
for b in /sys/class/backlight/*/brightness; do
	[ -e "$b" ] || continue
	m=$(cat "$(dirname "$b")/max_brightness" 2>/dev/null || echo 255)
	echo "$m" > "$b" 2>/dev/null
done

cd "$CC_DIR" || { echo "[cc] ERROR: $CC_DIR missing"; exit 1; }
chmod 755 ./ClassiCube 2>/dev/null
echo "[cc] launching ClassiCube (Ctrl-C to quit / restore stock UI)"
./ClassiCube
