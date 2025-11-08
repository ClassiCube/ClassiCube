#include "Core.h"
#include "Funcs.h"
#include "Drawer2D.h"
#include "Utils.h"
#include "LBackend.h"
#include "Window.h"

static void VirtualDialog_Show(const char* title, const char* message) {
	struct Bitmap bmp;
	Platform_LogConst(title);
	Platform_LogConst(message);

	if (!LBackend_FB.bmp.scan0) {
		Window_AllocFramebuffer(&bmp, Window_Main.Width, Window_Main.Height);
	} else {
		bmp = LBackend_FB.bmp;
	}

	struct Context2D ctx;
	Context2D_Wrap(&ctx, &bmp);
	Context2D_Clear(&ctx, BitmapCol_Make(200, 200, 200, 255),
					0, 0, bmp.width, bmp.height);

	for (int i = 0; i < 1000; i++) {
		Rect2D rect = { 0, 0, bmp.width, bmp.height };
		Window_DrawFramebuffer(rect, &bmp);
		Thread_Sleep(50);
	}

	if (!LBackend_FB.bmp.scan0) {
		Window_FreeFramebuffer(&bmp);
	} else {
		LBackend_Redraw();
	}
}
