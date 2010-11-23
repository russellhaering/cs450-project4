/************************************************************************/
/*						 CS 450 / 550 Assignment4  					    */
/*							by Christophe Torne							*/
/*								11/11/2010								*/
/************************************************************************/


#include <stdlib.h> /* Must come first to avoid redef error */

#include "glslprogram.h"
#include "helperFunctions.h"

#ifdef __APPLE__
#include <GLUI/glui.h>
#else
#include <GL/glui.h>
#endif

#include <vector>

using namespace std;

/************************************************************************/
/*							DATA STRUCTURES                             */
/************************************************************************/

typedef struct 
{
	float ambient[3];
	float diffuse[3];
	float specular[3];
} Material;

typedef struct 
{
	int v, t, n;
} Point;

typedef struct Object
{
	Object()
	{
		normals = vertices = NULL;
		texcoords = NULL;
		faces = NULL;
	}
	~Object()
	{
		glDeleteLists(displayList, 1);
		delete [] normals;
		delete [] vertices;
		delete [] texcoords;
		delete [] faces;
	}
	Material material;
	string name;
	float (*normals)[3], (*vertices)[3], (*texcoords)[2];
	Point (*faces)[3];
	int numVertices, numNormals, numFaces, numTexCoords;
	GLuint displayList;
} Object;

typedef struct
{
	GLuint programID;
	string VSFile, FSFile;
} Shader;

typedef struct 
{
	GLuint textureID;
	unsigned char *values;
	int width, height;
} Texture;

enum buttonTypes {OBJ_TEXTFIELD = 0, LOAD_BUTTON, SCREENSHOT};
enum projections {ORTHO = 0, PERSP, FOV};
enum shadingModels {FLAT = 0, SMOOTH};
enum textures {NOTEXTURE = -1, OSU, EARTH, WOOD};
enum texFilters {LINEAR = 0, NEAREST};
enum texWraps {REPEAT = 0, CLAMPTOEDGE};
enum texEnvironments {REPLACE = 0, MODULATE, DECAL, BLEND};
enum texGenModes {OBJECT = 0, EYE, SPHERE};
enum shaders {NOSHADER = -1, PASS_THROUGH, DIFFUSE, PHONG, TOON, PROCEDURAL};

/************************************************************************/
/*                       FUNCTION PROTOTYPES                            */
/************************************************************************/

int		loadObj			(const char *, Object &);
void	setupVV			(void);
void	projCB			(int);
void	textureCB		(int);
void	shadingCB		(int);
void	textCB			(int);
void	buttonCB		(int);
void	colorCB			(int);
void	drawObjects		(GLenum);
void	myGlutDisplay	(void);
void	myGlutReshape	(int, int);
void	myGlutMouse		(int , int , int , int);
void	processHits		(GLint, GLuint[]);
void	initScene		();

/************************************************************************/
/*                   CONSTANTS & GLOBAL VARIABLES                       */
/************************************************************************/

vector<Object> Objects;

const unsigned WIN_WIDTH = 500;
const unsigned WIN_HEIGHT = 500;

int main_window;
int objSelected = -1;

const float light0_pos[] = {0.f, 3.f, 2.f, 1.f};
Material lightCoeffs;
Material objectMat;

int fov;
const int FOVMIN = 0;
const int FOVMAX = 90;

projections projType;
shadingModels shadingModel;
textures texture;
texFilters texMinFilter;
texFilters texMagFilter;
texWraps texWrap_S;
texWraps texWrap_T;
texEnvironments texEnv;
texGenModes texGen;
shaders shader;

#define NUMTEXTURES 3
const char *textureFiles[NUMTEXTURES] = {
	"data/textures/osu.bmp",
	"data/textures/earth.bmp",
	"data/textures/wood.bmp"
};
Texture *tex[NUMTEXTURES];

#define NUMSHADERS 2
char *vsFiles[NUMSHADERS] = {
	"data/shaders/PassThrough.vert",
	"data/shaders/Diffuse.vert"
};
char *fsFiles[NUMSHADERS] = {
	"data/shaders/PassThrough.frag",
	"data/shaders/Diffuse.frag"
};
GLSLProgram *programs[NUMSHADERS];

