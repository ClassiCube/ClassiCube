// https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_SH_FLOATUNIFORM_INDEX
// https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_SH_FLOATUNIFORM_DATAi
#define UPLOAD_TYPE_F32 0x80000000

static CC_INLINE void pica_upload_mat4_constant(int offset, C3D_Mtx* value) {
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, offset | UPLOAD_TYPE_F32);
	GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA, (u32*)value, 16);
}

static CC_INLINE void pica_upload_vec4_constant(int offset, C3D_FVec* value) {
	GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, offset | UPLOAD_TYPE_F32);
	GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA, (u32*)value, 4);
}
