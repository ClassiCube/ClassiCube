#ifndef CS_GFXENUMS_H
#define CS_GFXENUMS_H

typedef Int32 GfxResourceID;

/* Vertex data format types*/
typedef Int32 VertexFormat;
#define VertexFormat_P3fC4b 0
#define VertexFormat_P3fT2fC4b 1


/* 3D graphics pixel comparison functions */
typedef Int32 CompareFunc;
#define CompareFunc_Always 0
#define CompareFunc_NotEqual 1
#define CompareFunc_Never 2
#define CompareFunc_Less 3
#define CompareFunc_LessEqual 4
#define CompareFunc_Equal 5
#define CompareFunc_GreaterEqual 6
#define CompareFunc_Greater 7


/* 3D graphics pixel blending functions */
typedef Int32 BlendFunc;
#define BlendFunc_Zero 0
#define BlendFunc_One 1
#define BlendFunc_SourceAlpha 2
#define BlendFunc_InvSourceAlpha 3
#define BlendFunc_DestAlpha 4
#define BlendFunc_InvDestAlpha 5


/* 3D graphics pixel fog blending functions */
typedef Int32 Fog;
#define Fog_Linear 0
#define Fog_Exp 1
#define Fog_Exp2 2


/* 3D graphics matrix types */
typedef Int32 MatrixType;
#define MatrixType_Projection 0
#define MatrixType_Modelview 1
#define MatrixType_Texture 2
#endif