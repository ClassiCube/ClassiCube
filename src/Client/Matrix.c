#include "Matrix.h"
#include "ExtMath.h"

Vector4 Vector4_Create4(Real32 x, Real32 y, Real32 z, Real32 w) {
	Vector4 v; v.X = x; v.Y = y; v.Z = z; v.W = w; return v;
}

/* Transposed, copied from https://open.gl/transformations */

void Matrix_RotateX(Real32 angle, Matrix* result) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	*result = Matrix_Identity;
	result->Row1.Y = cosA; result->Row1.Z = sinA;
	result->Row2.Y = -sinA; result->Row2.Z = cosA;
}

void Matrix_RotateY(Real32 angle, Matrix* result) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	*result = Matrix_Identity;
	result->Row1.X = cosA; result->Row1.Z = -sinA;
	result->Row2.X = sinA; result->Row2.Z = cosA;
}

void Matrix_RotateZ(Real32 angle, Matrix* result) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	*result = Matrix_Identity;
	result->Row1.X = cosA; result->Row1.Y = sinA;
	result->Row2.X = -sinA; result->Row2.Y = cosA;
}

void Matrix_Translate(Real32 x, Real32 y, Real32 z, Matrix* result) {
	*result = Matrix_Identity;
	result->Row3.X = x;
	result->Row3.Y = y;
	result->Row3.Z = z;
}

void Matrix_Scale(Real32 x, Real32 y, Real32 z, Matrix* result) {
	*result = Matrix_Identity;
	result->Row0.X = x;
	result->Row1.Y = y;
	result->Row2.Z = z;
}


void Matrix_Orthographic(Real32 width, Real32 height, Real32 zNear, Real32 zFar, Matrix* result) {
	Matrix_OrthographicOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNear, zFar, result);
}

void Matrix_OrthographicOffCenter(Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar, Matrix* result) {
	/* Transposed, sourced from https://msdn.microsoft.com/en-us/library/dd373965(v=vs.85).aspx */
	*result = Matrix_Identity;

	result->Row0.X = 2.0f / (right - left);
	result->Row1.Y = 2.0f / (top - bottom);
	result->Row2.Z = -2.0f / (zFar - zNear);

	result->Row3.X = -(right + left) / (right - left);
	result->Row3.Y = -(top + bottom) / (top - bottom);
	result->Row3.Z = -(zFar + zNear) / (zFar - zNear);
}

void Matrix_PerspectiveFieldOfView(Real32 fovy, Real32 aspect, Real32 zNear, Real32 zFar, Matrix* result) {
	Real32 c = zNear * Math_Tan(0.5f * fovy);
	CreatePerspectiveOffCenter(-c * aspect, c * aspect, -c, c, zNear, zFar, result);
}

void Matrix_PerspectiveOffCenter(Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar, Matrix* result) {
	/* Transposed, sourced from https://msdn.microsoft.com/en-us/library/dd373537(v=vs.85).aspx */
	*result = Matrix_Identity;

	result->Row0.X = (2.0f * zNear) / (right - left);
	result->Row1.Y = (2.0f * zNear) / (top - bottom);
	result->Row3.Z = -(2.0f * zFar * zNear) / (zFar - zNear);

	result->Row2.X = (right + left) / (right - left);
	result->Row2.Y = (top + bottom) / (top - bottom);
	result->Row2.Z = -(zFar + zNear) / (zFar - zNear);
	result->Row2.W = -1.0f;
}