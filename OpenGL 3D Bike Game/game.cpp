// Name: Gaurav Mishra(201202057)
// Game: 'Fossil Park Ride'

#include <stdlib.h> 
#include <math.h> 
#include <string.h> 
#include <stdio.h> 
#include <GL/glut.h>
#include <vector>
#include <ctime>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "imageloader.h"
#include "vec3f.h"
#define PI 3.141592653589
#define DEG2RAD(deg) ((deg) * PI / 180)
#define RAD2DEG(rad) ((rad) * 180 / PI)
#define NUM_BUFFERS 3
#define NUM_SOURCES 3
#define ESC 27 
using namespace std;


GLuint _skybox[5];
GLuint _nightbox[5];
// Camera position
float x = 0.0, y = -5.0; // initially 5 units south of origin
float translate = 0.0; // initially camera doesn't move
float rotate = 0.0; // initially camera doesn't rotate left/right
float tilt=0.0;

// Camera direction
float lx = 0.0, ly = 1.0; // camera points initially along y-axis
float angle = 0.0; // angle of rotation for the camera direction
float deltaAngle = 0.0; // additional angle change when dragging
float bikeAngle = 0.0;
float bikeX=0.0,bikeY=0.0,bikeZ=0.0;
float h,h1,h2,h3,h4,h5,h6,h7,h8,avg;	//for height avg to translate up
float torRotate=0.0,prevheight=-1,prevpitch=0,ha,hb;
float pitch=0.0,vel=0.0,acc=0.002,decel=0.001;
int timeleft=120;
int night=0;
int level=0;
time_t pt=time(0);
int scorearr[2]={3,3};
// Mouse drag control
int isDragging = 0; // true when dragging
int xDragStart = 0; // records the x-coordinate when dragging starts
int camselect=0,flight=0,score=0,tilted=0;
int X,Z;
typedef struct fossil
{	float fossil_x,fossil_y,fossil_z;
}fossil;
vector < fossil > fossils;

ALuint Buffers[NUM_BUFFERS];	// Buffers hold sound data.
ALuint Sources[NUM_SOURCES];	// Sources are points of emitting sound.
ALfloat SourcesPos[NUM_SOURCES][3];	// Position of the source sounds.
ALfloat SourcesVel[NUM_SOURCES][3];	// Velocity of the source sounds.
ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };	// Position of the listener.
ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };	// Velocity of the listener.
ALfloat ListenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 }; // Orientation of the listener. (first 3 elements are "at", second 3 are "up")

