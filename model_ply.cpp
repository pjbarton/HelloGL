// Load a very simple PLY file (vertices and triangle FaceVertices)
 
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <string>
#include "model_ply.h"
 
Model_PLY::Model_PLY()
{ 

}

void seekLine(FILE *file, char *buffer, char seekString[])
{
	while (strncmp(seekString, buffer, strlen(seekString)) != 0)
	{
		fgets(buffer, 300, file);
	}
}

int Model_PLY::Load(char* filename) 
{
	int i = 0;
	int face_index = 0;
	char buffer[1000];
	int vertex1 = 0, vertex2 = 0, vertex3 = 0;

	FILE* file = fopen(filename,"r");
	fseek(file,0,SEEK_END);
	long fileSize = ftell(file);
	try {
		Vertices = (float*) malloc (ftell(file));
	}
	catch (char* ) {
		return -1;
	}
	if (Vertices == NULL) return -1;
	fseek(file, 0, SEEK_SET); 
 
	FaceVertices = (float*) malloc(fileSize * sizeof(float));
 
	if (file)
	{
		//--- Read Header
		fgets(buffer,300,file);			
		seekLine(file, buffer, "element vertex");
		strcpy(buffer, buffer + strlen("element vertex"));
		sscanf(buffer,"%i", &this->NumVertices);
 
		fseek(file, 0, SEEK_SET);
		seekLine(file, buffer, "element face");
		strcpy(buffer, buffer + strlen("element face"));
		sscanf(buffer,"%i", &this->NumFaces);
 
		seekLine(file, buffer, "end_header");
		//----------------------
 
		// read vertices
		for (int iterator = 0, i = 0; iterator < this->NumVertices; iterator++, i += 3)
		{
			fgets(buffer, 300, file);
			sscanf(buffer,"%f %f %f", &Vertices[i], &Vertices[i+1], &Vertices[i+2]);
		}
 
		// read FaceVertices
		for (int iterator = 0; iterator < this->NumFaces; iterator++)
		{
			fgets(buffer, 300, file);
			buffer[0] = ' ';
			sscanf(buffer,"%i%i%i", &vertex1, &vertex2, &vertex3 );
 
			FaceVertices[face_index]   = Vertices[3*vertex1];
			FaceVertices[face_index+1] = Vertices[3*vertex1+1];
			FaceVertices[face_index+2] = Vertices[3*vertex1+2];
			FaceVertices[face_index+3] = Vertices[3*vertex2];
			FaceVertices[face_index+4] = Vertices[3*vertex2+1];
			FaceVertices[face_index+5] = Vertices[3*vertex2+2];
			FaceVertices[face_index+6] = Vertices[3*vertex3];
			FaceVertices[face_index+7] = Vertices[3*vertex3+1];
			FaceVertices[face_index+8] = Vertices[3*vertex3+2];
 
			face_index += 9;
		}
		fclose(file);
	}
	else 
	{ 
		printf("%s can't be opened\n", filename);
	}
  
	return 0;
}
 