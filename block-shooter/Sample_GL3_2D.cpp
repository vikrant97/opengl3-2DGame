#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include<unistd.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include<ao/ao.h>
#include<mpg123.h>
#define BITS 8
#include<pthread.h>
#include<bits/stdc++.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/
mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;
int t=0,reload=0;
VAO *triangle,*bucket1, *bucket2,*gun1,*gun2,*bullet,*red_block,*green_block,*black_block,*mirror1,*mirror2,*mirror3,*mirror4;
float triangle_rot_dir = 1,zoom=1,x_change=0,y_change=0;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
int countred=0,countgreen=0,countblack=0,countBullet=0,flagbullet[1000],flagblack[1000],flaggreen[1000],flagred[1000],flagbulletmirror1[1000],flagbulletmirror2[1000],flagbulletmirror3[1000],flagbulletmirror4[1000];
float score=0,move1=0,move2=0,change=0,speed=0.03,current_time,rotation_angle=0,last_update_time,changered[1000]={4.5},changegreen[1000]={4.5},changeblack[1000]={4.5},xred[1000],xgreen[1000],xblack[1000],bulletx[1000],bullety[1000],adjusty[1000],rotationBullet[1000];
void initialise()
{
	int i;
	srand(time(NULL));
	for(i=0;i<1000;i++)
	{
		changered[i]=4.5;
		xred[i]=(float)(((rand())%680-280)/100.0);
		xgreen[i]=(float)(((rand())%680-280)/100.0);
		xblack[i]=(float)(((rand())%680-280)/100.0);
		changeblack[i]=4.5;
		changegreen[i]=4.5;
		bulletx[i]=0;
		bullety[i]=0;
		adjusty[i]=0;
		rotationBullet[i]=0;
		flagbullet[i]=0;
		flagblack[i]=0;
		flaggreen[i]=0;
		flagred[i]=0;
		flagbulletmirror1[i]=0;
		flagbulletmirror2[i]=0;
		flagbulletmirror3[i]=0;
		flagbulletmirror4[i]=0;

	}

}
void* playsound(void *x)
{
while(1)
{
 while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        ao_play(dev, (char *)buffer, done);
}
}
int control=0,alt=0;
/* Executed when a regular key is pressed */
void keyboard (unsigned char key, int x, int y)
{
	switch (key) {

		case 'Q':
		case 'q':
		case 27: //ESC
			control=0;
			alt=0;
			exit (0);
		case 32:
			control=0;
			alt=0;
			countBullet++;
			adjusty[countBullet]=change;
			rotationBullet[countBullet]=rotation_angle;
			break;
		case 'n':
			control=0;
			alt=0;
			speed*=2;
			break;
		case 'm':
			control=0;
			alt=0;
			speed/=2;
			break;
		case 's':
			control=0;
			alt=0;
			change+=0.2;
			break;
		case 'a':
			control=0;
			alt=0;
			rotation_angle+=5;
			break;
		case 'd':
			control=0;
			alt=0;
			rotation_angle-=5;
			break;
		case 'f':
			control=0;
			alt=0;
			change-=0.2;
			break;
		default:
			control=0;
			alt=0;
			break;
	}

}

/* Executed when a regular key is released */
void keyboardUp (unsigned char key, int x, int y)
{

	switch (key) {
		case 'c':
		case 'C':
			rectangle_rot_status = !rectangle_rot_status;
			break;
		case 'p':
		case 'P':
			triangle_rot_status = !triangle_rot_status;
			break;
		case 'x':
			// do something
			break;
		default:
			break;
	}
}
/* Executed when a special key is pressed */