ALboolean LoadALData()
{	// Variables to load into.
	ALenum format;
	ALsizei size;
	ALvoid* data;
	ALsizei freq;
	ALboolean loop;
	// Load wav data into buffers.
	alGenBuffers(NUM_BUFFERS, Buffers);

	if(alGetError() != AL_NO_ERROR)	// Do error check
		return AL_FALSE;

	alutLoadWAVFile((ALbyte*)"audio/bike.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[1], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);

	alutLoadWAVFile((ALbyte*)"audio/coin.wav", &format, &data, &size, &freq, &loop);
	alBufferData(Buffers[2], format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);

	// Bind buffers into audio sources.
	alGenSources(NUM_SOURCES, Sources);
	if(alGetError() != AL_NO_ERROR)	// Do error check
		return AL_FALSE;

	alSourcei (Sources[1], AL_BUFFER,   Buffers[1]   );
	alSourcef (Sources[1], AL_PITCH,    1.0f         );
	alSourcef (Sources[1], AL_GAIN,     1.0f         );
	alSourcefv(Sources[1], AL_POSITION, SourcesPos[1]);
	alSourcefv(Sources[1], AL_VELOCITY, SourcesVel[1]);
	alSourcei (Sources[1], AL_LOOPING,  AL_FALSE     );

	alSourcei (Sources[2], AL_BUFFER,   Buffers[2]   );
	alSourcef (Sources[2], AL_PITCH,    1.0f         );
	alSourcef (Sources[2], AL_GAIN,     1.0f         );
	alSourcefv(Sources[2], AL_POSITION, SourcesPos[2]);
	alSourcefv(Sources[2], AL_VELOCITY, SourcesVel[2]);
	alSourcei (Sources[2], AL_LOOPING,  AL_FALSE     );

	if(alGetError() != AL_NO_ERROR)	// Do another error check and return.
		return AL_FALSE;

	return AL_TRUE;
}

void SetListenerValues()	//We already defined certain values for the listener, but we need
{	alListenerfv(AL_POSITION,    ListenerPos);	//to tell OpenAL to use that data. This function does just that.
	alListenerfv(AL_VELOCITY,    ListenerVel);
	alListenerfv(AL_ORIENTATION, ListenerOri);
}

void KillALData()	//We have allocated memory for our buffers and sources which needs 
{	alDeleteBuffers(NUM_BUFFERS, Buffers);	//to be returned to the system. This function frees that memory.
	alDeleteSources(NUM_SOURCES, Sources);
	alutExit();
}


//Makes the image into a texture, and returns the id of the texture
GLuint loadTexture(Image* image) 
{	GLuint textureId;
	glGenTextures(1, &textureId); //Make room for our texture
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
			0,                            //0 for now
			GL_RGB,                       //Format OpenGL uses for image
			image->width, image->height,  //Width and height
			0,                            //The border of the image
			GL_RGB, //GL_RGB, because pixels are stored in RGB format
			GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
			//as unsigned numbers
			image->pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}

GLuint _textureId,_textureId1,_textureId2,_textureId3,_textureId4; //The id of the texture


//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
	private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) 
		{	w = w2;
			l = l2;

			hs = new float*[l];
			for(int i = 0; i < l; i++) 
				hs[i] = new float[w];

			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) 
				normals[i] = new Vec3f[w];

			computedNormals = false;
		}

		~Terrain() 
		{	for(int i = 0; i < l; i++) 
			delete[] hs[i];
			delete[] hs;

			for(int i = 0; i < l; i++) 
				delete[] normals[i];
			delete[] normals;
		}

		int width() 
		{	return w;	}

		int length() 
		{	return l;	}

		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) 
		{	hs[z][x] = y;
			computedNormals = false;
		}

		//Returns the height at (x, z)
		float getHeight(int x, int z) 
		{	return hs[z][x];	}

		//Computes the normals, if they haven't been computed yet
		void computeNormals() 
		{	if (computedNormals) 
			return;

			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) 
				normals2[i] = new Vec3f[w];

			for(int z = 0; z < l; z++) 
			{	for(int x = 0; x < w; x++) 
				{	Vec3f sum(0.0f, 0.0f, 0.0f);	
					Vec3f out;
					if (z > 0) 
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					Vec3f in;
					if (z < l - 1) 
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					Vec3f left;
					if (x > 0) 
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					Vec3f right;
					if (x < w - 1) 
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);

					if (x > 0 && z > 0) 
						sum += out.cross(left).normalize();					
					if (x > 0 && z < l - 1) 
						sum += left.cross(in).normalize();
					if (x < w - 1 && z < l - 1) 
						sum += in.cross(right).normalize();
					if (x < w - 1 && z > 0) 
						sum += right.cross(out).normalize();

					normals2[z][x] = sum;
				}
			}

			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) 
			{	for(int x = 0; x < w; x++) 
				{	Vec3f sum = normals2[z][x];
					if (x > 0) 
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					if (x < w - 1) 
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					if (z > 0) 
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					if (z < l - 1)
						sum += normals2[z + 1][x] * FALLOUT_RATIO;

					if (sum.magnitude() == 0) 
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					normals[z][x] = sum;
				}
			}

			for(int i = 0; i < l; i++) 
				delete[] normals2[i];
			delete[] normals2;

			computedNormals = true;
		}

		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) 
		{	if (!computedNormals) 
			computeNormals();
			return normals[z][x];
		}
};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) 
{	Image* image = loadBMP(filename);
	int span;
	if(level==0)
		span=3;
	else
		span=1;
	printf("level=%d span=%d\n",level,span);
	Terrain* t = new Terrain(image->width*span, image->height*span);
	for(int y = 0; y < image->height*span; y++) 
	{	for(int x = 0; x < image->width*span; x++) 
		{	unsigned char color = (unsigned char)image->pixels[3 * ((y%image->height) * image->width + (x%image->width))];
			float h = height * ((color / 255.0f) - 0.5f);
			if(h<0 && level==0)
				t->setHeight(x, y, 0);
			else
				t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float heightAt(Terrain* terrain, float x, float z) 
{	//Make (x, z) lie within the bounds of the terrain
	if (x < 0)
		x = 0;
	else if (x > terrain->width() - 1) 
		x = terrain->width() - 1;
	if (z < 0) 
		z = 0;
	else if (z > terrain->length() - 1) 
		z = terrain->length() - 1;

	//Compute the grid cell in which (x, z) lies and how close we are to the left and outward edges
	int leftX = (int)x;
	if (leftX == terrain->width() - 1) 
		leftX--;
	float fracX = x - leftX;

	int outZ = (int)z;
	if (outZ == terrain->width() - 1)
		outZ--;
	float fracZ = z - outZ;

	//Compute the four heights for the grid cell
	float h11 = terrain->getHeight(leftX, outZ);
	float h12 = terrain->getHeight(leftX, outZ + 1);
	float h21 = terrain->getHeight(leftX + 1, outZ);
	float h22 = terrain->getHeight(leftX + 1, outZ + 1);

	//Take a weighted average of the four heights
	return (1 - fracX) * ((1 - fracZ) * h11 + fracZ * h12) + fracX * ((1 - fracZ) * h21 + fracZ * h22);
}

float getmagnitude(float a,float b,float c)
{
	return sqrt((a*a)+(b*b)+(c*c));
}

float getdistance(Vec3f a,Vec3f b)
{
	return sqrt(((a[0]-b[0])*(a[0]-b[0]))+((a[1]-b[1])*(a[1]-b[1]))+((a[2]-b[2])*(a[2]-b[2])));
}

void writeScore(float x, float y, char *str) 
{ 	//(x,y) is from the bottom left of the window 
	glMatrixMode(GL_PROJECTION); 
	glPushMatrix(); 
	glLoadIdentity(); 
	glMatrixMode(GL_MODELVIEW); 
	glPushMatrix(); 
	glLoadIdentity(); 
	glPushAttrib(GL_DEPTH_TEST); 
	glDisable(GL_DEPTH_TEST); 
	glRasterPos2f(x,y); 
	glColor3f(0,0,0);
	if(timeleft>0)
		for (unsigned int i=0; i<strlen(str); i++) 
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, str[i]); 
	else
		for (unsigned int i=0; i<strlen(str); i++) 
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str[i]); 
	glPopAttrib(); 
	glMatrixMode(GL_PROJECTION); 
	glPopMatrix(); 
	glMatrixMode(GL_MODELVIEW); 
	glPopMatrix(); 
}

float _angle = 60.0f;
Terrain* _terrain;

void cleanup() 
{	delete _terrain;
}
//------------------CLASS FUNCTIONS END HERE----------------------

float RandomFloat(float a, float b) 
{	float random = ((float) rand()) / (float) RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}
//----------------------------------------------------------------------
// Process mouse drag events
// 
// This is called when dragging motion occurs. The variable
// angle stores the camera angle at the instance when dragging
// started, and deltaAngle is a additional angle based on the
// mouse movement since dragging started.
//----------------------------------------------------------------------
	void mouseMove(int x, int y) 
{	if (isDragging) 
	{	// only when dragging, update the change in angle
		deltaAngle = (x - xDragStart) * 0.005;
		lx = -sin(angle + deltaAngle+DEG2RAD(bikeAngle));
		ly = cos(angle + deltaAngle+DEG2RAD(bikeAngle));
	}
}

	void mouseButton(int button, int state, int x, int y) 
{	if (button == GLUT_LEFT_BUTTON) 
	{	if (state == GLUT_DOWN) // left mouse button pressed
		{	isDragging = 1; // start dragging
			xDragStart = x; // save x where button first pressed
		}
		else  	/* (state = GLUT_UP) */
		{	angle += deltaAngle; // update camera turning angle
			isDragging = 0; // no longer dragging
		}
	}
}

void initRendering() 
{	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
	Image* image1 = loadBMP("z.bmp");
	Image* image2 = loadBMP("x.bmp");	
	Image* image3 = loadBMP("a.bmp");	
	Image* image4 = loadBMP("b.bmp");	
	_textureId1 = loadTexture(image1);
	_textureId2 = loadTexture(image2);
	_textureId3 = loadTexture(image3);
	_textureId4 = loadTexture(image4);
	delete image1;
	delete image2;
	delete image3;
	delete image4;
	Image* image21 = loadBMP("skybox/5.bmp");//5
	_skybox[0] = loadTexture(image21);
	delete image21;
	Image* image31 = loadBMP("skybox/4.bmp");//4 
	_skybox[1] = loadTexture(image31);
	delete image31;
	Image* image41 = loadBMP("skybox/3.bmp");//2
	_skybox[2] = loadTexture(image41);
	delete image41;
	Image* image51 = loadBMP("skybox/1.bmp");
	_skybox[3] = loadTexture(image51);
	delete image51;
	Image* image61 = loadBMP("skybox/2.bmp");//3
	_skybox[4] = loadTexture(image61);
	delete image61;



}

void handleResize(int w, int h) 
{	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}

void drawBike()
{	glTranslatef(bikeX,bikeY,bikeZ);
	glRotatef(bikeAngle,0,1,0);
	glRotatef(-pitch,1,0,0);
	glRotatef(tilt,0,0,1);
	GLfloat lightColor1[] = {100, 100, 0, 1.0f}; 
	GLfloat lightPos1[] = {0,0,0, 1.0f}; 
	GLfloat lightpoint[] = {0,0,1}; 
	//	GLfloat lightpoint[] = {(bikeX+5)*sin(DEG2RAD(bikeAngle)),0,(bikeZ+5)*cos(DEG2RAD(bikeAngle)) ,1.0f}; 
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1); 
	glLightfv(GL_LIGHT1, GL_POSITION, lightPos1); 
	glLightf(GL_LIGHT1,GL_SPOT_CUTOFF, 20); 
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 128.0f); 
	glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION, lightpoint);

	glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
	// Top face (y = 1.0f)
	// Define vertices in counter-clockwise (CCW) order with normal pointing out
	glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
	glVertex3f( 0.3f,  1.0f, 1.0f);
	glVertex3f(-0.3f,  1.0f, 1.0f);
	glVertex3f(-0.3f, 1.0f, -1.0f);
	glVertex3f( 0.3f, 1.0f, -1.0f);

	// Bottom face (y = 0.2f)
	glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
	glVertex3f( 0.3f,  0.2f, 1.0f);
	glVertex3f(-0.3f,  0.2f, 1.0f);
	glVertex3f(-0.3f, 0.2f, -1.0f);
	glVertex3f( 0.3f, 0.2f, -1.0f);

	// Front face  (z = 1.0f)
	glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
	glVertex3f( 0.3f,  1.0f, 1.0f);
	glVertex3f(-0.3f,  1.0f, 1.0f);
	glVertex3f(-0.3f,  0.2f, 1.0f);
	glVertex3f( 0.3f,  0.2f, 1.0f);

	// Back face (z = -1.0f)
	glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
	glVertex3f( 0.3f, 1.0f, -1.0f);
	glVertex3f(-0.3f, 1.0f, -1.0f);
	glVertex3f(-0.3f, 0.2f, -1.0f);
	glVertex3f( 0.3f, 0.2f, -1.0f);

	// Left face (x = -0.3f)
	glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
	glVertex3f(-0.3f,  1.0f, 1.0f);
	glVertex3f(-0.3f,  0.2f, 1.0f);
	glVertex3f(-0.3f, 0.2f, -1.0f);
	glVertex3f(-0.3f, 1.0f, -1.0f);

	// Right face (x = 0.3f)
	glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
	glVertex3f( 0.3f,  1.0f, 1.0f);
	glVertex3f( 0.3f,  0.2f, 1.0f);
	glVertex3f( 0.3f, 0.2f, -1.0f);
	glVertex3f( 0.3f, 1.0f, -1.0f);
	glEnd();  
	// End of drawing color-cube

	//LEFT HEADLIGHT	
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 0.0f);     
	glTranslatef(-0.2,0.85,1);
	glutSolidSphere(0.1, 20, 20); 
	glPopMatrix();

	//RIGHT HEADLIGHT	
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 0.0f);     //Yellow
	glTranslatef(0.2,0.85,1);
	glutSolidSphere(0.1, 20, 20); 
	glPopMatrix();

	//BACK WHEEL	
	glPushMatrix();
	glLineWidth(1);
	glColor3f(0.0f, 0.0f, 0.0f);     
	glTranslatef(0,0.2,-1.05);
	glRotatef(90,0,1,0);
	glRotatef(torRotate,0,0,1);	
	glutWireTorus(0.15,0.22, 20, 20); 
	glPopMatrix();

	//FRONT WHEEL	
	glPushMatrix();
	glColor3f(0.0f, 0.0f, 0.0f);  
	glTranslatef(0,0.2,1.05);
	glRotatef(90,0,1,0);
	glRotatef(torRotate,0,0,1);	
	glutWireTorus(0.15,0.22, 20, 20); 
	glPopMatrix();

	//HANDLES
	glPushMatrix();
	glTranslatef(0,0.85,0.75);
	glColor3f(1.0,0.0,0.0); 
	glLineWidth(10);
	glBegin(GL_LINES);
	glVertex3f(0.125,0.125,0.0);
	glVertex3f(0.4,0.4,0.0);
	glVertex3f(0.4,0.4,0.0);
	glVertex3f(0.6,0.5,0.0);
	glVertex3f(-0.125,0.125,0.0);
	glVertex3f(-0.4,0.4,0.0);
	glVertex3f(-0.4,0.4,0.0);
	glVertex3f(-0.6,0.5,0.0);
	glEnd();
	glPopMatrix();

	//DIAL
	glPushMatrix();
	glTranslatef(0,1.1,0.75);
	glColor3f(0.8,0.9,0.9); 
	glutSolidSphere(0.1, 20, 20);
	glRotatef(20,1,0,0);
	GLUquadricObj *quadratic;
	quadratic = gluNewQuadric();
	gluCylinder(quadratic,0.1f,0.1f,0.1f,32,32);
	glPopMatrix(); 
	glPopMatrix();
}

