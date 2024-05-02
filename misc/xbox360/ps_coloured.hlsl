struct INPUT_VERTEX {
   float4 col : COLOR0;
};

float4 main(INPUT_VERTEX input) : COLOR0 {
	return input.col;
}