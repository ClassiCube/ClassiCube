/*########################################################################################################################*
*--------------------------------------------------Vertex attribute config------------------------------------------------*
*#########################################################################################################################*/

// https://3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_ATTRIBBUFFERi_CONFIG1
// Sets how to map first 8 vertex array components to vertex attributes (or padding)
static CC_INLINE void pica_set_attrib_array0_mapping(u32 mapping) {
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFER0_CONFIG1, mapping);
}

// https://3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_ATTRIBBUFFERi_CONFIG2
// Sets how to map last 4 vertex array components to vertex attributes, bytes per vertex, components count
static CC_INLINE void pica_set_attrib_array0_format(u32 num_attribs, u32 stride) {
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFER0_CONFIG2, (stride << 16) | (num_attribs << 28));
}


/*########################################################################################################################*
*--------------------------------------------------Vertex shader constants------------------------------------------------*
*#########################################################################################################################*/
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