void genFossils()
{	int i;
	for(i=0;i<scorearr[level];i++)
	{	fossil a;	
		a.fossil_x=RandomFloat(0, _terrain->width()-1);
		a.fossil_z=RandomFloat(0, _terrain->length()-1);
		a.fossil_y= _terrain->getHeight(a.fossil_x, a.fossil_z);
		fossils.push_back(a);	//fossils.erase(fossils.begin()+i); TO ERASE iTH FOSSIL
	}
}

void drawSun()
{	glColor3f(1.0f, 1.0f, 0.0f);     //BLACK
	glTranslatef(100,75,100);
	GLfloat lightColor2[] = {100, 100, 100, 1.0f}; 
	GLfloat lightPos2[] = {0,0,0,1};
	GLfloat lightpoint[] = {50,0,50}; 
	glLightfv(GL_LIGHT2, GL_DIFFUSE, lightColor2); 
	glLightfv(GL_LIGHT2, GL_POSITION, lightPos2); 
	//	glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.0);
	glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 2);
	//	glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 1);
	glLightf(GL_LIGHT2,GL_SPOT_CUTOFF, 170); 
	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 128.0f); 
	glLightfv(GL_LIGHT2,GL_SPOT_DIRECTION, lightpoint);
	glutSolidSphere(5, 20, 20); 
}

