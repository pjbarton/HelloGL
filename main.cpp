//////////////////////////////////
//
// Load and Render PLY file
// P.Barton 5/31/16
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
bool detTriVis[12] = { 1,1,1,1,  1,1,1,1,  1,1,1,1 };

// rotation
float myRotations[192*9];
glm::mat4 transform, transform0;
GLuint transformLoc;
int nside = 16;
int npix = 12 * nside * nside; 
double* theta;
double* phi;
double th, ph;
GLdouble pixVec[3];
glm::vec3 v3Center = glm::vec3(0, 0, 0);
glm::vec3 v3Up = glm::vec3(0, 1, 0);
int ithRotation = 0;
bool bAnimate = true;
bool bHealpix = true;

// mask
char maskHexName[49];
//const char *maskHexNameString;
bool bMask[192], bMask0[192], bMaskRand[192];
bool bNonMask = false;
bool bDOI = false;

// display / control
int winSize = 200;
int xMouse;
int yMouse;
float fBW = 0.0;
int DESIRED_FPS = 60; 
int frameCounter = 0, myTime, startTime = 0, pausedTime;
bool paused = false;
bool bShowBuffers = true;

// hist
float bar_width;
unsigned short* mapRGB;
unsigned short* mapR;
GLubyte* myPixelsR;
GLubyte* myPixelsG;
GLubyte* myPixelsB;
GLubyte* myPixelsRGB;
GLubyte* myPixelsRGB2;
GLubyte* myPixelsRGBA;
bool altPxRGB = 0;
int myCountR[192];
int myCountG[192];
int myCountB[192];
int myCountRGB[3][192];
bool bHist = false; // showHistogram
bool bFileWrite = true;

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

GLuint create_texture(GLenum target, int w, int h)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(target, tex);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D(target, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	return tex;
}