void keyboardSpecialDown (int key, int x, int y)
{
	switch(key)
	{
		case 101:
			zoom+=0.2;
			break;
		case 103:
			if(zoom-0.2>=0.2)
				zoom-=0.2;
			break;
		case 114:
			control=1;
			alt=0;
			break;
		case 115:
			control=1;
			alt=0;
			break;
		case 116:
			alt=1;
			control=0;
			break;
		case 117:
			alt=1;
			control=0;
			break;
		case 100:
			if(control==1)
				move1+=0.3;
			else if(alt==1)
				move2+=0.3;
			else
				x_change-=0.2;
			break;
		case 102:
			if(control==1)
				move1-=0.3;
			else if(alt==1)
				move2-=0.3;
			else
				x_change+=0.2;
			break;
		default:
			control=0;
			alt=0;
			break;
	}

}

/* Executed when a special key is released */
void keyboardSpecialUp (int key, int x, int y)
{
	switch(key)
	{
		case 114:
			control=0;
			break;
		case 116:
			alt=0;
			break;
		case 115:
			control=0;
			break;
		case 117:
			alt=0;
			break;

	}
}

/* Executed when a mouse button 'button' is put into state 'state'
   at screen position ('x', 'y')
 */
int check_pan,check_redbucket=0,right_click=0,left_click=0,check_gun=0;
void mouseClick (int button, int state, int x, int y)
{
	switch (button) {
		case 4:
			zoom+=0.2;
			break;
		case 3:
			if(zoom-0.2>=0.2)
				zoom-=0.2;
			break;
		case GLUT_LEFT_BUTTON:
			right_click=0;
			if (state == 0)
			{
			countBullet++;
			adjusty[countBullet]=change;
			rotationBullet[countBullet]=rotation_angle;

			left_click=1;
			check_redbucket=x;
			check_gun=y;
			}
			if(state==1)
				left_click=0;
			break;

		case GLUT_RIGHT_BUTTON:
			right_click=1;
			left_click=0;
			if (state == GLUT_DOWN) {
				check_pan=x;
			}
			break;
		default:
			right_click=0;
			left_click=0;
			break;
	}
}

/* Executed when the mouse moves to position ('x', 'y') */
void mouseMotion (int x, int y)
{
	if(right_click==1)
	{
		if(x>check_pan)
		{
			check_pan=x;
			x_change+=0.02;
		}
		else if(x<check_pan)
		{
			check_pan=x;
			x_change-=0.02;
		}
	}
	if(left_click==1)
	{
			if(-0.6f-2.0f-move1<=x/100.0-4.0f and x/100.0-4.0f<=0.6f-2.0f-move1 and 4.0-y/75.0<=-3.6)
			{
				if(x>=check_redbucket)
				move1-=0.05;
				else
					move1+=0.05;
				check_redbucket=x;
			}
			else if(-0.6f+2.0f-move2<=x/100.0-4.0f and x/100.0-4.0f<=0.6f+2.0f-move2 and 4.0-y/75.0<=-3.6)
			{
				if(x>=check_redbucket)
				move2-=0.05;
				else
					move2+=0.05;
				check_redbucket=x;
			}
			else if(x/100.0-4.0>=-4.0 and x/100.0-4.0<=-3.7)
			{
				if(y>=check_gun)
				change-=0.03;
				else
					change+=0.03;
				check_gun=y;
			}
	}


}
/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (int width, int height)
{
	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) width, (GLsizei) height);

	// set the projection matrix as perspective/ortho
	// Store the projection matrix in a variable for future use

	// Perspective projection for 3D views
	// Matrices.projection = glm::perspective (fov, (GLfloat) width / (GLfloat) height, 0.1f, 500.0f);

	// Ortho projection for 2D views
	Matrices.projection = glm::ortho(-(4.0f), (4.0f), (-4.0f), (4.0f), 0.1f, 500.0f);
}


