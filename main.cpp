//////////////////////////////////
//
// OpenGL calculation of spherical coded aperture system response
// P.Barton 10/13/16
//
//////////////////////////////////

#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include "GL\glew.h"
#include "GL\freeglut.h"
#include "glm\glm.hpp"
#include "glm\common.hpp"
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include "model_ply.h"
#include "LoadShaders.h"
#include "chealpix.h"
#include "../packages/libconfig-1.5/lib/libconfig.h" // compiled in VS2015 after altering line 697 of libconfig.c
using namespace std;

#define BUFFER_OFFSET(offset) ((void *)(offset))
#define printOpenGLError() printOglError(__FILE__, __LINE__)

// 
GLuint glutWinID;
GLuint fbo;
enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0, vColor = 1, vDetector = 2 };
GLuint vaoQuad, vaoDetectors;
GLuint vboID, vbotID, vboiID;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint colorbuffer;
GLuint NumVertices;
GLuint detIDLoc, detMinLoc, detMaxLoc;
GLuint scene_tex;
Model_PLY ply;
GLuint program, program2;
char fnModel[60];
bool detTriVisAll[12] = { 1,1,1,1,  1,1,1,1,  1,1,1,1 };
bool detTriVisInnerOnly[12] = { 1,1,0,0,  0,0,0,0,  0,0,0,0 };
bool detTriVisOuterOnly[12] = { 0,0,1,1,  0,0,0,0,  0,0,0,0 };
bool* detTriVis;// = detTriVisAll;

// rotation
float myRotations[192*9];
glm::mat4 transform, transform0, perspective;
double srcDist = 1e6;
double detMaxRadius = 80;
GLuint transformLoc;
int nside = 16;
int npix = 12 * nside * nside; 
GLdouble pixVec[3];
glm::vec3 v3Center = glm::vec3(0, 0, 0);
glm::vec3 v3Up = glm::vec3(0, 1, 0);
int ithRotation = 0;
bool bAnimate = true;
bool bHealpix = true;

// mask
char maskHexName[49];
int maskSum = 0;
bool bMask[192], bMask0[192], bMaskRand[192];
bool bNonMask = false;
bool bInnerOnly = false;

// display / control
int winSize = 200;
int xMouse;
int yMouse;
float fBW = 0.0;
int frameCounter = 0, myTime, startTime = 0, pausedTime;
bool paused = false;
bool bShowBuffers = true;

// hist
FILE* pFile; 
unsigned short* mapHist;
GLubyte* myPixelsRGB;
int* myCounts;
int nDepthBins = 1;
int iDepth = 0;
int bs; // bitshift for 8-bit depth value

int printOglError(char *file, int line)
{
	GLenum glErr;
	int    retCode = 0;

	glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		printf("glError in file %s @ line %d: %s\n",
			file, line, gluErrorString(glErr));
		retCode = 1;
	}
	return retCode;
}

