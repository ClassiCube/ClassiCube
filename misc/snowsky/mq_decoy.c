/* Placeholder process for run.sh.
 *
 * The stock firmware's supervisor (/usr/project/fiio_init.sh) relaunches the UI whenever
 * `pgrep -x mq_ui` / `pgrep -x mq_player` fails - and that relaunch also kills wifi and blanks
 * the backlight. run.sh keeps those checks passing by running two copies of this do-nothing
 * program named mq_ui / mq_player, so the supervisor stays satisfied and never fights us.
 *
 * It just sleeps forever using no CPU. Build it with the same toolchain as the main binary:
 *   zig cc -target mipsel-linux-musleabi -static -march=mips32r2 -mabi=32 -mnan=2008 \
 *          -o mq_decoy misc/snowsky/mq_decoy.c && python3 misc/snowsky/set_nan2008.py mq_decoy
 */
#include <unistd.h>

int main(void) {
	for (;;) pause();
	return 0;
}
