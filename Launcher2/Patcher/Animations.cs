// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;

namespace Launcher.Patcher {
	
	public partial class ResourcePatcher {
		
		const string animationsTxt = @"# This file defines the animations used in a texture pack for ClassicalSharp and other supporting applications.
# Each line is in the format: <TileX> <TileY> <FrameX> <FrameY> <Frame size> <Frames count> <Tick delay>
# - TileX and TileY indicate the coordinates of the tile in terrain.png that 
#     will be replaced by the animation frames. These range from 0 to 15. (inclusive of 15)
# - FrameX and FrameY indicates the pixel coordinates of the first animation frame in animations.png.
# - Frame Size indicates the size in pixels of an animation frame.
# - Frames count indicates the number of used frames.  The first frame is located at 
#     (FrameX, FrameY), the second one at (FrameX + FrameSize, FrameY) and so on.
# - Tick delay is the number of ticks a frame doesn't change. For instance, a value of 0
#     means that the frame would be changed every tick, while a value of 2 would mean 
#    'replace with frame 1, don't change frame, don't change frame, replace with frame 2'.
# NOTE: If a file called 'uselavaanim' is in the texture pack, ClassicalSharp 0.99.2 onwards will use its built-in dynamic generation for the lava texture animation.

# still water
14 0 0 0 16 32 2
# still lava
14 1 0 16 16 39 2
# fire
6 2 0 32 16 32 0";
		
		unsafe void PatchDefault(byte[] data, int y) {
			// Sadly files in modern are 24 rgb, so we can't use fastbitmap here
			using (Bitmap bmp = new Bitmap(new MemoryStream(data))) {
				for (int tile = 0; tile < bmp.Height; tile += 16) {
					CopyTile(tile, tile, y, bmp);
				}
			}
		}
		
		unsafe void PatchCycle(byte[] data, int y) {
			using (Bitmap bmp = new Bitmap(new MemoryStream(data))) {
				int dst = 0;
				for (int tile = 0; tile < bmp.Height; tile += 16, dst += 16) {
					CopyTile(tile, dst, y, bmp);
				}
				// Cycle back to first frame.
				for (int tile = bmp.Height - 32; tile >= 0; tile -= 16, dst += 16) {
					CopyTile(tile, dst, y, bmp);
				}
			}
		}
	}
}
