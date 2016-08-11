class Model_PLY
{
public:
	int Model_PLY::Load(char *filename);
	
	Model_PLY();

	float* Vertices;
	float* FaceVertices;

	int NumVertices;
	int NumFaces;
};