void init(void)
{
	npix = 12 * nside * nside;
	myCounts = (int*)malloc(nDepthBins * 192 * sizeof(int));
	mapHist = (unsigned short*)malloc(npix * 192 * nDepthBins * sizeof(unsigned short));
	bs = 8 - (int)log2(nDepthBins);
	xMouse = winSize / 2;
	yMouse = winSize / 2;
	myPixelsRGB = (GLubyte*)malloc(3 * winSize * winSize);

	// Binary Mask and Sum
	char *endptr;
	for (int i = 0; i < 48; i++) {
		char c = maskHexName[i];
		long hexDigit = strtol(&c, &endptr, 16);
		for (unsigned int j = 3; j != -1; --j) {
			bMask[(4 * i + j)] = hexDigit & 1;
			hexDigit /= 2;
		}
	}
	printf("Mask:");
	for (int i = 0; i < 192; i++)
	{		
		maskSum += bMask[i];
		if (!(i % 16)) printf("\n");
		printf("%d", bMask[i]);
	}
	printf("\n\n");
	int iMask[192/4];
	for (int i = 0; i < 192/4; i++)
	{
		iMask[i] = 0;
		for (int j = 0; j < 4; j++)
			iMask[i] += bMask[i * 4 + j] << (3-j);
	}
	NumVertices = maskSum * 12 * 3; // detector * triangles * vertices
	printf("MaskSum: %d \n\n", maskSum);
	printf("MaskHex: %s \n\n", maskHexName);

	// Load Vertex and Color Data
	ply.Load(fnModel);  // Load PRISM PLY Model
	GLfloat * g_color_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	GLfloat * g_position_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	int myIndex, myIndex2;
	int i = 0; // detectors
	for (int iterator = 0; iterator < 192; iterator++) // possible detectors
	{
		if (bMask[iterator])
		{
			for (int j = 0; j < 12; j++) // triangles
			{
				for (int k = 0; k < 3; k++) // vertices
				{
					myIndex = (i * 12 * 3 * 3) + (j * 3 * 3) + (k * 3);
					myIndex2 = (iterator * 12 * 3 * 3) + (j * 3 * 3) + (k * 3);

					g_color_buffer_data[myIndex + 0] = fBW; // R
					g_color_buffer_data[myIndex + 1] = fBW; // G
					g_color_buffer_data[myIndex + 2] = fBW; // B

					if (*(detTriVis + j))
					{
						// Red: detector index
						g_color_buffer_data[myIndex + 0] = (191 - iterator + 1) / 255.0; 

						// Blue: depth (inner = 1, outer = 0)
						if (nDepthBins > 1) { 
							// Inner (depth = 1)
							if (j < 2) 
								g_color_buffer_data[myIndex + 2] = 1.0; 
							// Outer (depth = 0)
							else if (j >= 2 && j < 4) 
								g_color_buffer_data[myIndex + 2] = 0; 
							else if (j >= 4) {
								g_color_buffer_data[myIndex + 2] = 1.0; 
								if (k == 2)
									g_color_buffer_data[myIndex + 2] = 0; 
								if (j % 2 != 0 && k == 1)
									g_color_buffer_data[myIndex + 2] = 0; 
							}
						}
					}
					g_position_buffer_data[myIndex + 0] = ply.FaceVertices[myIndex2 + 0]; // X 
					g_position_buffer_data[myIndex + 1] = ply.FaceVertices[myIndex2 + 1]; // Y
					g_position_buffer_data[myIndex + 2] = ply.FaceVertices[myIndex2 + 2]; // Z
				}
			}
			i++;
		}
	}

	glGenVertexArrays(NumVAOs, VAOs); // fix enum...
	glBindVertexArray(VAOs[Triangles]);

	glGenBuffers(NumBuffers, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[ArrayBuffer]);	
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_position_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vPosition);
	
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_color_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vColor);

	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};
	program = LoadShaders(shaders);
	
	perspective = glm::perspective(2 * asin(detMaxRadius / srcDist), 1.0, srcDist - detMaxRadius, srcDist + detMaxRadius);

	glEnable(GL_DEPTH_TEST);
	glClearColor(fBW, fBW, fBW, 1.0);
}

void display(void)
{
	glUseProgram(program);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Rotate 
	// projection * view * (translate * rotate * scale * original)
	transform = glm::translate(transform0, glm::vec3(0.0, 0.0, -srcDist));
	if (bHealpix) {
		if (ithRotation >= npix) {
			
			pFile = fopen(strcat(maskHexName,".bin"), "wb");
			fwrite(mapHist, sizeof(unsigned short), npix * 192 * nDepthBins, pFile);
			fclose(pFile);
			exit(0);
		}
		pix2vec_ring(nside, ithRotation, pixVec);
		transform = transform * glm::lookAt(glm::vec3(pixVec[0], pixVec[1], -pixVec[2]), v3Center, v3Up);
		ithRotation++;
	}
	else if (bAnimate) {
		transform = glm::rotate(transform, myTime * 0.0002f, glm::vec3(1.0, 1.0, 0.0));
		transform = glm::rotate(transform, myTime * 0.0002f, glm::vec3(1.0, 0.0, 1.0));
	}
	else { // by-mouse 
		transform = glm::rotate(transform0, -(xMouse - winSize / 2) * 0.003f, glm::vec3(0.0, 1.0, 0.0));
		transform = glm::rotate(transform, -(yMouse - winSize / 2) * 0.003f, glm::vec3(1.0, 0.0, 0.0));
	}
	transform = perspective * transform;
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

	glDrawArrays(GL_TRIANGLES, 0, NumVertices);  // Draw to Buffer
	
	// Generate Pixel Histograms	
	glReadPixels(0, 0, winSize, winSize, GL_RGB, GL_UNSIGNED_BYTE, myPixelsRGB); // store image in myPixelsRGB
	memset(myCounts, 0, sizeof(int) * nDepthBins * 192);
	for (int ithPx = 0; ithPx < (winSize * winSize * 3); ithPx += 3) // for all RGB pixels in image
	{
		if (myPixelsRGB[ithPx + 0] > 0) { // if valid detector index
			iDepth = myPixelsRGB[ithPx + 2] >> bs; // blue [0,255] divided by 2^7 -> [0, 1] (2-bins)
			myCounts[(iDepth * 192 + (myPixelsRGB[ithPx + 0] - 1))]++; // *** implicit cast OK?
		}
	}
	for (int ithDet = 0; ithDet < 192; ithDet++)
		for (int jthDepth = 0; jthDepth < nDepthBins; jthDepth++)
			mapHist[(ithRotation - 1) * 192 * nDepthBins + ithDet * nDepthBins + jthDepth] = myCounts[jthDepth * 192 + ithDet];

	if (bShowBuffers) 
		glutSwapBuffers();  // Render to Screen

	// Report
	myTime = glutGet(GLUT_ELAPSED_TIME);
	if ( myTime - startTime > 1000 )
	{
		printf("\n FPS: %4.0f\n", frameCounter * 1000.0 / (myTime - startTime));
		startTime = myTime;
		frameCounter = 0;
	}
	frameCounter++;
}