void init(void)
{
	npix = 12 * nside * nside;
	mapRGB = (unsigned short*)malloc(npix * 192 * 3 * sizeof(unsigned short));
	mapR = (unsigned short*)malloc(npix * 192 * sizeof(unsigned short));
	theta = (double*)malloc(nside * sizeof(double));
	phi = (double*)malloc(nside * sizeof(double));

	// Mask
	//srand(0); 
	// 21524fa478bd521ab44791322b545c979943a029753854bb
	/*for (int i = 0; i < 192; i++) {
	bMaskRand[i] = (rand() / (RAND_MAX + 1.)) > 0.5;
	bMask[i] = bMaskRand[i];
	}*/
	char *endptr;
	for (int i = 0; i < 48; i++) {
		char c = maskHexName[i];
		
		long hexDigit = strtol(&c, &endptr, 16);
		//printf("%c - %d\n", c, hexDigit);
		for (unsigned int j = 3; j != -1; --j) {
			bMask[(4 * i + j)] = hexDigit & 1;
			hexDigit /= 2;
		}
	}
	// Window Size
	xMouse = winSize / 2;
	yMouse = winSize / 2;
	myPixelsR = (GLubyte*)malloc(1 * winSize * winSize);
	myPixelsG = (GLubyte*)malloc(1 * winSize * winSize);
	myPixelsB = (GLubyte*)malloc(1 * winSize * winSize);
	myPixelsRGB = (GLubyte*)malloc(3 * winSize * winSize);
	myPixelsRGB2 = (GLubyte*)malloc(3 * winSize * winSize);
	myPixelsRGBA = (GLubyte*)malloc(4 * winSize * winSize);
	bar_width = 2.00 * (2.0 / winSize);



	// Load PRISM 1.0 Model
	ply.Load(fnModel);
	NumVertices = ply.NumFaces * 3;

	// Load Rotations (not used)
	/*std::ifstream inFile("PRISM-1_0_Rotations.txt");
	char temp[20];
	for (int i = 0; i < 192 * 9; i++) {
		inFile.getline(temp, 20);
		myRotations[i] = strtof(temp, NULL);		
	}*/

	// Binary Mask and Mask Sum
	int maskSum = 0;
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
	
	// MaskHex
	printf("MaskHex: %s\n\n", maskHexName);
	//for (int i = 0; i < 192 / 4; i++)
	//{
	//	if (!(i % 8)) printf(" ");
	//	printf("%x", (char) iMask[i]);
	//	//if (!(i % 8)) sprintf(maskHexName + strlen(maskHexName), " "); // add space every 8 digits
	//	sprintf(maskHexName + strlen(maskHexName), "%x", (char)iMask[i]);
	//}
	//printf("\n\n");
	
	//=== Quad ===
	// http://wiki.lwjgl.org/wiki/The_Quad_textured
	// http://stackoverflow.com/questions/21652546/what-is-the-role-of-glbindvertexarrays-vs-glbindbuffer-and-what-is-their-relatio

	//GLfloat quadVert[8] = { 0.0,  0.0,   1.0, 0.0,    0.0, 1.0,   1.0, 1.0 };
	GLfloat quadVert[8] = { -0.5,  -0.5,   0.5,-0.5,   -0.5, 0.5,   0.5, 0.5 };
	GLfloat quadTex[8] = { 0.0,  0.0,   1.0, 0.0,    0.0, 1.0,   1.0, 1.0, };
	GLuint quadIndices[6] = { 0, 1, 2,  	1, 3, 2 };

	glGenVertexArrays(1, &vaoQuad);
	glBindVertexArray(vaoQuad);
	
	glGenBuffers(1, &vboID);
	glBindBuffer(GL_ARRAY_BUFFER, vboID);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), quadVert, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(2);
	
	glGenBuffers(1, &vbotID);
	glBindBuffer(GL_ARRAY_BUFFER, vbotID);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), quadTex, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(3);

	glGenBuffers(1, &vboiID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboiID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), quadIndices, GL_STATIC_DRAW);
	
	// Load Vertex and Color Data
	GLfloat * g_color_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	GLfloat * g_position_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	int myIndex, myIndex2;
	int i = 0;
	for (int iterator = 0; iterator < 192; iterator++) // cubes
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

					if (detTriVis[j])
					{
						if (bDOI) {
							if (j < 2)
								g_color_buffer_data[myIndex + 0] = (191-iterator +1) / 255.0;
							else if (j >= 2 & j < 4)
								g_color_buffer_data[myIndex + 1] = (191-iterator +1) / 255.0;
							else if (j >= 4)
								g_color_buffer_data[myIndex + 2] = (191-iterator +1) / 255.0;
						}
						else {
							g_color_buffer_data[myIndex + 0] = (iterator + 1) / 255.0;  // correct.
						}
					}

					g_position_buffer_data[myIndex + 0] = ply.FaceVertices[myIndex2 + 0] * 0.0125; // X *** fix 0.0125 scale! with transform
					g_position_buffer_data[myIndex + 1] = ply.FaceVertices[myIndex2 + 1] * 0.0125; // Y
					g_position_buffer_data[myIndex + 2] = ply.FaceVertices[myIndex2 + 2] * 0.0125; // Z
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
	ShaderInfo shaders2[] = {
		{ GL_VERTEX_SHADER, "texhist.vs" },
		{ GL_FRAGMENT_SHADER, "texhist.fs" },
		{ GL_NONE, NULL }
	};
	program = LoadShaders(shaders);
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(fBW, fBW, fBW, 1.0);
}