int sizeX = WIN_WIDTH;
int sizeY = WIN_WIDTH;
float vl = -1.f;
float vr = 1.f;
float vb = -1.f;
float vt = 1.f;
const float vn = .9f;
const float vf = 3.f;

float scaleFactor = 1.f;
GLfloat rotMatrix[4][4];
GLfloat xRotation = 0.f, yRotation = 0.f;
int xMouse, yMouse;

GLUI *glui;
GLUI_EditText *objFileNameTextField;
const string path = string("data/objects/");
const string ext = string(".obj");

/************************************************************************/
/*                       FUNCTION DEFINITIONS                           */
/************************************************************************/

/// Reads the contents of the obj file and appends the data at the end of 
/// the vector of Objects. This time we allow the same object to be loaded
/// several times.
int loadObj(const char *fileName, Object &obj)
{
	bool stprovided = false;
	ifstream file;
	file.open(fileName);

	if(!file.is_open())
	{
		cerr << "Cannot open .obj file " << fileName << endl;
		return EXIT_FAILURE;
	}

	obj.name = string(fileName);
	obj.material.diffuse[0] = obj.material.diffuse[1] = obj.material.diffuse[2] = 1.f;
	obj.material.ambient[0] = obj.material.ambient[1] = obj.material.ambient[2] = 1.f;
	obj.material.specular[0] = obj.material.specular[1] = obj.material.specular[2] = 1.f;

	/// First pass: count the vertices, normals, texture coordinates and faces, allocate the arrays.
	int numVertices = 0, numNormals = 0, numTexCoords = 0, numFaces = 0;
	string buffer;
	while (getline(file, buffer), !buffer.empty())
	{
		if (buffer[0] == 'v')
		{
			if (buffer[1] == 'n')
				numNormals++;
			else
			{
				if (buffer[1] == ' ')
					numVertices++;
				else
				{
					if (buffer[1] == 't')
					{
						stprovided = true;
						numTexCoords++;
					}
				}
			}
		}
		else
		{
			if (buffer[0] == 'f')
				numFaces++;
		}
	};

	cout << "Verts\t" << numVertices << endl;
	cout << "Normals\t" << numNormals << endl;
	cout << "Faces\t" << numFaces << endl;
	cout << "Texture Coordinates\t" << numTexCoords << endl;

	obj.vertices = new float [numVertices][3];
	obj.numVertices = numVertices;
	obj.normals = new float [numNormals][3];
	obj.numNormals = numNormals;
	obj.faces = new Point [numFaces][3];
	obj.numFaces = numFaces;
	if (stprovided)
	{
		obj.texcoords = new float [numTexCoords][2];
		obj.numTexCoords = numTexCoords;
	}

	file.clear();
	file.seekg(ios::beg);

	/// Second pass: populate the arrays
	numFaces = numNormals = numVertices = numTexCoords = 0;
	while (getline(file, buffer), !buffer.empty())
	{
		if (buffer[0] == 'v')
		{
			if (buffer[1] == 'n')
			{
				sscanf (	buffer.data() + 2*sizeof(char), " %f %f %f",
					&obj.normals[numNormals][0],
					&obj.normals[numNormals][1],
					&obj.normals[numNormals][2]);
				numNormals++;

			}
			else
			{
				if (buffer[1] == ' ')
				{
					sscanf (	buffer.data() + sizeof(char), " %f %f %f",
						&obj.vertices[numVertices][0],
						&obj.vertices[numVertices][1],
						&obj.vertices[numVertices][2]);
					numVertices++;
				}
				else
				{
					if (buffer[1] == 't')
					{
						sscanf (	buffer.data() + 2*sizeof(char), " %f %f",
							&obj.texcoords[numTexCoords][0],
							&obj.texcoords[numTexCoords][1]);
						numTexCoords++;
					}
				}
			}
		}
		else
		{
			if (buffer[0] == 'f')
			{
				if (stprovided)
					sscanf(buffer.data() + sizeof(char), " %d/%d/%d %d/%d/%d %d/%d/%d",
					&obj.faces[numFaces][0].v, &obj.faces[numFaces][0].t, &obj.faces[numFaces][0].n,
					&obj.faces[numFaces][1].v, &obj.faces[numFaces][1].t, &obj.faces[numFaces][1].n,
					&obj.faces[numFaces][2].v, &obj.faces[numFaces][2].t, &obj.faces[numFaces][2].n);
				else
					sscanf(buffer.data() + sizeof(char), " %d//%d %d//%d %d//%d",
					&obj.faces[numFaces][0].v, &obj.faces[numFaces][0].n,
					&obj.faces[numFaces][1].v, &obj.faces[numFaces][1].n,
					&obj.faces[numFaces][2].v, &obj.faces[numFaces][2].n);
				numFaces++;
			}
		}
	};

	file.close();
	cout << "Finished loading " << fileName << endl;

	/// If texture coordinates were not provided, let OpenGL generate some for us.
	/// Object linear is static, Eye linear depends on the position of the camera.
	/// Sphere map acts as if the objects was reflecting a textured background.
	/// This has to be set before loading the object, it will have no effect on the
	/// current object. It will ignore all the glTexCoord calls for this object.
	if (!stprovided)
	{
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		GLfloat zPlane[] = {0.f, 0.f, 1.f, 0.f};
		switch (texGen)
		{
		case OBJECT:
			/// Object Linear
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, zPlane);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, zPlane);
			break;
		case EYE:
			/// Eye Linear
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
			glTexGenfv(GL_S, GL_EYE_PLANE, zPlane);
			glTexGenfv(GL_T, GL_EYE_PLANE, zPlane);
			break;
		case SPHERE:
			/// Sphere Map
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
			break;
		}
	}
	else
	{
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}

	/// Now we generate the display list.
	obj.displayList = glGenLists(1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glNewList(obj.displayList, GL_COMPILE);
	{
		glBegin(GL_TRIANGLES);
		{
			if (stprovided)
			{
				for (int faceNum = 0 ; faceNum < obj.numFaces ; faceNum++)
				{
					glNormal3fv(obj.normals[obj.faces[faceNum][0].n-1]);
					glTexCoord2fv(obj.texcoords[obj.faces[faceNum][0].t-1]);
					glVertex3fv(obj.vertices[obj.faces[faceNum][0].v-1]);
					glNormal3fv(obj.normals[obj.faces[faceNum][1].n-1]);
					glTexCoord2fv(obj.texcoords[obj.faces[faceNum][1].t-1]);
					glVertex3fv(obj.vertices[obj.faces[faceNum][1].v-1]);
					glNormal3fv(obj.normals[obj.faces[faceNum][2].n-1]);
					glTexCoord2fv(obj.texcoords[obj.faces[faceNum][2].t-1]);
					glVertex3fv(obj.vertices[obj.faces[faceNum][2].v-1]);
				}
			}
			else
			{
				for (int faceNum = 0 ; faceNum < obj.numFaces ; faceNum++)
				{
					glNormal3fv(obj.normals[obj.faces[faceNum][0].n-1]);
					glVertex3fv(obj.vertices[obj.faces[faceNum][0].v-1]);
					glNormal3fv(obj.normals[obj.faces[faceNum][1].n-1]);
					glVertex3fv(obj.vertices[obj.faces[faceNum][1].v-1]);
					glNormal3fv(obj.normals[obj.faces[faceNum][2].n-1]);
					glVertex3fv(obj.vertices[obj.faces[faceNum][2].v-1]);
				}
			}
		}
		glEnd();
	}
	glEndList();

	return EXIT_SUCCESS;
}


