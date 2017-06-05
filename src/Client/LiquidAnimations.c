#include "LiquidAnimations.h"
#include "ExtMath.h"
#include "Random.h"

Real32 L_soupHeat[LiquidAnim_Max * LiquidAnim_Max];
Real32 L_potHeat[LiquidAnim_Max * LiquidAnim_Max];
Real32 L_flameHeat[LiquidAnim_Max * LiquidAnim_Max];
Random L_rnd;
bool L_rndInitalised;

void LavaAnimation_Tick(UInt32* ptr, Int32 size) {
	if (!L_rndInitalised) {
		Random_InitFromCurrentTime(&L_rnd);
		L_rndInitalised = true;
	}
	Int32 mask = size - 1;
	Int32 shift = Math_Log2(size);

	Int32 x, y, i = 0;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */
			Int32 xx = x + (Int32)(1.2f * Math_Sin(y * 22.5f * MATH_DEG2RAD));
			Int32 yy = y + (Int32)(1.2f * Math_Sin(x * 22.5f * MATH_DEG2RAD));
			Real32 lSoupHeat =
				L_soupHeat[((yy - 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | (xx & mask)] +
				L_soupHeat[((yy - 1) & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[(yy & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[(yy & mask) << shift | (xx & mask)] +
				L_soupHeat[(yy & mask) << shift | ((xx + 1) & mask)] +

				L_soupHeat[((yy + 1) & mask) << shift | ((xx - 1) & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | (xx & mask)] +
				L_soupHeat[((yy + 1) & mask) << shift | ((xx + 1) & mask)];

			Real32 lPotHeat =
				L_potHeat[i] +                                          /* x    , y     */
				L_potHeat[y << shift | ((x + 1) & mask)] +              /* x + 1, y     */
				L_potHeat[((y + 1) & mask) << shift | x] +              /* x    , y + 1 */
				L_potHeat[((y + 1) & mask) << shift | ((x + 1) & mask)];/* x + 1, y + 1 */

			L_soupHeat[i] = lSoupHeat * 0.1f + lPotHeat * 0.2f;
			L_potHeat[i] += L_flameHeat[i];
			if (L_potHeat[i] < 0.0f) L_potHeat[i] = 0.0f;
			L_flameHeat[i] -= 0.06f * 0.01f;

			if (Random_Float(&L_rnd) <= 0.005f)
				L_flameHeat[i] = 1.5f * 0.01f;

			/* Output the pixel */
			Real32 col = 2.0f * L_soupHeat[i];
			col = col < 0.0f ? 0.0f : col;
			col = col > 1.0f ? 1.0f : col;

			Real32 r = col * 100.0f + 155.0f;
			Real32 g = col * col * 255.0f;
			Real32 b = col * col * col * col * 128.0f;
			*ptr = 0xFF000000UL | ((UInt8)r << 16) | ((UInt8)g << 8) | (UInt8)b;

			ptr++; i++;
		}
	}
}


Real32 W_soupHeat[LiquidAnim_Max * LiquidAnim_Max];
Real32 W_potHeat[LiquidAnim_Max * LiquidAnim_Max];
Real32 W_flameHeat[LiquidAnim_Max * LiquidAnim_Max];
Random W_rnd;
bool W_rndInitalised;

void WaterAnimation_Tick(UInt32* ptr, Int32 size) {
	if (!W_rndInitalised) {
		Random_InitFromCurrentTime(&W_rnd);
		W_rndInitalised = true;
	}
	Int32 mask = size - 1;
	Int32 shift = Math_Log2(size);

	Int32 x, y, i = 0;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			/* Calculate the colour at this coordinate in the heatmap */
			Real32 lSoupHeat =
				W_soupHeat[y << shift | ((x - 1) & mask)] +
				W_soupHeat[y << shift | x] +
				W_soupHeat[y << shift | ((x + 1) & mask)];

			W_soupHeat[i] = lSoupHeat / 3.3f + W_potHeat[i] * 0.8f;
			W_potHeat[i] += W_flameHeat[i] * 0.05f;
			if (W_potHeat[i] < 0.0f) W_potHeat[i] = 0.0f;
			W_flameHeat[i] -= 0.1f;

			if (Random_Float(&W_rnd) <= 0.05f)
				W_flameHeat[i] = 0.5f;

			/* Output the pixel */
			Real32 col = W_soupHeat[i];
			col = col < 0 ? 0 : col;
			col = col > 1 ? 1 : col;
			col = col * col;

			Real32 r = 32.0f + col * 32.0f;
			Real32 g = 50.0f + col * 64.0f;
			Real32 a = 146.0f + col * 50.0f;

			*ptr = ((UInt8)a << 24) | ((UInt8)r << 16) | ((UInt8)g << 8) | 0xFFUL;

			ptr++; i++;
		}
	}
}