void display(void)
{
	glUseProgram(program);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Rotate 
	if (bHealpix) {
		if (ithRotation > npix - 1) {
			FILE* pFile;
			pFile = fopen(strcat(maskHexName,".bin"), "wb");
			if (bDOI)
				fwrite(mapRGB, sizeof(unsigned short), npix * 192 * 3, pFile);
			else
				fwrite(mapR, sizeof(unsigned short), npix * 192, pFile);
			fclose(pFile);
			//system("pause");
			exit(1);
		}
		//pix2ang_ring(nside, ithRotation, &th, &ph);
		//printf("ith: %d th: %f ph: %f \n", ithRotation, th, ph);
		pix2vec_ring(nside, ithRotation, pixVec);
		//printf("ith: %d x: %f y: %f z: %f \n", ithRotation, pixVec[0], pixVec[1], pixVec[2]);
		transform = glm::lookAt(glm::vec3(pixVec[0] / 80, pixVec[1] / 80, -pixVec[2] / 80), v3Center, v3Up);

		ithRotation++;
	}
	else if (bAnimate) {
		transform = glm::rotate(transform0, myTime * 0.0002f, glm::vec3(1.0, 1.0, 0.0));
		transform = glm::rotate(transform, myTime * 0.0002f, glm::vec3(1.0, 0.0, 1.0));
	}
	else { // by-mouse 
		transform = glm::rotate(transform0, -(xMouse - winSize / 2) * 0.003f, glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, -(yMouse - winSize / 2) * 0.003f, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

	// Draw to Buffer
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	
	// Generate Pixel Histograms	
	// Non-DOI (one channel) Histogram
	if (bDOI) {
		glReadPixels(0, 0, winSize, winSize, GL_RGB, GL_UNSIGNED_BYTE, myPixelsRGB);
		memset(myCountRGB, 0, 3 * 192 * sizeof(int));
		for (int i = 0; i < (winSize * winSize * 3); i += 3)
			for (int j = 0; j < 3; j++)
				if (myPixelsRGB[i + j] > 0)
					myCountRGB[j][myPixelsRGB[i + j] - 1]++;
		// Pack Histograms
		for (int i = 0; i < 192; i++)
			for (int j = 0; j < 3; j++)
				mapRGB[(ithRotation - 1) * 192 * 3 + i * 3 + j] = myCountRGB[j][i];
	}
	else {
		glReadPixels(0, 0, winSize, winSize, GL_RED, GL_UNSIGNED_BYTE, myPixelsR);
		memset(myCountR, 0, 192 * sizeof(int));
		for (int i = 0; i < (winSize * winSize); i++)
				if (myPixelsR[i] > 0)
					myCountR[myPixelsR[i] - 1]++;
		// Pack Histograms for file writing
		if (bHealpix)
			for (int i = 0; i < 192; i++)
				mapR[(ithRotation - 1) * 192 + i] = myCountR[i];
	}

	// Draw Histograms
	if (bHist) {
		glUseProgram(0);
		if (bDOI) {
			for (int j = 0; j < 3; j++) {
				glColor3f(j == 0, j == 1, j == 2);
				for (int i = 0; i < 192; i++) {
					glColor3f((j == 0) * i / 192.0, (j == 1)* i / 192.0, (j == 2)* i / 192.0);
					float bar_top = (myCountRGB[j][i] / (2.0 * winSize)) * 0.25;
					glBegin(GL_QUADS);
						glVertex3f(-1.0 + ((i + j * 192) * bar_width), -1.0, -1.0);
						glVertex3f(-1.0 + ((i + j * 192 + 1) * bar_width), -1.0, -1.0);
						glVertex3f(-1.0 + ((i + j * 192 + 1) * bar_width), -1.0 + bar_top, -1.0);
						glVertex3f(-1.0 + ((i + j * 192) * bar_width), -1.0 + bar_top, -1.0);
					glEnd();
				}
			}
		}
		else {
			for (int j = 0; j < 3; j++) {
				glColor3f(j == 0, j == 1, j == 2);
				for (int i = 0; i < 192; i++) {
					glColor3f(1, 0, 0);
					float bar_top = (myCountR[i] / (2.0 * winSize)) * 0.25;
					glBegin(GL_QUADS);
					glVertex3f(-1.0 + (i * bar_width), -1.0, -1.0);
					glVertex3f(-1.0 + ((i + 1) * bar_width), -1.0, -1.0);
					glVertex3f(-1.0 + ((i + 1) * bar_width), -1.0 + bar_top, -1.0);
					glVertex3f(-1.0 + (i * bar_width), -1.0 + bar_top, -1.0);
					glEnd();
				}
			}
		}
	}

	// Render to Screen
	if (bShowBuffers) 
		glutSwapBuffers();

	// Report
	myTime = glutGet(GLUT_ELAPSED_TIME);
	if ( paused ) myTime = pausedTime;
	if ( myTime - startTime > 1000 )
	{
		if (bHist) {
			printf("\nHistogram:\n");
			for (int j = 0; j < 192; j++) { // for all bins (up to match)
				if (bDOI)
					printf("%5d | ", myCountRGB[0][j]);
				else
					printf("%5d | ", myCountR[j]);
			}
		}
		
		int totalCounts = 0;
		for (int i = 0; i < 192; i++)
			totalCounts += (myCountRGB[0][i] + myCountRGB[1][i] + myCountRGB[2][i]);

		printf("\n FPS: %4.0f, TotalCounts: %d\n", frameCounter * 1000.0 / (myTime - startTime), totalCounts);
		startTime = myTime;
		frameCounter = 0;
	}
	frameCounter++;
}

void key(unsigned char k, int x, int y)
{
	switch (k) {
	case '1':
		detTriVis[0] = !detTriVis[0];
		detTriVis[1] = !detTriVis[1];
		break;
	case '2':
		detTriVis[2] = !detTriVis[2];
		detTriVis[3] = !detTriVis[3];
		break; 
	case '3':
		for (int i = 4; i < 12; i++)
			detTriVis[i] = !detTriVis[i];
		break;
	case 'a':
		bAnimate = !bAnimate;
		break;
	case 'b':
		fBW = (float)!fBW;
		break;
	case 'h':
		bHist = !bHist;
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
		exit(1);
		break;
	}

	init();
}

void MotionCallback(int x, int y)
{
		xMouse = x;
		yMouse = y;
}

void processMenuEvents(int option) 
{
	memset(bMask, 0, 192 * sizeof(bool));
	bMask[option] = 1;
	init();
	bAnimate = false;
}

void createGLUTMenus() {

	int menu = glutCreateMenu(processMenuEvents);

	//add entries to our menu
	glutAddMenuEntry("16", 16); 
	glutAddMenuEntry("32", 32);
	glutAddMenuEntry("64", 64);
	glutAddMenuEntry("96", 96);
	glutAddMenuEntry("126", 126);
	glutAddMenuEntry("127", 127);

	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void idle()
{
	glutPostRedisplay();
}

// Test glCopyTexSubImage2D
// http://opensource.apple.com//source/X11apps/X11apps-44/mesa-demos/mesa-demos-8.0.1/src/tests/subtexrate.c
void displayQuad(void)
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GLUT_MULTISAMPLE);
	
	glBegin(GL_QUADS);            
		glColor3f(1.0, 0.0, 0.0); 
		glVertex2f(-1.0, -0.5);   
		glVertex2f( 0.0, -0.5);
		glVertex2f( 0.0,  0.5);
		glVertex2f(-1.0,  0.5);
	glEnd();

	//glutSwapBuffers();  
	//return;

	glBindTexture(GL_TEXTURE_2D, scene_tex);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, winSize, winSize);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);              
			glTexCoord2f(0.0, 0.0); glVertex2f(0.0, -0.5);    
			glTexCoord2f(1.0, 0.0); glVertex2f(1.0, -0.5);
			glTexCoord2f(1.0, 1.0); glVertex2f(1.0,  0.5);
			glTexCoord2f(0.0, 1.0); glVertex2f(0.0,  0.5);
		glEnd();
	glDisable(GL_TEXTURE_2D);
	
	glutSwapBuffers();  
}