void projCB(int id)
{
	switch(id)
	{
	case ORTHO:
		projType = ORTHO;
		break;
	case PERSP:
	case FOV:
		projType = PERSP;
		break;
	default:
		break;
	}

	setupVV();

	glui->sync_live();
	glutPostRedisplay();
}

void textureCB(int id)
{
	glui->sync_live();
	glutPostRedisplay();
}

void shadingCB(int id)
{
	glui->sync_live();
	glutPostRedisplay();
}

void textCB(int id)
{
	glui->sync_live();
	glutPostRedisplay();
}

void buttonCB(int control)
{
	string filename = path + string(objFileNameTextField->get_text()) + ext;
	switch (control)
	{
	case LOAD_BUTTON:
		/// Delete the previous object and replace it with an empty one
		Objects.assign(1, Object());
		xRotation = yRotation = 0.f;
		loadObj(filename.data(), Objects.back());
		break;
	case SCREENSHOT:
		snapshot();
		break;
	}

	glui->sync_live();
	glutPostRedisplay();
}

void colorCB(int id)
{
	if ((id == 0) && (objSelected != -1))
	{
		/// UPDATE THE OBJECT MATERIAL
	}
	glui->sync_live();
	glutPostRedisplay();
}

void drawObjects(GLenum mode)
{
	for (int i = 0 ; i < (int) Objects.size() ; i++)
	{
		if (mode == GL_SELECT)
			glLoadName(i);

		glCallList(Objects.at(i).displayList);

		if ((mode == GL_RENDER) && (objSelected == i))
		{
			glDisable(GL_LIGHTING);
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			glColor3f(1.f - Objects.at(i).material.diffuse[0], 1.f - Objects.at(i).material.diffuse[1], 1.f - Objects.at(i).material.diffuse[2]);
			glCallList(Objects.at(i).displayList);

			glEnable(GL_LIGHTING);
			glDisable(GL_POLYGON_OFFSET_LINE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
}

void setupVV()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (projType == ORTHO)
		glOrtho(vl, vr, vb, vt, vn, vf);
	else
		glFrustum(vl * fov/FOVMAX, vr * fov/FOVMAX, vb * fov/FOVMAX, vt * fov/FOVMAX, vn, vf);
}

void setupView()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.f, 0.f, -2.f);

	glRotatef(yRotation, 0.f, 1.f, 0.f);
	glRotatef(xRotation, 1.f, 0.f, 0.f);

	if(scaleFactor < 0.05f)
		scaleFactor = 0.05f;
	glScalef((GLfloat)scaleFactor, (GLfloat)scaleFactor, (GLfloat)scaleFactor);
}

