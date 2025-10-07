// Vertex data pipeline:
// - 12 vertex arrays, which each consist of 12 components
// which are mapped to
// - 12 vertex attributes
// which are mapped to
// - 16 shader inputs
// 
// Note that ClassiCube only uses 1 vertex array at present


/*########################################################################################################################*
*-----------------------------------------------------Index buffer config-------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void pica_set_index_buffer_address(u32 addr) {
	GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, (addr - BUFFER_BASE_PADDR) | (C3D_UNSIGNED_SHORT << 31));
}


/*########################################################################################################################*
*---------------------------------------------------------Fog config------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void pica_set_fog_color(u32 color) {
	GPUCMD_AddWrite(GPUREG_FOG_COLOR, color);
}

static CC_INLINE void pica_set_fog_table(const u32 table[128]) {
	GPUCMD_AddWrite(GPUREG_FOG_LUT_INDEX, 0);
	GPUCMD_AddWrites(GPUREG_FOG_LUT_DATA0, table, 128);
}

static CC_INLINE void pica_update_fog_mode(bool fogEnabled, bool flipZ) {
	GPU_FOGMODE fog = fogEnabled ? GPU_FOG : GPU_NO_FOG;
	GPU_GASMODE gas = GPU_PLAIN_DENSITY;

	u32 buf = fog | (gas << 3) | (flipZ ? (1 << 16) : 0);
	GPUCMD_AddMaskedWrite(GPUREG_TEXENV_UPDATE_BUFFER, 0x5, buf);
}


/*########################################################################################################################*
*----------------------------------------------------Vertex arrays config-------------------------------------------------*
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
*-------------------------------------------------Vertex shader input config----------------------------------------------*
*#########################################################################################################################*/
// https://www.3dbrew.org/wiki/GPU/Internal_Registers#GPUREG_SH_ATTRIBUTES_PERMUTATION_LOW
// Sets how to map vertex attributes to vertex shader input registers
static CC_INLINE void pica_set_vsh_input_mapping(u32 mappingLo, u32 mappingHi) {
	GPUCMD_AddWrite(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW,  mappingLo);
	GPUCMD_AddWrite(GPUREG_VSH_ATTRIBUTES_PERMUTATION_HIGH, mappingHi);
}

#define SH_MODE_VSH 0xA0000000
static CC_INLINE void pica_set_vsh_input_count(u32 count) {
	GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0xB, SH_MODE_VSH | (count - 1));
	GPUCMD_AddWrite(GPUREG_VSH_NUM_ATTR, count - 1);
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