void drawSky()
{	



	//starting skybox
	//front

	if(!night)
	{
		float cx = _terrain->length()+20,negx=-20;
		float cy = 50;
		float cy2 = -20;
		float cz = _terrain->width()+20;

		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, _skybox[2]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(  cx, cy2, negx );
		glTexCoord2f(1, 0); glVertex3f( negx, cy2, negx );
		glTexCoord2f(1, 1); glVertex3f( negx,  cy, negx );
		glTexCoord2f(0, 1); glVertex3f(  cx,  cy, negx );
		glEnd();
		//left

		glBindTexture(GL_TEXTURE_2D, _skybox[4]);



		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(  cx, cy2,  cz );
		glTexCoord2f(1, 0); glVertex3f(  cx, cy2, negx );
		glTexCoord2f(1, 1); glVertex3f(  cx,  cy, negx );
		glTexCoord2f(0, 1); glVertex3f(  cx,  cy,  cz );
		glEnd();
		//right

		glBindTexture(GL_TEXTURE_2D, _skybox[1]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f( negx, cy2, negx );
		glTexCoord2f(1, 0); glVertex3f( negx, cy2,  cz );
		glTexCoord2f(1, 1); glVertex3f( negx,  cy,  cz );
		glTexCoord2f(0, 1); glVertex3f( negx,  cy, negx );
		glEnd();
		//back


		glBindTexture(GL_TEXTURE_2D, _skybox[0]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f( negx, cy2,  cz );
		glTexCoord2f(1, 0); glVertex3f(  cx, cy2,  cz );
		glTexCoord2f(1, 1); glVertex3f(  cx,  cy,  cz );
		glTexCoord2f(0, 1); glVertex3f( negx,  cy,  cz );
		glEnd();
		//up

		glBindTexture(GL_TEXTURE_2D, _skybox[3]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f( negx,  cy, negx);
		glTexCoord2f(0, 0); glVertex3f( negx,  cy,  cz );
		glTexCoord2f(1, 0); glVertex3f(  cx,  cy,  cz );
		glTexCoord2f(1, 1); glVertex3f(  cx,  cy, negx);
		glEnd();
		//down



		glBindTexture(GL_TEXTURE_2D, _skybox[2]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f( negx,  cy2, negx );
		glTexCoord2f(0, 0); glVertex3f( negx,  cy2,  cz );
		glTexCoord2f(1, 0); glVertex3f(  cx,  cy2,  cz );
		glTexCoord2f(1, 1); glVertex3f(  cx,  cy2, negx );
		glEnd();

		glDisable(GL_TEXTURE_2D);


	}

	else
	{
		glTranslatef(_terrain->width()/2,0,_terrain->length()/2);
		glEnable(GL_TEXTURE_2D);	

		glBindTexture(GL_TEXTURE_2D, _textureId3);


		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//SKY
		int x=125,y=25,z=125;

		glBegin(GL_QUADS);            
		glVertex3f(-x,y,-z);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-x,y,z);
		glTexCoord2f(5.0f, 0.0f);
		glVertex3f(x,y,z);
		glTexCoord2f(5.0f, 5.0f);
		glVertex3f(x,y,-z);
		glTexCoord2f(0.0f, 5.0f);
		glEnd();
		glBegin(GL_QUADS);            
		glVertex3f(-x,-y,-z);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-x,-y,z);
		glTexCoord2f(5.0f, 0.0f);
		glVertex3f(x,-y,z);
		glTexCoord2f(5.0f, 5.0f);
		glVertex3f(x,-y,-z);
		glTexCoord2f(0.0f, 5.0f);
		glEnd();
		glBegin(GL_QUADS);            
		glVertex3f(-x,-y,-z);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-x,-y,z);
		glTexCoord2f(5.0f, 0.0f);
		glVertex3f(-x,y,z);
		glTexCoord2f(5.0f, 5.0f);
		glVertex3f(-x,y,-z);
		glTexCoord2f(0.0f, 5.0f);
		glEnd();
		glBegin(GL_QUADS);            
		glVertex3f(x,-y,-z);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(x,-y,z);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(x,y,z);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(x,y,-z);
		glTexCoord2f(0.0f, 1.0f);
		glEnd();
		glBegin(GL_QUADS);            
		glVertex3f(x,-y,z);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-x,-y,z);
		glTexCoord2f(5.0f, 0.0f);
		glVertex3f(-x,y,z);
		glTexCoord2f(5.0f, 5.0f);
		glVertex3f(x,y,z);
		glTexCoord2f(0.0f, 5.0f);
		glEnd();
		glBegin(GL_QUADS);            
		glVertex3f(x,-y,-z);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-x,-y,-z);
		glTexCoord2f(5.0f, 0.0f);
		glVertex3f(-x,y,-z);
		glTexCoord2f(5.0f, 5.0f);
		glVertex3f(x,y,-z);
		glTexCoord2f(0.0f, 5.0f);
		glEnd();
		glDisable(GL_TEXTURE_2D);

	}



}