void changeSize(int w, int h)
{

}

void timerCB(int millisec)
{
	glutTimerFunc(millisec, timerCB, millisec); // for next timer event
	glutPostRedisplay();
}

void config(char *fn) {

	config_t cfg;
	const char *str = NULL;
	int iBool;

	config_init(&cfg);
	config_read_file(&cfg, fn);

	config_lookup_int(&cfg, "nside", &nside);

	if (config_lookup_string(&cfg, "mask", &str)) {
		for (int i = 0; i < 48; i++)
			maskHexName[i] = str[i];
		//maskHexName[49] = '\0';
	}

	config_lookup_string(&cfg, "model", &str);
	strcpy(fnModel, str);

	config_lookup_int(&cfg, "winSize", &winSize);

	if (config_lookup_bool(&cfg, "showBuffers", &iBool))
		bShowBuffers = (bool)iBool;
	if (config_lookup_bool(&cfg, "bHealpix", &iBool))
		bHealpix = (bool)iBool; 
	if (config_lookup_bool(&cfg, "bAnimate", &iBool))
		bAnimate = (bool)iBool;
	if (config_lookup_bool(&cfg, "bDOI", &iBool))
		bDOI = (bool)iBool;
	if (config_lookup_bool(&cfg, "bHist", &iBool))
		bHist = (bool)iBool;
	if (config_lookup_bool(&cfg, "bFileWrite", &iBool))
		bFileWrite = (bool)iBool;

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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow( "PRISM 1.0" );
	createGLUTMenus();

	glutDisplayFunc(display);
	glutMotionFunc(MotionCallback);
	glutKeyboardFunc(key);

	glutIdleFunc(idle);	
	//glutTimerFunc(1000.0 / DESIRED_FPS, timerCB, 1000.0 / DESIRED_FPS);

	//glutReshapeFunc(changeSize);
	
	glewExperimental = GL_TRUE;  // !!!
	glewInit();
	
	init();

	glutMainLoop();
}