void myGlutMotion(int x, int y)
{
	if (objSelected != -1)
	{
		int dx = x - xMouse;
		int dy = y - yMouse;
		xRotation += dy;
		yRotation += dx;
		xMouse = x;
		yMouse = y;
		glutPostRedisplay();
	}
}

void myGlutDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);

	setupView();

	/// ADD YOUR CODE HERE
	// This is where you choose textures, shading model, etc. based on the interface
	// Alternatively, you can use the callbacks above to update the relevant OGL state

	// Set the light position
	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightCoeffs.diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightCoeffs.ambient);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightCoeffs.specular);

	cerr << "Using Shader: " << shader << endl;
	switch (shader) {
	case -1:
		// No shader in use
		break;
	case 0 ... (NUMSHADERS - 1):
		// Implemented shader in use
		programs[shader]->Use();
		break;
	default:
		// Unimplemented shader in use
		cout << "Unimplemented Shader Selected: " << shader << endl;
		break;
	}

	drawObjects(GL_RENDER);

	glui->sync_live();
	glutSwapBuffers();
	glFlush();
}


void myGlutReshape(int x, int y)
{
	sizeX = x;
	sizeY = y;
	glViewport(0, 0, sizeX, sizeY);

	if(sizeX <= sizeY)
	{
		vb = vl * sizeY / sizeX;
		vt = vr * sizeY / sizeX;
	}
	else
	{
		vl = vb * sizeX / sizeY;
		vr = vt * sizeX / sizeY;
	}

	setupVV();

	glutPostRedisplay();
}

void myGlutMouse(int button, int button_state, int x, int y)
{
	GLuint selectBuffer[512];
	GLint viewport[4];

	if(button != GLUT_LEFT_BUTTON || button_state != GLUT_DOWN)
		return;

	glGetIntegerv(GL_VIEWPORT, viewport);

	glSelectBuffer(512, selectBuffer);
	glRenderMode(GL_SELECT);

	glInitNames();
	glPushName(-1);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluPickMatrix((GLdouble) x, (GLdouble) (viewport[3]-y), 2.0, 2.0, viewport);

	if (projType == ORTHO)
		glOrtho(vl, vr, vb, vt, vn, vf);
	else
		glFrustum(vl * fov/FOVMAX, vr * fov/FOVMAX, vb * fov/FOVMAX, vt * fov/FOVMAX, vn, vf);

	drawObjects(GL_SELECT);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glFlush();

	processHits(glRenderMode(GL_RENDER), selectBuffer);

	glutPostRedisplay();
}