void key(unsigned char k, int x, int y)
{
	switch (k) {
	case '1':
		detTriVisAll[0] = !detTriVisAll[0];
		detTriVisAll[1] = !detTriVisAll[1];
		break;
	case '2':
		detTriVisAll[2] = !detTriVisAll[2];
		detTriVisAll[3] = !detTriVisAll[3];
		break; 
	case '3':
		for (int i = 4; i < 12; i++)
			detTriVisAll[i] = !detTriVisAll[i];
		break;
	case 'a':
		bAnimate = !bAnimate;
		break;
	case 'b':
		fBW = (float)!fBW;
		break;
	case 'm':
		bNonMask = !bNonMask;
		if (!bNonMask) {
			for (int i = 0; i < 192; i++)
				bMask[i] = bMaskRand[i];
		}
		else {
			for (int i = 0; i < 192; i++)
				bMask[i] = 1;
		}

		break;
	case 'p':
		paused = !paused;
		pausedTime = myTime;
		break;
	case 'q':
		exit(0);
		break;
	}

	init();
}

void MotionCallback(int x, int y)
{
		xMouse = x;
		yMouse = y;
}

void idle()
{
	glutPostRedisplay();
}

void config(char *fn) {

	config_t cfg;
	const char *str = NULL;
	double dTemp;
	int iBool, iTemp;

	config_init(&cfg);
	config_read_file(&cfg, fn);

	config_lookup_int(&cfg, "nside", &nside);

	if (config_lookup_string(&cfg, "mask", &str)) {
		for (int i = 0; i < 48; i++)
			maskHexName[i] = str[i];
	}

	config_lookup_string(&cfg, "model", &str);
	strcpy(fnModel, str);

	config_lookup_int(&cfg, "winSize", &winSize);

	config_lookup_int(&cfg, "nDepthBins", &nDepthBins);

	if (config_lookup_bool(&cfg, "bShowBuffers", &iBool))
		bShowBuffers = (bool)iBool;
	if (config_lookup_bool(&cfg, "bHealpix", &iBool))
		bHealpix = (bool)iBool; 
	if (config_lookup_bool(&cfg, "bAnimate", &iBool))
		bAnimate = (bool)iBool;
	if (config_lookup_bool(&cfg, "bInnerOnly", &iBool)) {
		if ((bool)iBool)
			detTriVis = detTriVisInnerOnly;	
		else
			detTriVis = detTriVisAll;
	}
	if (config_lookup_float(&cfg, "srcDist", &dTemp))
		srcDist = dTemp;

	config_destroy(&cfg);
}

int main(int argc, char **argv)
{
	// Config File
	if (argc > 1)
		config(argv[1]);
	else {
		printf("Please provide a valid config file.\n\n");
		system("pause");
		exit(-1);
	}
	
	// GLUT-GLEW	
	glutInit(&argc, argv);
	glutInitWindowPosition(500, 0);
	glutInitWindowSize(winSize, winSize);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH ); 
	glutWinID = glutCreateWindow( "PRISM 1.0" );
	glutDisplayFunc(display);
	glutMotionFunc(MotionCallback);
	glutKeyboardFunc(key);
	glutIdleFunc(idle);	
	
	glewExperimental = GL_TRUE;  // !!!
	glewInit();
	
	init();

	glutMainLoop();
}