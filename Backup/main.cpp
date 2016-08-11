//////////////////////////////////
//
// Load and Render PLY file
// P.Barton 5/31/16
//
//////////////////////////////////

#include <iostream>
#include <fstream>
#include <stdio.h>
//#include "wglext.h"
//#include "GL\wglew.h"
#include "GL\glew.h"
#include "GL\freeglut.h"
#include "glm\glm.hpp"
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include "glut_ply.h"
#include "LoadShaders.h"
#include "lodepng.h"
using namespace std;

#define BUFFER_OFFSET(offset) ((void *)(offset))

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0, vColor = 1, vDetector = 2 };
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint NumVertices;
GLuint transformLoc;
glm::mat4 transform, transform0;
int frameCounter = 0, myTime, startTime = 0, pausedTime;
int winSize = 1000;
int xMouse = winSize / 2;
int yMouse = winSize / 2;
GLuint scene_tex;
GLuint m_query;
GLuint m_count[32];
GLubyte* myPixelsR = (GLubyte*) malloc(1 * winSize * winSize);
GLubyte* myPixelsG = (GLubyte*)malloc(1 * winSize * winSize);
GLubyte* myPixelsB = (GLubyte*)malloc(1 * winSize * winSize);
GLubyte* myPixelsRGB = (GLubyte*)malloc(3 * winSize * winSize); 
GLubyte* myPixelsRGBA = (GLubyte*)malloc(4 * winSize * winSize);
int myCountR[192];
int myCountG[192];
int myCountB[192];
int myCountRGB[3][192];
bool paused = false;
Model_PLY ply;
bool detTriVis[12] = { 1,1,1,1, 1,1,1,1, 1,1,1,1 };
bool bAnimate = true;
GLfloat myRotations[192 * 9];
bool bMask[192];
bool bNonMask = true;
float fBW = 0.0;


GLuint create_texture(GLenum target, int w, int h)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(target, tex);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(target, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	return tex;
}

void init(void)
{
	// Load PRISM 1.0 Model
	ply.Load("PRISM-1_0_tri.ply");
	NumVertices = ply.TotalFaces * 3;

	std::ifstream inFile("PRISM-1_0_Rotations.txt");
	char temp[20];
	for (int i = 0; i < 192 * 9; i++)
	{
		inFile.getline(temp, 20);
		myRotations[i] = strtof(temp,NULL);
	}

	// Mask Model
	srand(0);
	int maskSum = 0;
	printf("Mask:");
	for (int i = 0; i < 192; i++)
	{
		bMask[i] = (rand() / (RAND_MAX + 1.)) > 0.5;
		if (bNonMask)
			bMask[i] = true;
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
	printf("MaskHex: ");
	for (int i = 0; i < 192 / 4; i++)
	{
		if (!(i % 4)) printf(" ");
		printf("%X", (char) iMask[i]);
	}
	printf("\n\n");

	// Load Vertex and Color Data
	GLfloat * g_color_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	GLfloat * g_position_buffer_data = (GLfloat*)malloc(NumVertices * 3 * sizeof(float));
	GLfloat * g_detector_buffer_data = (GLfloat*)malloc(NumVertices * 3 * sizeof(float));
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

					g_color_buffer_data[myIndex + 0] = fBW/2; // R
					g_color_buffer_data[myIndex + 1] = fBW/2; // G
					g_color_buffer_data[myIndex + 2] = fBW/2; // B
					g_detector_buffer_data[myIndex + 0] = i;
					g_detector_buffer_data[myIndex + 1] = i;
					g_detector_buffer_data[myIndex + 2] = i;

					if (detTriVis[j])
					{
						if (j < 2)
							g_color_buffer_data[myIndex + 0] = i / 256.0;
						else if (j >= 2 & j < 4)
							g_color_buffer_data[myIndex + 1] = i / 256.0;
						else if (j >= 4)
							g_color_buffer_data[myIndex + 2] = i / 256.0;
					}

					g_position_buffer_data[myIndex + 0] = ply.Faces_Triangles[myIndex2 + 0]; // X
					g_position_buffer_data[myIndex + 1] = ply.Faces_Triangles[myIndex2 + 1]; // Y
					g_position_buffer_data[myIndex + 2] = ply.Faces_Triangles[myIndex2 + 2]; // Z
				}
			}
			i++;
		}
		
		//g_color_buffer_data[i + 0] = 1 - (i * 1.0 / (NumVertices * 3));
		//g_color_buffer_data[i + 1] = 0.0;
		//g_color_buffer_data[i + 2] = 0;
		//printf("%f ",g_color_buffer_data[i]);
	}

	glGenVertexArrays(NumVAOs, VAOs);
	glBindVertexArray(VAOs[Triangles]);

	glGenBuffers(NumBuffers, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[ArrayBuffer]);	
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_position_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vPosition);

	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_color_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vColor);

	GLuint detectorbuffer;
	glGenBuffers(1, &detectorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, detectorbuffer);
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_detector_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vDetector, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vDetector);


	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};
	GLuint program = LoadShaders(shaders);
	glUseProgram(program);
	transformLoc = glGetUniformLocation(program, "transform");
	
	//glEnable(GL_MULTISAMPLE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glRotatef(1.0f * glutGet(GLUT_ELAPSED_TIME), 1.0, 1.0, 0.0);
	
	scene_tex = create_texture(GL_TEXTURE_RECTANGLE_NV, winSize, winSize);
	//glGenQueriesARB(1, &m_query);
}

