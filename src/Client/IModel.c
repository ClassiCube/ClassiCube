#include "IModel.h"

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, UInt16 u, UInt16 v) {
	vertex->X = x; vertex->Y = y; vertex->Z = z;
	vertex->U = u; vertex->V = v;
}

void IModel_Init(IModel* model) {
	model->Bobbing = true;
	model->UsesSkin = true;
	model->CalcHumanAnims = false;
	model->Gravity = 0.08f;
	model->Drag = Vector3_Create3(0.91f, 0.98f, 0.91f);
	model->GroundFriction = Vector3_Create3(0.6f, 1.0f, 0.6f);
	model->MaxScale = 2.0f;
	model->ShadowScale = 1.0f;
	model->NameScale = 1.0f;
}