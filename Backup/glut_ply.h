#pragma once
#ifndef GLUT_PLY_H
#define GLUT_PLY_H

class Model_PLY
{
public:
	int Model_PLY::Load(char *filename);
	float* Model_PLY::calculateNormal(float *coord1, float *coord2, float *coord3);
	Model_PLY();

	float* Faces_Triangles;
	float* Vertex_Buffer;
	float* Normals;

	int TotalConnectedTriangles;
	int TotalConnectedPoints;
	int TotalFaces;
};

#endif
