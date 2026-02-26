for (y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
{
	int bc0 = bc0_start;
	int bc1 = bc1_start;
	int bc2 = bc2_start;

	for (x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
	{
		if ((bc0 | bc1 | bc2) < 0) continue;

		int cb_index = y * cb_stride + x;
		PIXEL_PLOT_FUNC(cb_index, x, y);
	}
}