void drawFossils()
{	int i;
	glColor3f(RandomFloat(0,1),RandomFloat(0,1),RandomFloat(0,1));
	for(i=0;i<fossils.size();i++)
	{	glPushMatrix();
		glTranslatef(fossils[i].fossil_x,fossils[i].fossil_y+0.1,fossils[i].fossil_z);
		glutSolidSphere(0.7, 20, 20); 
		glPopMatrix();
	}
}

void collectFossils()
{	int i;
	for(i=0;i<fossils.size();i++)
	{	Vec3f f(fossils[i].fossil_x,fossils[i].fossil_y,fossils[i].fossil_z);
		Vec3f bike(bikeX,bikeY,bikeZ);
		if(getdistance(f,bike)<3.5)
		{	fossils.erase(fossils.begin()+i);
			alSourcePlay(Sources[2]);	
			score++;	//increase the score on eating fossil
		}
	}
}

void drawScene() 
{	char str[100],go[100],qtoe[100],sc[100],view[100],winner[100];	
	if(timeleft==0)
	{	glClearColor(0.0,0.7,0.5,1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPushMatrix();
		glColor3f(0,0,0);
		sprintf(go,"GAME OVER !!");	//GAME OVER CONDITION
		sprintf(sc,"Score = %d",score);
		sprintf(qtoe,"PRESS Q TO EXIT");
		writeScore(-0.17,0.0,go);
		writeScore(-0.1,-0.1,sc);
		writeScore(-0.2,-0.2,qtoe);
		glPopMatrix();	
	}
	else if(score==scorearr[level])
	{	glClearColor(0.0,0.7,0.5,1.0);	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPushMatrix();	
		glColor3f(0,0,0);
		sprintf(go,"YOU WIN !!");	//WIN CONDITION
		sprintf(sc,"Score = %d",score);
		sprintf(qtoe,"PRESS Q TO EXIT");
		if(level==0)
			sprintf(winner,"OR L TO PROCEED TO NEXT LEVEL");
		writeScore(-0.17,0.0,go);
		writeScore(-0.1,-0.1,sc);
		writeScore(-0.2,-0.2,qtoe);
		if(level==0)
			writeScore(-0.2,-0.3,winner);
		glPopMatrix();	
	}
	else
	{	if(!night)	
		glClearColor(0.0,0.7,1.0,1.0);	//SKY BLUE BACKGROUND COLOR
		else
			glClearColor(0.0,0.0,0.0,1.0);
		if(night==0)	
		{//	glEnable(GL_LIGHT0);
			glEnable(GL_LIGHT2);
			glDisable(GL_LIGHT1);
		}
		else	
		{	glEnable(GL_LIGHT1);
			glDisable(GL_LIGHT2);
			glDisable(GL_LIGHT0);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0,0,0);
		GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

		GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
		GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

		if(time(0)-pt > 1)
		{	timeleft=timeleft-1;
			pt=time(0);
		}


		if(bikeX<0.1)
			bikeX=0.1;
		if(bikeX> _terrain->width()-1.5)
			bikeX= _terrain->width()-1.5;
		if(bikeZ<0.1)
			bikeZ=0.1;
		if(bikeZ> _terrain->length()-1.5)
			bikeZ= _terrain->length()-1.5;

		ha=heightAt(_terrain,bikeX,bikeZ);
		hb=ha;
		if(flight==1)
			ha=prevheight-0.1;
		if(prevheight-(hb)>0.2)
			flight=1;
		else if(ha<0)
			flight=0;
		prevheight=ha;

		Vec3f tnormal = _terrain->getNormal(bikeX, bikeZ);	
		if(!flight)
		{	float tryc,ab,ac,ad;
			ab=sinf(DEG2RAD(bikeAngle));
			ac=0;
			ad=cosf(DEG2RAD(bikeAngle));
			tryc=((tnormal[0]*ab+tnormal[2]*ad+tnormal[1]*ac)/(getmagnitude(tnormal[0],tnormal[1],tnormal[2])*getmagnitude(ab,ac,ad)));
			tryc=RAD2DEG(acos(tryc));
			pitch=-(90-tryc);
		}
		bikeY=ha+0.25;

		//VIEW SPECIFICATION
		if(camselect==0)	//Driver View
		{	gluLookAt(bikeX-4*sinf(DEG2RAD(bikeAngle)), bikeY+3, bikeZ-4*cosf(DEG2RAD(bikeAngle)),
				bikeX, bikeY+2.8, bikeZ,
				0.0,    1.0,    0.0);
		}
		else if(camselect==1)	//Wheel view
		{	gluLookAt(	bikeX,   bikeY+2.0, bikeZ+0.5, 
				bikeX+sinf(DEG2RAD(bikeAngle)), bikeY+2, bikeZ+0.5+cosf(DEG2RAD(bikeAngle)),
				0.0,    1.0,    0.0);
		}
		else if(camselect==2)	//Overhead View
		{	gluLookAt(	bikeX,      bikeY+18.0,      bikeZ,
				bikeX+sinf(DEG2RAD(bikeAngle)), bikeY+2, bikeZ+cosf(DEG2RAD(bikeAngle)),
				0.0,        1.0,        0.0);
		}	
		else if(camselect==3)	//Follow Cam
		{	gluLookAt(bikeX-8*sinf(DEG2RAD(bikeAngle)), bikeY+2, bikeZ-8*cosf(DEG2RAD(bikeAngle)),
				bikeX, bikeY+2, bikeZ,
				0.0,    1.0,    0.0);
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _textureId2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glColor3f(0.0f, 1.0f, 0.0f);
		for(int z = 0; z < _terrain->length() - 1; z++) 
		{	glBegin(GL_TRIANGLE_STRIP); //Makes OpenGL draw a triangle at every three consecutive vertices
			for(int x = 0; x < _terrain->width(); x++) 
			{	if( _terrain->getHeight(x, z) > 3)	
				glColor3f(1.0f, 1.0f, 1.0f);
				else
					glColor3f(0.6f, 0.4f, 0.12f);
				Vec3f normal = _terrain->getNormal(x, z);
				glNormal3f(normal[0], normal[1], normal[2]);
				glVertex3f(x, _terrain->getHeight(x, z), z);
				if( _terrain->getHeight(x, z) > 2.5)	
					glTexCoord2f(0.0f, 0.0f);
				normal = _terrain->getNormal(x, z + 1);
				glNormal3f(normal[0], normal[1], normal[2]);
				glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
				if( _terrain->getHeight(x, z) > 2.5)	
					glTexCoord2f(10.0f, 0.0f);						
			}
			glEnd();
		}
		glDisable(GL_TEXTURE_2D);
		if(level==0)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, _textureId1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			//WATER

			glBegin(GL_QUADS);                // Begin drawing the color cube with 6 quads
			glColor3f(0.0f, 0.0f, 1.0f);     // Yellow
			glVertex3f(0,0.02,0);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(0,0.02,3);
			glTexCoord2f(5.0f, 0.0f);
			glVertex3f(175,0.02,180);
			glTexCoord2f(5.0f, 5.0f);
			glVertex3f(180,0.02,177);
			glTexCoord2f(0.0f, 5.0f);
			glEnd();
			glDisable(GL_TEXTURE_2D);
		}

		glPushMatrix();
		drawBike();	//draw the bike
		glPopMatrix();

		glPushMatrix();
		drawSky();
		glPopMatrix();

		if(!night)
		{	glPushMatrix();
			drawSun();
			glPopMatrix();
		}

		glPushMatrix();
		drawFossils();	//draw randomly generated fossils
		glPopMatrix();

		glPushMatrix();	
		if(!night)
			glColor3f(0,0,0);
		else
			glColor3f(1,1,1);
		sprintf(str,"Score = %d",score);
		writeScore(0.55,0.88,str);	//SCORE WRITING	
		glPopMatrix();	

		glPushMatrix();		
		if(!night)
			glColor3f(0,0,0);
		else
			glColor3f(1,1,1);
		sprintf(str,"Time Left = %d",timeleft);
		writeScore(0.55,0.78,str);	//TIME WRITING	
		glPopMatrix();	

		glPushMatrix();	
		if(!night)
			glColor3f(0,0,0);
		else
			glColor3f(1,1,1);
		if(camselect==0)
			sprintf(view,"DRIVER VIEW");
		else if(camselect==1)
			sprintf(view,"WHEEL VIEW");
		else if(camselect==3)
			sprintf(view,"FOLLOW CAM");
		writeScore(-0.97,0.88,view);	//VIEW NAME
		glPopMatrix();
	}
	glutSwapBuffers();
}

void update(void) 
{
	//cout<<fossils.size()<<"\n";	
	collectFossils();	
	if(translate)
	{	vel+=acc;
		if(vel>1.0)
			vel=1.0;
	}
	else if(!translate)
	{	if(vel>0)
		vel-=decel;
		else
			vel=0;
	}
	if (rotate) // update bike position (rotate)
		bikeAngle+=rotate*5;	//right if dr=1, left if -1

	if(tilted && (tilt+5*tilted)>-45 && (tilt+5*tilted)<45)
		tilt=tilt+5*tilted;
	if(tilted && vel>0.05)
		bikeAngle+=-0.6*tanf(DEG2RAD(tilt))/vel;	

	if (vel) // update bike position (translate)
	{	bikeX+= sinf(DEG2RAD(bikeAngle)) * vel;	//forward if dm=1,backward if -1
		bikeZ+= cosf(DEG2RAD(bikeAngle)) * vel;
		torRotate+=vel*2;
	}

	//	printf("bikeangle=%f\n",bikeAngle);
	glutPostRedisplay(); // redisplay everything
}
//----------------------------------------------------------------------
// User-input callbacks
//
// processNormalKeys: ESC, q, and Q cause program to exit
// pressSpecialKey: Up arrow = forward motion, down arrow = backwards
// releaseSpecialKey: Set incremental motion to zero
//----------------------------------------------------------------------
void handleKeypress(unsigned char key, int x, int y) 
{	if (key == ESC || key == 'q' || key == 'Q') exit(0);
	if (key == 'c' || key == 'C') 
	{	camselect++;
		if(camselect==4)
			camselect=0;
	}
	if((key=='a' || key=='A') && !flight)
		rotate=1.0; 
	if((key=='d' || key=='D') && !flight)
		rotate=-1.0;
	if(key=='n')
		night=1-night;
	if(key=='l' && level==0 && score==scorearr[0])
	{	cleanup();
		timeleft=120;
		score=0;
		level+=1;
		_terrain = loadTerrain("aa.bmp", 20);
		genFossils();
	}
} 

	void pressSpecialKey(int key, int xx, int yy)
{	if(key==GLUT_KEY_UP)
	{	alSourcePlay(Sources[1]);	
		translate = 1.0;
	} 
	else if(key==GLUT_KEY_DOWN && !flight)
		vel = 0.0; 
	else if(key==GLUT_KEY_LEFT && !flight)
		tilted = -1; 
	else if(key==GLUT_KEY_RIGHT && !flight)
		tilted = 1; 
} 

	void releaseSpecialKey(int key, int x, int y) 
{	if(key==GLUT_KEY_UP || key==GLUT_KEY_DOWN)
	translate = 0.0; 
	else if(key==GLUT_KEY_LEFT || key==GLUT_KEY_RIGHT)
	{	tilted = 0; 
		tilt=0;
	}
} 

void releaseKeyboardKey(unsigned char key, int x, int y) 
{	if(key=='a' || key=='A' || key=='d' || key=='D')
	rotate = 0.0; 
} 

//----------------------------------------------------------------------
// Main program  - standard GLUT initializations and callbacks
//----------------------------------------------------------------------
int main(int argc, char** argv) 
{	alutInit(NULL, 0);	// Initialize OpenAL and clear the error bit.
	alGetError();
	if(LoadALData() == AL_FALSE)	// Load the wav data.
		return 0;
	SetListenerValues();
	atexit(KillALData);	// Setup an exit procedure.
	//alSourcePlay(Sources[0]);	// Begin playing.
	printf("-------------------------------------------------------------\nFossil Park Ride:\n- Hold up/down arrow-key to move bike forward/backward\n- Hold left/right arrow-key to tilt the bike\n- Hold 'a' or 'd' to move bike left/right\n- Press 'c' to change camera mode\n- q or ESC to quit\n-------------------------------------------------------------\n");

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(800, 400);
	glutCreateWindow("Fossil Park Ride");
	initRendering();

	_terrain = loadTerrain("heightmap.bmp", 20);
	genFossils();	//generate 50 Randomly placed fossils

	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutSpecialFunc(pressSpecialKey); // process special key pressed
	glutReshapeFunc(handleResize);
	glutIdleFunc(update); // incremental update 
	glutIgnoreKeyRepeat(1); // ignore key repeat when holding key down
	glutMouseFunc(mouseButton); // process mouse button push/release
	glutMotionFunc(mouseMove); // process mouse dragging motion
	// Warning: Nonstandard function! 
	glutSpecialUpFunc(releaseSpecialKey); // process special key release
	glutKeyboardUpFunc(releaseKeyboardKey); // process special key release

	// OpenGL init
	glEnable(GL_DEPTH_TEST);

	// enter GLUT event processing cycle
	glutMainLoop();
	return 0;
}
