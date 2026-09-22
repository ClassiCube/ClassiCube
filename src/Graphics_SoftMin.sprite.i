if ((color & BITMAPCOLOR_A_MASK) == 0) continue;
int cb_index = y * cb_stride + x;

if (vColor != PACKEDCOL_WHITE) {
	int r1 = PackedCol_R(vColor), r2 = BitmapCol_R(color);
	int R  = ( r1 * r2 ) >> 8;
	int g1 = PackedCol_G(vColor), g2 = BitmapCol_G(color);
	int G  = ( g1 * g2 ) >> 8;
	int b1 = PackedCol_B(vColor), b2 = BitmapCol_B(color);
	int B  = ( b1 * b2 ) >> 8;

	colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
} else {
	colorBuffer[cb_index] = color;
}
