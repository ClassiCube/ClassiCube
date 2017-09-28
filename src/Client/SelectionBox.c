#include "SelectionBox.h"

void SelectionBox_Make(SelectionBox* box, Vector3I* p1, Vector3I* p2, PackedCol col) {
	box->ID = 0;
	box->MinDist = 0.0f; box->MaxDist = 0.0f;
	Vector3I_Min(&box->Min, p1, p2);
	Vector3I_Max(&box->Max, p1, p2);
	box->Col = col;
}

void SelectionBox_Render(SelectionBox* box, VertexP3fC4b** vertices, VertexP3fC4b** lineVertices) {
	Real32 offset = box->MinDist < 32.0f * 32.0f ? (1.0f / 32.0f) : (1.0f / 16.0f);
	Vector3 p1, p2;
	PackedCol col = box->Col;

	Vector3I_ToVector3(&p1, &box->Min);
	Vector3I_ToVector3(&p2, &box->Max);
	Vector3_Add1(&p1, &p1, -offset);
	Vector3_Add1(&p2, &p2,  offset);

	SelectionBox_HorQuad(vertices, col, p1.X, p1.Z, p2.X, p2.Z, p1.Y);       /* bottom */
	SelectionBox_HorQuad(vertices, col, p1.X, p1.Z, p2.X, p2.Z, p2.Y);       /* top */
	SelectionBox_VerQuad(vertices, col, p1.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z); /* sides */
	SelectionBox_VerQuad(vertices, col, p1.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z);
	SelectionBox_VerQuad(vertices, col, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p2.Z);
	SelectionBox_VerQuad(vertices, col, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p2.Z);

	col.R = (UInt8)~col.R; col.G = (UInt8)~col.G; col.B = (UInt8)~col.B;
	/* bottom face */
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p1.Z, p2.X, p1.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p1.Z, p2.X, p1.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p2.Z, p1.X, p1.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p2.Z, p1.X, p1.Y, p1.Z);
	/* top face */
	SelectionBox_Line(lineVertices, col, p1.X, p2.Y, p1.Z, p2.X, p2.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p2.Y, p1.Z, p2.X, p2.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p2.Y, p2.Z, p1.X, p2.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p1.X, p2.Y, p2.Z, p1.X, p2.Y, p1.Z);
	/* side faces */
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p1.Z, p1.X, p2.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p1.Z, p2.X, p2.Y, p1.Z);
	SelectionBox_Line(lineVertices, col, p2.X, p1.Y, p2.Z, p2.X, p2.Y, p2.Z);
	SelectionBox_Line(lineVertices, col, p1.X, p1.Y, p2.Z, p1.X, p2.Y, p2.Z);
}

void SelectionBox_VerQuad(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	VertexP3fC4b* ptr = *vertices;
	VertexP3fC4b_Set(ptr, x1, y1, z1, col); ptr++;
	VertexP3fC4b_Set(ptr, x1, y2, z1, col); ptr++;
	VertexP3fC4b_Set(ptr, x2, y2, z2, col); ptr++;
	VertexP3fC4b_Set(ptr, x2, y1, z2, col); ptr++;
	*vertices = ptr;
}

void SelectionBox_HorQuad(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 z1, Real32 x2, Real32 z2, Real32 y) {
	VertexP3fC4b* ptr = *vertices;
	VertexP3fC4b_Set(ptr, x1, y, z1, col); ptr++;
	VertexP3fC4b_Set(ptr, x1, y, z2, col); ptr++;
	VertexP3fC4b_Set(ptr, x2, y, z2, col); ptr++;
	VertexP3fC4b_Set(ptr, x2, y, z1, col); ptr++;
	*vertices = ptr;
}

void SelectionBox_Line(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	VertexP3fC4b* ptr = *vertices;
	VertexP3fC4b_Set(ptr, x1, y1, z1, col); ptr++;
	VertexP3fC4b_Set(ptr, x2, y2, z2, col); ptr++;
	*vertices = ptr;
}