void processHits(GLint hits, GLuint buffer[])
{
	int i;
	int pick = -1;
	unsigned int min_depth = 4294967295;
	GLuint *ptr = (GLuint *) buffer;
	Object *obj;

	cout << "Number of hits = " << hits << endl;
	for(i = 0 ; i < hits ; i++, ptr+=4)
	{
		if (*(ptr+1) < min_depth)
		{
			min_depth = *(ptr+1);
			pick = *(ptr+3);
		}
	}

	objSelected = pick;

	if (pick != -1)
	{
		cout << "We got ourselves a winner: #" << pick << endl;

		/// UPDATE THE INTERFACE MATERIAL VALUES TO REFLECT THE SELECTED OBJECT
		obj = &Objects.at(objSelected);
		for (i = 0; i < 3; i++) {
			objectMat.ambient[i] = obj->material.ambient[i];
			objectMat.diffuse[i] = obj->material.diffuse[i];
		}
	}
	else
	{
		objectMat.ambient[0] = objectMat.diffuse[0] = 1.f;
		objectMat.ambient[1] = objectMat.diffuse[1] = 1.f;
		objectMat.ambient[2] = objectMat.diffuse[2] = 1.f;
	}
	glui->sync_live();
}

void initScene()
{
	int i;

	/// ADD YOUR CODE HERE TO INITIALIZE TEXTURES, LIGHTING, CREATE SHADERS, CREATE TEXTURES, etc.
	glEnable(GL_NORMALIZE);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	/*
	glShadeModel(GL_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glClearColor(BACKGROUND_COLOR);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(0, -100);
	*/

	//Here's an example of how to initialize a shader, however, it will not work until lighting is properly 
	//initialized above.
	for (i = 0; i < NUMSHADERS; i++) {
		programs[i] = new GLSLProgram(vsFiles[i], fsFiles[i]);
	}
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);  
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);

	main_window = glutCreateWindow("OBJ Loader");
	glutDisplayFunc(myGlutDisplay);
	glutReshapeFunc(myGlutReshape);  
	glutMouseFunc(myGlutMouse);
	glutMotionFunc(myGlutMotion);

	initScene();
	Objects.push_back(Object());

	glui = GLUI_Master.create_glui("OBJ Loader GUI", 0, 600, 50);

	GLUI_Panel *objPanel = glui->add_panel("");
	GLUI_Panel *objPanel2 = glui->add_panel_to_panel(objPanel, "", 0);
	objFileNameTextField = glui->add_edittext_to_panel(objPanel2, "Filename:",GLUI_EDITTEXT_TEXT,NULL,OBJ_TEXTFIELD,textCB);
	objFileNameTextField->set_text("cow");
	glui->add_button_to_panel(objPanel2, "Load", LOAD_BUTTON, buttonCB);
	glui->add_button_to_panel(objPanel2, "Screenshot (or it didn't happen)", SCREENSHOT, buttonCB);

	glui->add_column_to_panel(objPanel, 0);

	GLUI_Panel *projPanel = glui->add_panel_to_panel(objPanel, "", 0);
	GLUI_RadioGroup *projGroup = glui->add_radiogroup_to_panel(projPanel, (int*) &projType, -1, projCB);
	glui->add_radiobutton_to_group(projGroup, "Orthographic");
	glui->add_radiobutton_to_group(projGroup, "Perspective");
	projGroup->set_int_val(ORTHO);

	GLUI_Spinner *fovSpinner = glui->add_spinner_to_panel(projPanel, "FOV", GLUI_SPINNER_INT, &fov, FOV, projCB);
	fovSpinner->set_int_limits(FOVMIN, FOVMAX);
	fovSpinner->set_int_val(30);

	glui->add_column_to_panel(projPanel, 0);

	GLUI_Translation *scale = glui->add_translation_to_panel(projPanel, "Scale", GLUI_TRANSLATION_Y , &scaleFactor, -1, (GLUI_Update_CB) projCB);
	scale->set_speed(0.005f);

	GLUI_Panel *lightingPanel = glui->add_panel("", 0);

	GLUI_Panel *objMaterialPanel = glui->add_panel_to_panel(lightingPanel, "Object material");

	GLUI_Panel *objDiffusePanel = glui->add_panel_to_panel(objMaterialPanel, "Diffuse");
	GLUI_Spinner *kdRedValue = glui->add_spinner_to_panel(objDiffusePanel, "Red", GLUI_SPINNER_FLOAT, &objectMat.diffuse[0], 0, colorCB);
	kdRedValue->set_float_limits(0.f, 1.f);
	kdRedValue->set_float_val(1.f);
	GLUI_Spinner *kdGreenValue = glui->add_spinner_to_panel(objDiffusePanel, "Green", GLUI_SPINNER_FLOAT, &objectMat.diffuse[1], 0, colorCB);
	kdGreenValue->set_float_limits(0.f, 1.f);
	kdGreenValue->set_float_val(1.f);
	GLUI_Spinner *kdBlueValue = glui->add_spinner_to_panel(objDiffusePanel, "Blue", GLUI_SPINNER_FLOAT, &objectMat.diffuse[2], 0, colorCB);
	kdBlueValue->set_float_limits(0.f, 1.f);
	kdBlueValue->set_float_val(1.f);

	GLUI_Panel *objAmbientPanel = glui->add_panel_to_panel(objMaterialPanel, "Ambient");
	GLUI_Spinner *kaRedValue = glui->add_spinner_to_panel(objAmbientPanel, "Red", GLUI_SPINNER_FLOAT, &objectMat.ambient[0], 0, colorCB);
	kaRedValue->set_float_limits(0.f, 1.f);
	kaRedValue->set_float_val(1.f);
	GLUI_Spinner *kaGreenValue = glui->add_spinner_to_panel(objAmbientPanel, "Green", GLUI_SPINNER_FLOAT, &objectMat.ambient[1], 0, colorCB);
	kaGreenValue->set_float_limits(0.f, 1.f);
	kaGreenValue->set_float_val(1.f);
	GLUI_Spinner *kaBlueValue = glui->add_spinner_to_panel(objAmbientPanel, "Blue", GLUI_SPINNER_FLOAT, &objectMat.ambient[2], 0, colorCB);
	kaBlueValue->set_float_limits(0.f, 1.f);
	kaBlueValue->set_float_val(1.f);

	GLUI_Panel *objSpecularPanel = glui->add_panel_to_panel(objMaterialPanel, "Specular");
	GLUI_Spinner *ksRedValue = glui->add_spinner_to_panel(objSpecularPanel, "Red", GLUI_SPINNER_FLOAT, &objectMat.specular[0], 0, colorCB);
	ksRedValue->set_float_limits(0.f, 1.f);
	ksRedValue->set_float_val(1.f);
	GLUI_Spinner *ksGreenValue = glui->add_spinner_to_panel(objSpecularPanel, "Green", GLUI_SPINNER_FLOAT, &objectMat.specular[1], 0, colorCB);
	ksGreenValue->set_float_limits(0.f, 1.f);
	ksGreenValue->set_float_val(1.f);
	GLUI_Spinner *ksBlueValue = glui->add_spinner_to_panel(objSpecularPanel, "Blue", GLUI_SPINNER_FLOAT, &objectMat.specular[2], 0, colorCB);
	ksBlueValue->set_float_limits(0.f, 1.f);
	ksBlueValue->set_float_val(1.f);

	glui->add_column_to_panel(lightingPanel,0);
	GLUI_Panel *lightPanel = glui->add_panel_to_panel(lightingPanel, "Light source");

	GLUI_Panel *lightDiffusePanel = glui->add_panel_to_panel(lightPanel, "Diffuse");
	GLUI_Spinner *ldRedValue = glui->add_spinner_to_panel(lightDiffusePanel, "Red", GLUI_SPINNER_FLOAT, &lightCoeffs.diffuse[0], -1, colorCB);
	ldRedValue->set_float_limits(0.f, 1.f);
	ldRedValue->set_float_val(1.f);
	GLUI_Spinner *ldGreenValue = glui->add_spinner_to_panel(lightDiffusePanel, "Green", GLUI_SPINNER_FLOAT, &lightCoeffs.diffuse[1], -1, colorCB);
	ldGreenValue->set_float_limits(0.f, 1.f);
	ldGreenValue->set_float_val(1.f);
	GLUI_Spinner *ldBlueValue = glui->add_spinner_to_panel(lightDiffusePanel, "Blue", GLUI_SPINNER_FLOAT, &lightCoeffs.diffuse[2], -1, colorCB);
	ldBlueValue->set_float_limits(0.f, 1.f);
	ldBlueValue->set_float_val(1.f);

	GLUI_Panel *lightAmbientPanel = glui->add_panel_to_panel(lightPanel, "Specular");
	GLUI_Spinner *laRedValue = glui->add_spinner_to_panel(lightAmbientPanel, "Red", GLUI_SPINNER_FLOAT, &lightCoeffs.ambient[0], -1, colorCB);
	laRedValue->set_float_limits(0.f, 1.f);
	laRedValue->set_float_val(1.f);
	GLUI_Spinner *laGreenValue = glui->add_spinner_to_panel(lightAmbientPanel, "Green", GLUI_SPINNER_FLOAT, &lightCoeffs.ambient[1], -1, colorCB);
	laGreenValue->set_float_limits(0.f, 1.f);
	laGreenValue->set_float_val(1.f);
	GLUI_Spinner *laBlueValue = glui->add_spinner_to_panel(lightAmbientPanel, "Blue", GLUI_SPINNER_FLOAT, &lightCoeffs.ambient[2], -1, colorCB);
	laBlueValue->set_float_limits(0.f, 1.f);
	laBlueValue->set_float_val(1.f);

	GLUI_Panel *lightSpecularPanel = glui->add_panel_to_panel(lightPanel, "Ambient");
	GLUI_Spinner *lsRedValue = glui->add_spinner_to_panel(lightSpecularPanel, "Red", GLUI_SPINNER_FLOAT, &lightCoeffs.specular[0], -1, colorCB);
	lsRedValue->set_float_limits(0.f, 1.f);
	lsRedValue->set_float_val(1.f);
	GLUI_Spinner *lsGreenValue = glui->add_spinner_to_panel(lightSpecularPanel, "Green", GLUI_SPINNER_FLOAT, &lightCoeffs.specular[1], -1, colorCB);
	lsGreenValue->set_float_limits(0.f, 1.f);
	lsGreenValue->set_float_val(1.f);
	GLUI_Spinner *lsBlueValue = glui->add_spinner_to_panel(lightSpecularPanel, "Blue", GLUI_SPINNER_FLOAT, &lightCoeffs.specular[2], -1, colorCB);
	lsBlueValue->set_float_limits(0.f, 1.f);
	lsBlueValue->set_float_val(1.f);

	GLUI_Panel *texturingPanel = glui->add_panel("Texturing");
	GLUI_Listbox *textureListbox = glui->add_listbox_to_panel(texturingPanel, "Texture", (int*)&texture, -1, textureCB);
	textureListbox->add_item(NOTEXTURE, "No texture");
	textureListbox->add_item(OSU, "OSU");
	textureListbox->add_item(EARTH, "EARTH");
	textureListbox->add_item(WOOD, "WOOD");
	textureListbox->set_int_val(NOTEXTURE);

	GLUI_Panel *texFiltersPanel = glui->add_panel_to_panel(texturingPanel, "", 0);
	GLUI_Panel *minFilterPanel = glui->add_panel_to_panel(texFiltersPanel, "Minifying filter", 1);
	GLUI_RadioGroup *minFilterGroup = glui->add_radiogroup_to_panel(minFilterPanel, (int*) &texMinFilter, -1, textureCB);
	glui->add_radiobutton_to_group(minFilterGroup, "Linear");
	glui->add_radiobutton_to_group(minFilterGroup, "Nearest");
	minFilterGroup->set_int_val(LINEAR);

	GLUI_Panel *magFilterPanel = glui->add_panel_to_panel(texFiltersPanel, "Magnifying filter", 1);
	GLUI_RadioGroup *magFilterGroup = glui->add_radiogroup_to_panel(magFilterPanel, (int*) &texMagFilter, -1, textureCB);
	glui->add_radiobutton_to_group(magFilterGroup, "Linear");
	glui->add_radiobutton_to_group(magFilterGroup, "Nearest");
	magFilterGroup->set_int_val(LINEAR);

	glui->add_column_to_panel(texFiltersPanel, 0);

	GLUI_Panel *texWrapSPanel = glui->add_panel_to_panel(texFiltersPanel, "Wrapping S", 1);
	GLUI_RadioGroup *texWrapSGroup = glui->add_radiogroup_to_panel(texWrapSPanel, (int*) &texWrap_S, -1, textureCB);
	glui->add_radiobutton_to_group(texWrapSGroup, "Repeat");
	glui->add_radiobutton_to_group(texWrapSGroup, "Clamp to edge");
	texWrapSGroup->set_int_val(CLAMPTOEDGE);

	GLUI_Panel *texWrapTPanel = glui->add_panel_to_panel(texFiltersPanel, "Wrapping T", 1);
	GLUI_RadioGroup *texWrapTGroup = glui->add_radiogroup_to_panel(texWrapTPanel, (int*) &texWrap_T, -1, textureCB);
	glui->add_radiobutton_to_group(texWrapTGroup, "Repeat");
	glui->add_radiobutton_to_group(texWrapTGroup, "Clamp to edge");
	texWrapTGroup->set_int_val(CLAMPTOEDGE);

	GLUI_Listbox *texEnvListbox = glui->add_listbox_to_panel(texturingPanel, "Environment", (int*)&texEnv, -1, textureCB);
	texEnvListbox->add_item(REPLACE, "GL_REPLACE");
	texEnvListbox->add_item(MODULATE, "GL_MODULATE");
	texEnvListbox->add_item(DECAL, "GL_DECAL");
	texEnvListbox->add_item(BLEND, "GL_BLEND");
	texEnvListbox->set_int_val(MODULATE);

	GLUI_Listbox *texGenListbox = glui->add_listbox_to_panel(texturingPanel, "UV Generation", (int*)&texGen, -1, textureCB);
	texGenListbox->add_item(OBJECT, "Object Linear");
	texGenListbox->add_item(EYE, "Eye Linear");
	texGenListbox->add_item(SPHERE, "Sphere Map");
	texGenListbox->set_int_val(OBJECT);

	GLUI_Panel *shadingPanel = glui->add_panel("Shading");
	GLUI_RadioGroup *shadingGroup = glui->add_radiogroup_to_panel(shadingPanel, (int*) &shadingModel, -1, shadingCB);
	glui->add_radiobutton_to_group(shadingGroup, "Flat");
	glui->add_radiobutton_to_group(shadingGroup, "Smooth");
	shadingGroup->set_int_val(SMOOTH);

	GLUI_Listbox *shaderListbox = glui->add_listbox_to_panel(shadingPanel, "Shader", (int*)&shader, -1, shadingCB);
	shaderListbox->add_item(NOSHADER, "Fixed function");
	shaderListbox->add_item(PASS_THROUGH, "Pass through");
	shaderListbox->add_item(DIFFUSE, "Lambert illumination");
	shaderListbox->add_item(PHONG, "Phong illumination");
	shaderListbox->add_item(TOON, "Toon shading");
	shaderListbox->add_item(PROCEDURAL, "Procedural texture");
	shaderListbox->set_int_val(PASS_THROUGH);

	glui->set_main_gfx_window(main_window);

	// We register the idle callback with GLUI, *not* with GLUT 
	GLUI_Master.set_glutIdleFunc(NULL);
	glui->sync_live();
	glutMainLoop();
	return EXIT_SUCCESS;
}