// Creates the triangle object used in this sample code
void createTriangle ()
{
	/* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

	/* Define vertex array as used in glBegin (GL_TRIANGLES) */
	static const GLfloat vertex_buffer_data [] = {
		0, 1,0, // vertex 0
		-1,-1,0, // vertex 1
		1,-1,0, // vertex 2
	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 0
		0,1,0, // color 1
		0,0,1, // color 2
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	triangle = create3DObject(GL_TRIANGLES, 12, vertex_buffer_data, color_buffer_data, GL_LINE);
}

void createbucket1 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {

		-0.4,0.4,0, // vertex 1
		-0.4,-0.4,0, // vertex 2
		0.4, -0.4,0, // vertex 3

		0.4, -0.4,0, // vertex 3
		0.4, 0.4,0, // vertex 4
		-0.4,0.4,0,  // vertex 1

		0.4,0.4,0, //vertex 4
		0.6,0.4,0, //vertex 5
		0.4,-0.4,0, //vertex 3

		-0.4,0.4,0, //vertex 1
		-0.6,0.4,0, //vertex 6
		-0.4,-0.4,0 //vertex 2
	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 1
		1,0,0, // color 1
		1,0,0, // color 1

		1,0,0, // color 1
		1,0,0, // color 1
		1,0,0, // color 1

		1,0,0, // color 1
		1,0,0, // color 1
		1,0,0, // color 1

		1,0,0, // color 1
		1,0,0, // color 1
		1,0,0 // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	bucket1 = create3DObject(GL_TRIANGLES, 12, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createmirror1 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.4,0.025,0, // vertex 1
		-0.4,-0.025,0, // vertex 2
		0.4, -0.025,0, // vertex 3

		0.4, -0.025,0, // vertex 3
		0.4, 0.025,0, // vertex 4
		-0.4,0.025,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	mirror1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createmirror2 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.4,0.025,0, // vertex 1
		-0.4,-0.025,0, // vertex 2
		0.4, -0.025,0, // vertex 3

		0.4, -0.025,0, // vertex 3
		0.4, 0.025,0, // vertex 4
		-0.4,0.025,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	mirror2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createmirror3 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.4,0.025,0, // vertex 1
		-0.4,-0.025,0, // vertex 2
		0.4, -0.025,0, // vertex 3

		0.4, -0.025,0, // vertex 3
		0.4, 0.025,0, // vertex 4
		-0.4,0.025,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	mirror3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createmirror4 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.4,0.025,0, // vertex 1
		-0.4,-0.025,0, // vertex 2
		0.4, -0.025,0, // vertex 3

		0.4, -0.025,0, // vertex 3
		0.4, 0.025,0, // vertex 4
		-0.4,0.025,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1
		0.52f,0.8f,0.98f, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	mirror4 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createblack_block ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.1,0.1,0, // vertex 1
		-0.1,-0.1,0, // vertex 2
		0.1, -0.1,0, // vertex 3

		0.1, -0.1,0, // vertex 3
		0.1, 0.1,0, // vertex 4
		-0.1,0.1,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0,0,0, // color 1
		0,0,0, // color 1
		0,0,0, // color 1

		0,0,0, // color 1
		0,0,0, // color 1
		0,0,0, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	black_block = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void creategreen_block ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.1,0.1,0, // vertex 1
		-0.1,-0.1,0, // vertex 2
		0.1, -0.1,0, // vertex 3

		0.1, -0.1,0, // vertex 3
		0.1, 0.1,0, // vertex 4
		-0.1,0.1,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0,0.5,0, // color 1
		0,0.5,0, // color 1
		0,0.5,0, // color 1

		0,0.5,0, // color 1
		0,0.5,0, // color 1
		0,0.5,0, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	green_block = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createred_block ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.1,0.1,0, // vertex 1
		-0.1,-0.1,0, // vertex 2
		0.1, -0.1,0, // vertex 3

		0.1, -0.1,0, // vertex 3
		0.1, 0.1,0, // vertex 4
		-0.1,0.1,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		1,0,0, // color 1
		1,0,0, // color 1
		1,0,0, // color 1

		1,0,0, // color 1
		1,0,0, // color 1
		1,0,0, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	red_block = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createBullet ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-3.5,0.05,0, // vertex 1
		-3.5,-0.05,0, // vertex 2
		-3.4, -0.05,0, // vertex 3


		-3.4, -0.05,0, // vertex 3
		-3.4,0.05,0, // vertex 4
		-3.5,0.05,0,  // vertex 1

		//-3.87, -0.05,0, // vertex 3
		//-3.87,0.05,0, // vertex 4
		//-3.80,0,0  // vertex 1


	};

	static const GLfloat color_buffer_data [] = {
		0.2,0.2,0.2, // color 1
		0.2,0.2,0.2, // color 1
		0.2,0.2,0.2, // color 1

		0.2,0.2,0.2, // color 1
		0.2,0.2,0.2, // color 1
		0.2,0.2,0.2, // color 1

		//0.2,0.2,0.2, // color 1
		//0.2,0.2,0.2, // color 1
		//0.2,0.2,0.2 // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	bullet = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createGun1 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-4,0.3,0, // vertex 1
		-4,-0.3,0, // vertex 2
		-3.7, -0.3,0, // vertex 3

		-3.7, -0.3,0, // vertex 3
		-3.7,0.3,0, // vertex 4
		-4,0.3,0,  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0,0,1, // color 1
		0,0,1, // color 1
		0,0,1, // color 1

		0,0,1, // color 1
		0,0,1, // color 1
		0,0,1, // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	gun1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createGun2 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-3.9, -0.05,0, // vertex 3
		-3.9,0.05,0, // vertex 4
		-3,0.05,0,  // vertex 1

		-3,0.05,0,
		-3,-0.05,0,
		-3.9,-0.05,0



	};

	static const GLfloat color_buffer_data [] = {
		0,0,1, // color 1
		0,0,1, // color 1
		0,0,1, // color 1

		0,0,1, // color 1
		0,0,1, // color 1
		0,0,1, // color 1

	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	gun2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createbucket2 ()
{
	// GL3 accepts only Triangles. Quads are not supported static
	const GLfloat vertex_buffer_data [] = {
		-0.4,0.4,0, // vertex 1
		-0.4,-0.4,0, // vertex 2
		0.4, -0.4,0, // vertex 3

		0.4, -0.4,0, // vertex 3
		0.4, 0.4,0, // vertex 4
		-0.4,0.4,0,  // vertex 1

		0.4,0.4,0, //vertex 4
		0.6,0.4,0, //vertex 5
		0.4,-0.4,0, //vertex 3

		-0.4,0.4,0, //vertex 1
		-0.6,0.4,0, //vertex 6
		-0.4,-0.4,0 //vertex 2
	};

	static const GLfloat color_buffer_data [] = {
		0,0.5,0, // color 1
		0,0.5,0, // color 1
		0,0.5,0, // color 1

		0,0.5,0, // color 1
		0,0.5,0, // color 1
		0,0.5,0, // color 1

		0,0.5,0, // color 1
		0,0.5,0, // color 1
		0,0.5,0, // color 1

		0,0.5,0, // color 1
		0,0.5,0, // color 1
		0,0.5,0 // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	bucket2 = create3DObject(GL_TRIANGLES, 12, vertex_buffer_data, color_buffer_data, GL_FILL);
}


float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
	int i;
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	Matrices.projection = glm::ortho(-(4.0f)/zoom+x_change, (4.0f)/zoom+x_change, (-4.0f)/zoom+y_change, (4.0f)/zoom+y_change, 0.1f, 500.0f);
	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model
	//mirror1
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatemirror1 = glm::translate (glm::vec3(3.0f, 0.0f, 0.0f)); // glTranslatef
	glm::mat4 rotatemirror1 = glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= translatemirror1*rotatemirror1;
	MVP = VP * Matrices.model; // MVP = p * V * M
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	srand(time(NULL));
	draw3DObject(mirror1);
	//mirror2
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatemirror2 = glm::translate (glm::vec3(2.0f, 3.0f, 0.0f)); // glTranslatef
	glm::mat4 rotatemirror2 = glm::rotate((float)(120*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= translatemirror2*rotatemirror2;
	MVP = VP * Matrices.model; // MVP = p * V * M
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	srand(time(NULL));
	draw3DObject(mirror2);
	//mirror3
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatemirror3 = glm::translate (glm::vec3(1.0f, -2.0f, 0.0f)); // glTranslatef
	glm::mat4 rotatemirror3 = glm::rotate((float)(60*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= translatemirror3*rotatemirror3;
	MVP = VP * Matrices.model; // MVP = p * V * M
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	srand(time(NULL));
	draw3DObject(mirror3);
	//mirror4
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatemirror4 = glm::translate (glm::vec3(-2.5f, 2.5f, 0.0f)); // glTranslatef
	glm::mat4 rotatemirror4 = glm::rotate((float)(15*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= translatemirror4*rotatemirror4;
	MVP = VP * Matrices.model; // MVP = p * V * M
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	srand(time(NULL));
	draw3DObject(mirror4);
	///Reflection from mirrors
	float cx,bx,cy,by,w,bw;
	int j=0;
	for(i=1;i<=countBullet;i++)
	{
		cx=-3.45+bulletx[i];
		cy=adjusty[i]+bullety[i];
		w=0.05;
		if(flagbullet[i]==0 && flagbulletmirror1[i]==0)
		{
			bx=3;
			by=0;
			bw=0.025;
			if((abs(bx-cx)<=w+bw) && (abs(by-cy)<=w+0.4))
			{
				flagbulletmirror1[i]=1;
				rotationBullet[i]=2*90-rotationBullet[i];
				break;
			}
		}

		if(flagbullet[i]==0 && flagbulletmirror2[i]==0)
		{
			bx=2;
			by=3;
			bw=0.04;
			if((abs(bx-cx)<=w+bw) && (abs(by-cy)<=w+0.4))
			{
				flagbulletmirror2[i]=1;
				rotationBullet[i]=2*120-rotationBullet[i];
				break;
			}




		}
		if(flagbullet[i]==0 && flagbulletmirror3[i]==0)
		{
			bx=1;
			by=-2;
			bw=0.04;
			if((abs(bx-cx)<=w+bw) && (abs(by-cy)<=w+0.4))
			{
				flagbulletmirror3[i]=1;
				rotationBullet[i]=2*60-rotationBullet[i];
				break;
			}




		}
		if(flagbullet[i]==0 && flagbulletmirror4[i]==0)
		{
			bx=-2.5;
			by=2.5;
			bw=0.025;

			if((abs(bx-cx)<=0.4) && (abs(by-cy)<=0.4))
			{
				flagbulletmirror4[i]=1;
				rotationBullet[i]=2*15-rotationBullet[i];
				break;
			}




		}
	}
	//bucket1
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatebucket1 = glm::translate (glm::vec3(-2.0f-move1, -3.6f, 0.0f)); // glTranslatef
	Matrices.model *= translatebucket1;
	MVP = VP * Matrices.model; // MVP = p * V * M
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	srand(time(NULL));
	draw3DObject(bucket1);

	/*colision with bucket1 and bucket2*/
	for(i=1;i<=countblack;i++)
	{
		if(flagblack[i]==0)
		{
			if((-0.1+changeblack[i]<=-3.2))
			{
				if( ((-0.6f-2.0f-move1<=-0.1+xblack[i]) and (-0.1+xblack[i]<=0.6f-2.0f-move1)) or ((-0.6f-2.0f-move1<=0.1+xblack[i]) and (0.1+xblack[i]<=0.6f-2.0f-move1)))
				{
					flagblack[i]=1;
					cout<<"Game Over"<<endl;
					cout<<"Total score:"<<score<<endl;
					exit(0);
					break;

				}
			}



		}
		if(flagblack[i]==0)
		{
			if((-0.1+changeblack[i]<=-3.2))
			{
				if( ((-0.6f+2.0f-move2<=-0.1+xblack[i]) and (-0.1+xblack[i]<=0.6f+2.0f-move2)) or ((-0.6f+2.0f-move2<=0.1+xblack[i]) and (0.1+xblack[i]<=0.6f+2.0f-move2)))

				{
					flagblack[i]=1;
					cout<<"Game Over"<<endl;
					cout<<"Total score:"<<score<<endl;
					exit(0);
					break;

				}
			}



		}


	}
	for(i=1;i<=countred;i++)
	{
		if(flagred[i]==0)
		{
			if((-0.1+changered[i]<=-3.2))
			{
				if( ((-0.6f-2.0f-move1<=-0.1+xred[i]) and (-0.1+xred[i]<=0.6f-2.0f-move1)) or ((-0.6f-2.0f-move1<=0.1+xred[i]) and (0.1+xred[i]<=0.6f-2.0f-move1)))
				{
					flagred[i]=1;
					score+=4;
					break;

				}
			}



		}
		if(flagred[i]==0)
		{
			if((-0.1+changered[i]<=-3.2))
			{
				if( ((-0.6f+2.0f-move2<=-0.1+xred[i]) and (-0.1+xred[i]<=0.6f+2.0f-move2)) or ((-0.6f+2.0f-move2<=0.1+xred[i]) and (0.1+xred[i]<=0.6f+2.0f-move2)))

				{
					flagred[i]=1;
					score-=1;
					break;

				}
			}



		}


	}
	for(i=1;i<=countgreen;i++)
	{
		if(flaggreen[i]==0)
		{
			if((-0.1+changegreen[i]<=-3.2))
			{
				if( ((-0.6f-2.0f-move1<=-0.1+xgreen[i]) and (-0.1+xgreen[i]<=0.6f-2.0f-move1)) or ((-0.6f-2.0f-move1<=0.1+xgreen[i]) and (0.1+xgreen[i]<=0.6f-2.0f-move1)))
				{
					flaggreen[i]=1;
					score-=1;
					break;

				}
			}



		}
		if(flaggreen[i]==0)
		{
			if((-0.1+changegreen[i]<=-3.2))
			{
				if( ((-0.6f+2.0f-move2<=-0.1+xgreen[i]) and (-0.1+xgreen[i]<=0.6f+2.0f-move2)) or ((-0.6f+2.0f-move2<=0.1+xgreen[i]) and (0.1+xgreen[i]<=0.6f+2.0f-move2)))

				{
					flaggreen[i]=1;
					score+=4;
					break;

				}
			}



		}


	}



	//bucket2
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatebucket2 = glm::translate (glm::vec3(2.0f-move2, -3.6f, 0.0f));        // glTranslatef
	Matrices.model *= (translatebucket2);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(bucket2);

	//Draw red,black & green blocks;
	for(i=1;i<=countred;i++)
	{
		if(flagred[i]==0)
		{

			Matrices.model = glm::mat4(1.0f);

			/* Render your scene */
			glm::mat4 translateRed = glm::translate (glm::vec3(xred[i], changered[i], 0.0f)); // glTranslatef
			Matrices.model *= translateRed;
			MVP = VP * Matrices.model; // MVP = p * V * M

			//  Don't change unless you are sure!!
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

			// draw3DObject draws the VAO given to it using current MVP matrix
			draw3DObject(red_block);
		}
	}
	for(i=1;i<=countgreen;i++)
	{
		if(flaggreen[i]==0)
		{
			Matrices.model = glm::mat4(1.0f);

			/* Render your scene */
			glm::mat4 translateGreen = glm::translate (glm::vec3(xgreen[i], changegreen[i], 0.0f)); // glTranslatef
			Matrices.model *= translateGreen;
			MVP = VP * Matrices.model; // MVP = p * V * M

			//  Don't change unless you are sure!!
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

			// draw3DObject draws the VAO given to it using current MVP matrix
			draw3DObject(green_block);
		}
	}
	for(i=1;i<=countblack;i++)
	{
		if(flagblack[i]==0)
		{
			Matrices.model = glm::mat4(1.0f);

			/* Render your scene */
			glm::mat4 translateBlack = glm::translate (glm::vec3(xblack[i], changeblack[i], 0.0f)); // glTranslatef
			Matrices.model *= translateBlack;
			MVP = VP * Matrices.model; // MVP = p * V * M

			//  Don't change unless you are sure!!
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

			// draw3DObject draws the VAO given to it using current MVP matrix
			draw3DObject(black_block);
		}
	}

	///Draw Gun1;
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translate1gun1 = glm::translate (glm::vec3(3.75f, 0.0f, 0.0f));        // glTranslatef
	glm::mat4 translate2gun1 = glm::translate (glm::vec3(-3.75f, change, 0.0f));        // glTranslatef
	glm::mat4 rotategun1 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translate2gun1*rotategun1*translate1gun1);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(gun1);

	//Draw gun2;
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translate1gun2 = glm::translate (glm::vec3(3.75f, 0.0f, 0.0f));        // glTranslatef
	glm::mat4 translate2gun2 = glm::translate (glm::vec3(-3.75f, change, 0.0f));        // glTranslatef
	glm::mat4 rotategun2 = glm::rotate((float)(rotation_angle*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translate2gun2*rotategun2*translate1gun2);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(gun2);
	//Draw rendered scene of bullet;
	for(i=1;i<=countBullet;i++)
	{

		if(flagbullet[i]==0)
		{

			Matrices.model = glm::mat4(1.0f);

			glm::mat4 translatebullet3 = glm::translate (glm::vec3(bulletx[i], bullety[i], 0.0f));        // glTranslatef
			glm::mat4 translatebullet2 = glm::translate (glm::vec3(-3.75f, adjusty[i], 0.0f));        // glTranslatef
			glm::mat4 translatebullet1 = glm::translate (glm::vec3(3.45f, 0.0f, 0.0f));        // glTranslatef
			glm::mat4 rotatebullet = glm::rotate((float)(rotationBullet[i]*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
			Matrices.model *= (translatebullet3*translatebullet2*rotatebullet*translatebullet1);
			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

			draw3DObject(bullet);
		}
	}
	///check colision btw bullet and block
	for(i=1;i<=countBullet;i++)
	{
		cx=-3.45+bulletx[i];
		cy=adjusty[i]+bullety[i];
		w=0.01;
		if(flagbullet[i]==0)
		{
			for(j=1;j<=countblack;j++)
			{
				if(flagblack[i]==0)
				{
					bx=xblack[j];
					by=changeblack[j];
					bw=0.2;
					if((abs(bx-cx)<=bw) && (abs(by-cy)<=bw))
					{
						flagbullet[i]=1;
						flagblack[j]=1;
						//perfect shoot
						score+=2;
						break;

					}



				}
			}
		}
		if(flagbullet[i]==0)
		{
			for(j=1;j<=countred;j++)
			{
				if(flagred[i]==0)
				{
					bx=xred[j];
					by=changered[j];
					bw=0.2;
					if((abs(bx-cx)<=bw) && (abs(by-cy)<=bw))
					{
						flagbullet[i]=1;
						flagred[j]=1;
						score-=1;
						break;
					}
				}



			}
		}
		if(flagbullet[i]==0)
		{
			for(j=1;j<=countgreen;j++)
			{
				if(flaggreen[i]==0)
				{
					bx=xgreen[j];
					by=changegreen[j];
					bw=0.2;
					if((abs(bx-cx)<=bw) && (abs(by-cy)<=bw))
					{
						flagbullet[i]=1;
						flaggreen[j]=1;
						score-=1;
						break;

					}
				}



			}
		}

	}

	// Swap the frame buffers
	glutSwapBuffers ();
	// Increment angles
	float increments = 1;
	//camera_rotation_angle++; // Simulating camera rotation
	triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
	rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}
/* Executed when the program is idle (no I/O activity) */
void idle ()
{
	// OpenGL should never stop drawing
	// can draw the same scene or a modified scene
	int i;
	t++;
	for(i=1;i<=countred;i++)
		changered[i]-=speed;

	for(i=1;i<=countgreen;i++)
		changegreen[i]-=speed;
	for(i=1;i<=countblack;i++)
		changeblack[i]-=speed;
	for(i=1;i<=countBullet;i++)
	{
		bullety[i]+=0.1*sin((rotationBullet[i]*M_PI)/180.0f);
		bulletx[i]+=0.1*cos((rotationBullet[i]*M_PI)/180.0f);
	}
	if(t%50==0)
	{
		i=(rand())%3;
		if(i==0)
			countred++;
		else if(i==1)
			countgreen++;
		else if(i==2)
			countblack++;
	}

	draw (); // drawing same scene
}
/* Initialise glut window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
void initGLUT (int& argc, char** argv, int width, int height)
{
	// Init glut
	glutInit (&argc, argv);

	// Init glut window
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitContextVersion (3, 3); // Init GL 3.3
	glutInitContextFlags (GLUT_CORE_PROFILE); // Use Core profile - older functions are deprecated
	glutInitWindowSize (width, height);
	glutCreateWindow ("Sample OpenGL3.3 Application");

	// Initialize GLEW, Needed in Core profile
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		cout << "Error: Failed to initialise GLEW : "<< glewGetErrorString(err) << endl;
		exit (1);
	}

	// register glut callbacks
	glutKeyboardFunc (keyboard);
	glutKeyboardUpFunc (keyboardUp);

	glutSpecialFunc (keyboardSpecialDown);
	glutSpecialUpFunc (keyboardSpecialUp);
	glutMouseFunc (mouseClick);
	glutMotionFunc (mouseMotion);

	glutReshapeFunc (reshapeWindow);
	//glutTimerFunc(2000,timer,0);
	glutDisplayFunc (draw); // function to draw when active
	glutIdleFunc (idle); // function to draw when idle (no I/O activity)

	//	glutTimerFunc(2000,timer,0);
	glutIgnoreKeyRepeat (true); // Ignore keys held down
}

/* Process menu option 'op' */
void menu(int op)
{
	switch(op)
	{
		case 'Q':
		case 'q':
			exit(0);
	}
}

void addGLUTMenus ()
{
	// create sub menus
	int subMenu = glutCreateMenu (menu);
	glutAddMenuEntry ("Do Nothing", 0);
	glutAddMenuEntry ("Really Quit", 'q');

	// create main "middle click" menu
	glutCreateMenu (menu);
	glutAddSubMenu ("Sub Menu", subMenu);
	glutAddMenuEntry ("Quit", 'q');
	glutAttachMenu (GLUT_MIDDLE_BUTTON);
}


/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (int width, int height)
{
	// Create the models
	createbucket1(); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createGun1();
	createGun2();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (width, height);

	// Background color of the scene
	glClearColor (1.0f, 1.0f, 1.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	createred_block();
	creategreen_block();
	createblack_block();
	createbucket2 ();
	createmirror1();
	createmirror2();
	createmirror3();
	createmirror4();
	createBullet();
	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)

{

    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
    string s="./example.mp3";
    /* open the file and get the decoding format */
    mpg123_open(mh,&s[0]);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

	pthread_t mythread;
        pthread_create(&mythread, NULL, playsound,(void*)NULL);
	initialise();
	int width = 800;

	int height = 600;

	initGLUT (argc, argv, width, height);
	last_update_time=glutGet(GLUT_ELAPSED_TIME);

	addGLUTMenus ();

	initGL (width, height);

	glutMainLoop ();
free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

	return 0;
}