void draw_quad(int w, int h)
{
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
	glTexCoord2f(w, 0.0); glVertex2f(w, 0.0);
	glTexCoord2f(w, h); glVertex2f(w, h);
	glTexCoord2f(0.0, h); glVertex2f(0.0, h);
	glEnd();
}
void begin_window_coords()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, winSize/2, 0.0, winSize/2, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
void end_window_coords()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
void draw_fullscreen_quad()
{
	begin_window_coords();
	draw_quad(winSize, winSize);
	end_window_coords();
}
void compute_histogram()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// draw scene texture n times, counting number of pixels
	begin_window_coords();
	for (int i = 0; i<32; i++) {
		
		// begin occlusion query
		//histo->BeginCount(i);
		glBeginQueryARB(GL_SAMPLES_PASSED_ARB, m_query);

		// draw_fullscreen_quad();
		//begin_window_coords();
		//draw_quad(winSize, winSize);
		//end_window_coords();

		// end occlusion query and get result
		// histo->EndCount();
		//glEndQueryARB(GL_SAMPLES_PASSED_ARB);
		//glGetQueryObjectuivARB(m_query, GL_QUERY_RESULT_ARB, &m_count[i]);

		printf("%u ", m_count[i]);
	}
	end_window_coords();
}

void display(void)
{
	frameCounter++;

	if (paused)
		myTime = pausedTime; 
	else
		myTime = glutGet(GLUT_ELAPSED_TIME);

	if (myTime - startTime > 1000) 
	{
		printf("\n FPS: %4.0f", frameCounter * 1000.0 / (myTime - startTime));
		startTime = myTime;
		frameCounter = 0;
		//printf("%s \n", myPixels);
		if (false)
		{
			printf("\nHistogram:\n");
			for (int j = 0; j < 192; j++) // for all bins (up to match)
				printf("%5d | ", myCountRGB[0][j]);
		}

		char chBuffer[50];
		sprintf(chBuffer, "myPixelsRGB_%d.png", myTime);
		unsigned char ucRGB = (unsigned char)myCountRGB;
		unsigned error = lodepng_encode32_file(chBuffer, &ucRGB, 192, 3);
		if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
	}
	
	glClearColor(fBW, fBW, fBW, fBW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	if (bAnimate)
	{
		transform = glm::rotate(transform0, myTime * 0.0002f, glm::vec3(1.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, myTime * 0.0002f, glm::vec3(1.0f, 0.0f, 1.0f));
	}
	else
	{
		transform = glm::rotate(transform0, -(xMouse - winSize / 2) * 0.002f, glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, -(yMouse - winSize / 2) * 0.002f, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));


	glDrawArrays(GL_TRIANGLES, 0, NumVertices);

	

	//glBindTexture(GL_TEXTURE_RECTANGLE_NV, scene_tex);
	//glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 0, 0, 0, 0, winSize, winSize);
	//draw_fullscreen_quad(); // test
	//compute_histogram();

	//glReadBuffer(GL_BACK);
	//glReadPixels(0, 0, winSize, winSize, GL_RED, GL_UNSIGNED_BYTE, myPixelsR);
	//glReadPixels(0, 0, winSize, winSize, GL_GREEN, GL_UNSIGNED_BYTE, myPixelsG);
	//glReadPixels(0, 0, winSize, winSize, GL_BLUE, GL_UNSIGNED_BYTE, myPixelsB);
	//glReadPixels(0, 0, winSize, winSize, GL_RGBA, GL_UNSIGNED_BYTE, myPixelsRGBA);
	glReadPixels(0, 0, winSize, winSize, GL_RGB, GL_UNSIGNED_BYTE, myPixelsRGB);
	if (true)
	{
		memset(myCountRGB, 0, 3 * 192 * sizeof(int));

		for (int i = 0; i < (winSize * winSize); i += 3)  // for all pixels
		{
			//if (myPixelsR[i] > 0)
				//myCountR[myPixelsR[i]]++;
			//if (myPixelsG[i] > 0)
				//myCountG[myPixelsG[i]]++;
			//if (myPixelsB[i] > 0)
				//myCountB[myPixelsB[i]]++;
			if (myPixelsRGB[i] > 0)
				myCountRGB[0][myPixelsRGB[i]]++;
			if (myPixelsRGB[i + 1] > 0)
				myCountRGB[1][myPixelsRGB[i + 1]]++;
			if (myPixelsRGB[i + 2] > 0)
				myCountRGB[2][myPixelsRGB[i + 2]]++;
		}
	}
	//for (int i = 0; i < (winSize * winSize); i++)  // for all pixels
	//for (int i = 0; i < (winSize * winSize); i++)  // for all pixels


	//glutSwapBuffers();
	//glutPostRedisplay();
	//glFlush();
}

void displayHistogram(void)
{

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
	case 'm':
		bNonMask = !bNonMask;
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

void idle()
{
	glutPostRedisplay();
}

int main(int argc, char **argv)
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(1000, 0);
	glutInitWindowSize(winSize, winSize);
	//glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow( "PRISM 1.0" );
	
	/*glutInitWindowPosition(1000, winSize);
	glutInitWindowSize(600, 200);
	glutCreateWindow("PRISM 1.0 - Histogram");
	if (!paused)
		glutDisplayFunc(displayHistogram);*/

	glewExperimental = GL_TRUE;  // !!!
	glewInit();
	
	init();

	glutMotionFunc(MotionCallback);
	glutKeyboardFunc(key);
	glutIdleFunc(idle);
	if (!paused)
		glutDisplayFunc(display);
			
	glutMainLoop();
}