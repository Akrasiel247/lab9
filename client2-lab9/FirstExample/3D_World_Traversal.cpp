#include <windows.networking.sockets.h>
#pragma comment(lib, "ws2_32.lib")

#include <vector>
#include <iostream> 
#include <algorithm>
#include "vgl.h"
#include "LoadShaders.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\rotate_vector.hpp"
#include "..\SOIL\src\SOIL.h"

using namespace std;

//Used to represent a Actor
struct Actor
{
	int id;	//Unique ID of this actor
	float x_pos, y_pos;	//Position of a Actor
	bool isAlive;	//Still alive?
	bool isBullet;	//Is this actor a bullet or a player?
	bool touchedActor;	//If this Actor is collided with other Actor
	glm::vec2 direction;	//Motion direction
};

float wheel_rotation = 0.0f;
float Actor_velocity = 3.0f;

//We use a container to keep the data corresponding to Actors
vector<Actor> sceneGraph;

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer };
enum Attrib_IDs { vPosition = 0 };

const GLint NumBuffers = 2;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint location;
GLuint cam_mat_location;
GLuint proj_mat_location;
GLuint texture[2];	//Array of pointers to textrure data in VRAM. We use two textures in this example.

const GLuint NumVertices = 28;

//Height of camera (Actor) from the level
float height = 0.8f;

int CLIENT_ID = 2;
Actor v;

//Actor motion speed for movement and pitch/yaw
float travel_speed = 60.0f;		//Motion speed
float mouse_sensitivity = 0.01f;	//Pitch/Yaw speed

//Used for tracking mouse cursor position on screen
int x0 = 0;
int y_0 = 0;

//Transformation matrices and camera vectors
glm::mat4 model_view;
glm::vec3 unit_z_vector = glm::vec3(0, 0, 1);	//Assigning a meaningful name to (0,0,1) :-)
glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, height);
glm::vec3 forward_vector = glm::vec3(1, 1, 0);	//Forward vector is parallel to the level at all times (No pitch)

//The direction which the camera is looking, at any instance
glm::vec3 looking_dir_vector = glm::vec3(1, 1, 0);
glm::vec3 up_vector = unit_z_vector;
glm::vec3 side_vector = glm::cross(up_vector, forward_vector);

//Used to measure time between two frames
float oldTimeSinceStart = 0;
float deltaTime;

//Creating and rendering bunch of objects on the scene to interact with
const int Num_Obstacles = 10;
float obstacle_data[Num_Obstacles][3];

//only increase score when the either the bullet or Actor is colliding
bool Increasescore = false;
//score counter
int score = 0;
//flag to know if Actor is alive
bool ActorAlive = true;
//counting time since start to spawn Actors
int oldgluttime = -1;


//Function Signatures
void spawnActor();
void spawnActor(Actor v);
void drawActor(float scale, glm::vec2 direction);
void renderActors();
void update();
void sendToServer();
void serverResponse();
void shoot();



void renderActors() {

	for (int i = 0; i < sceneGraph.size(); i++) {

		Actor v = sceneGraph[i];

		if (v.isAlive && !v.isBullet)
		{
			model_view = glm::translate(model_view, glm::vec3(v.x_pos, v.y_pos, 0));

			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

			drawActor(1.0, glm::normalize(v.direction));

			model_view = glm::mat4(1.0);

			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

		}
		// when Actor is colliding with a bullet
		else if (v.isAlive && v.isBullet)
		{
			model_view = glm::translate(model_view, glm::vec3(v.x_pos, v.y_pos, 0));

			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

			drawActor(0.4, glm::normalize(v.direction));

			model_view = glm::mat4(1.0);

			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

		}
	}
}

//Helper function to generate a random float number within a range
float randomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

// inititializing buffers, coordinates, setting up pipeline, etc.
void initializeGraphics(void)
{
	glEnable(GL_DEPTH_TEST);

	//Normalizing all vectors
	up_vector = glm::normalize(up_vector);
	forward_vector = glm::normalize(forward_vector);
	looking_dir_vector = glm::normalize(looking_dir_vector);
	side_vector = glm::normalize(side_vector);

	//Randomizing the position and scale of obstacles
	for (int i = 0; i < Num_Obstacles; i++)
	{
		obstacle_data[i][0] = randomFloat(-50, 50); //X
		obstacle_data[i][1] = randomFloat(-50, 50); //Y
		obstacle_data[i][2] = randomFloat(0.1, 10.0); //Scale
	}

	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	GLuint program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	GLfloat vertices[NumVertices][3] = {

		{ -100.0, -100.0, 0.0 }, //Plane to walk on and a sky
		{ 100.0, -100.0, 0.0 },
		{ 100.0, 100.0, 0.0 },
		{ -100.0, 100.0, 0.0 },

		{ -0.45, -0.45 ,-0.45 }, // bottom face
		{ 0.45, -0.45 ,-0.45 },
		{ 0.45, 0.45 ,-0.45 },
		{ -0.45, 0.45 ,-0.45 },

		{ -0.45, -0.45 ,0.45 }, //top face
		{ 0.45, -0.45 ,0.45 },
		{ 0.45, 0.45 ,0.45 },
		{ -0.45, 0.45 ,0.45 },

		{ 0.45, -0.45 , -0.45 }, //left face
		{ 0.45, 0.45 , -0.45 },
		{ 0.45, 0.45 ,0.45 },
		{ 0.45, -0.45 ,0.45 },

		{ -0.45, -0.45, -0.45 }, //right face
		{ -0.45, 0.45 , -0.45 },
		{ -0.45, 0.45 ,0.45 },
		{ -0.45, -0.45 ,0.45 },

		{ -0.45, 0.45 , -0.45 }, //front face
		{ 0.45, 0.45 , -0.45 },
		{ 0.45, 0.45 ,0.45 },
		{ -0.45, 0.45 ,0.45 },

		{ -0.45, -0.45 , -0.45 }, //back face
		{ 0.45, -0.45 , -0.45 },
		{ 0.45, -0.45 ,0.45 },
		{ -0.45, -0.45 ,0.45 },
	};

	//These are the texture coordinates for the second texture
	GLfloat textureCoordinates[28][2] = {
		0.0f, 0.0f,
		200.0f, 0.0f,
		200.0f, 200.0f,
		0.0f, 200.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
	};

	//Creating our texture:
	//This texture is loaded from file. To do this, we use the SOIL (Simple OpenGL Imaging Library) library.
	//When using the SOIL_load_image() function, make sure the you are using correct patrameters, or else, your image will NOT be loaded properly, or will not be loaded at all.
	GLint width1, height1;
	unsigned char* textureData1 = SOIL_load_image("grass.png", &width1, &height1, 0, SOIL_LOAD_RGB);

	GLint width2, height2;
	unsigned char* textureData2 = SOIL_load_image("apple.png", &width2, &height2, 0, SOIL_LOAD_RGB);

	glGenBuffers(2, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindAttribLocation(program, 0, "vPosition");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glBindAttribLocation(program, 1, "vTexCoord");
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1);

	location = glGetUniformLocation(program, "model_matrix");
	cam_mat_location = glGetUniformLocation(program, "camera_matrix");
	proj_mat_location = glGetUniformLocation(program, "projection_matrix");

	///////////////////////TEXTURE SET UP////////////////////////

	//Allocating two buffers in VRAM
	glGenTextures(2, texture);

	//First Texture: 
	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData1);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//And now, second texture: 
	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData2);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//////////////////////////////////////////////////////////////
}

//Draws the actor on the scene. 
//This function does actually draw the graphics.
void drawActor(float scale, glm::vec2 direction)
{
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	model_view = glm::translate(model_view, glm::vec3(0.0f, 0.0f, 0.45f));
	model_view = glm::rotate(model_view, atan(direction.y / direction.x), unit_z_vector);
	model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	glDrawArrays(GL_QUADS, 4, 24);
}

//Adds an actor to the scene graph (the vector). The location of this actor is random. 
//Note: This function does NOT draw the actor on the scene.
void spawnActor()
{
	Actor v;
	v.x_pos = randomFloat(-50, 50);
	v.y_pos = randomFloat(-50, 50);
	v.isAlive = TRUE;
	v.touchedActor = FALSE;
	v.isBullet = FALSE;
	sceneGraph.push_back(v);
}

//Adds an actor to the scene graph (the vector).
//Note: This function does NOT draw the actor on the scene, instead, it adds the actor to the scenegraph
void spawnActor(Actor v)
{
	sceneGraph.push_back(v);
}

// display function
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	model_view = glm::mat4(1.0);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

	//The 3D point in space that the camera is looking
	glm::vec3 look_at = cam_pos + looking_dir_vector;

	glm::mat4 camera_matrix = glm::lookAt(cam_pos, look_at, up_vector);
	glUniformMatrix4fv(cam_mat_location, 1, GL_FALSE, &camera_matrix[0][0]);

	glm::mat4 proj_matrix = glm::frustum(-0.01f, +0.01f, -0.01f, +0.01f, 0.01f, 100.0f);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &proj_matrix[0][0]);

	//Select the first texture (grass.png) when drawing the first geometry (floor)
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glDrawArrays(GL_QUADS, 0, 4);

	//Draw a column in the middle of the scene (0, 0, 0)
	//*****************************************************
	model_view = glm::scale(model_view, glm::vec3(0.1, 0.1, 10));
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	glDrawArrays(GL_QUADS, 4, 24);
	model_view = glm::mat4(1.0);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	//*****************************************************

	renderActors();

	glFlush();
}


void keyboard(unsigned char key, int x, int y)
{
	if (ActorAlive)
	{
		if (key == 'a')
		{
			//Moving camera along opposit direction of side vector
			cam_pos += side_vector * travel_speed * deltaTime;
		}
		if (key == 'd')
		{
			//Moving camera along side vector
			cam_pos -= side_vector * travel_speed * deltaTime;
		}
		if (key == 'w')
		{
			//Moving camera along forward vector. To be more realistic, we use X=V.T equation in physics
			cam_pos += forward_vector * travel_speed * deltaTime;
		}
		if (key == 's')
		{
			//Moving camera along backward (negative forward) vector. To be more realistic, we use X=V.T equation in physics
			cam_pos -= forward_vector * travel_speed * deltaTime;
		}
		//press F to shoot Actor
		if (key == 'f')
		{
			shoot();
		}
	}
}

//Controlling Pitch with vertical mouse movement
void mouse(int x, int y)
{
	if (ActorAlive)
	{
		int delta_x = x - x0;

		forward_vector = glm::rotate(forward_vector, -delta_x * mouse_sensitivity, unit_z_vector);

		looking_dir_vector = glm::rotate(looking_dir_vector, -delta_x * mouse_sensitivity, unit_z_vector);

		side_vector = glm::rotate(side_vector, -delta_x * mouse_sensitivity, unit_z_vector);

		up_vector = glm::rotate(up_vector, -delta_x * mouse_sensitivity, unit_z_vector);

		x0 = x;

		int delta_y = y - y_0;

		glm::vec3 tmp_up_vec = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);

		glm::vec3 tmp_looking_dir = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);

		GLfloat dot_product = glm::dot(tmp_looking_dir, forward_vector);

		if (dot_product > 0)
		{
			up_vector = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
			looking_dir_vector = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);
		}

		y_0 = y;
	}
}

void idle()
{
	//Calculate the delta time between two frames
	wheel_rotation += 0.01f;
	float timeSinceStart = (float)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

	//create Actor at every 2 seconds
	/*if ((int)timeSinceStart % 2 == 0 && oldgluttime != (int)timeSinceStart)
	{
		spawnActor();
		oldgluttime = (int)timeSinceStart;
	}*/
	deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;
	update();
	glutPostRedisplay();
}



/*************************WARNING!!!!!**************************/
/*************************WARNING!!!!!**************************/
/***************DO NOT TOUCH THE CODE ABOVE*********************/
/***************DO NOT TOUCH THE CODE ABOVE*********************/
/*************************WARNING!!!!!**************************/
/*************************WARNING!!!!!**************************/


/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/
/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/
/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/
/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“ YOUR CODE GOES BELOW â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/
/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/
/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/
/*â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“â†“*/

/***************************************************************/
/***************************************************************/
/**************************Networking Code**********************/
/***************************************************************/
/***************************************************************/

//starts Winsock DLLs
WSADATA wsaData;
SOCKET ClientSocket;


//generates a new Actor, called when F is pressed
void shoot()
{
	Actor v;
	v.x_pos = cam_pos.x + 2 * forward_vector.x;
	v.y_pos = cam_pos.y + 2 * forward_vector.y;
	v.isAlive = TRUE;
	v.isBullet = TRUE;
	v.touchedActor = FALSE;
	v.id = -1;
	v.direction = glm::vec2(forward_vector.x, forward_vector.y);
	//Send v to Server

	char* TxBuffer = (char*)malloc(sizeof(v));
	memcpy(TxBuffer, &(v), sizeof(v));
	send(ClientSocket, TxBuffer, sizeof(v), 0);
}

//Returns the scene-graph
vector<Actor>& getSceneGraph() {
	return sceneGraph;
}

int initializeNetwork()
{
	v.id = CLIENT_ID;
	v.touchedActor = FALSE;
	v.isBullet = FALSE;
	v.id = CLIENT_ID;
	v.isAlive = TRUE;

	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		return -1;
	}

	//initializes socket. SOCK_STREAM: TCP
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		WSACleanup();
		return -1;
	}

	//Connect socket to specified server
	sockaddr_in SvrAddr;

	SvrAddr.sin_family = AF_INET;						//Address family type itnernet

	SvrAddr.sin_port = htons(27000);					//port (host to network conversion)

	SvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	//IP address --> INADDR_ANY

	if ((connect(ClientSocket, (struct sockaddr*) & SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
		closesocket(ClientSocket);
		WSACleanup();
		cout << "Connecting to server failed!!!\n";
		return -1;
	}
	//Add your network initialization code here
	//To be added by astudents

	return 1;

}

// DONT CHANGE THIS FUNCTION
//This function gets called for every frame, say 30 times per second
void update()
{
	sendToServer();
	serverResponse();
}

//later on, this function will be used to receive data from a server
void serverResponse()
{
	//Receive the updated scene-graph from server.
	char Rxbuffer[20000] = {};
	int n = recv(ClientSocket, Rxbuffer, sizeof(Rxbuffer), 0);

	Actor* actorArr = (Actor*)(Rxbuffer);


	int number_of_players = n / sizeof(Actor);

	cout << "Number of players: " << number_of_players << endl;
	if (!ActorAlive) {
		cout << "Player: " << CLIENT_ID << " Game Over!!" << endl;
	}

	vector<Actor> tmp;


	for (int i = 0; i < number_of_players; i++) {

		if (actorArr[i].id != CLIENT_ID && actorArr[i].isAlive == TRUE) {
			tmp.push_back(actorArr[i]);
		}

		if (actorArr[i].id == CLIENT_ID && actorArr[i].isAlive == FALSE) {
			ActorAlive = FALSE;
		}

	}
	sceneGraph = tmp;
}

void sendToServer()
{
	//Create an instance of myself and send to server
	v.x_pos = cam_pos.x;
	v.y_pos = cam_pos.y;
	v.direction = glm::vec2(forward_vector.x, forward_vector.y);

	char* TxBuffer = (char*)malloc(sizeof(v));
	memcpy(TxBuffer, &(v), sizeof(v));
	send(ClientSocket, TxBuffer, sizeof(v), 0);
}


// main
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("Player: 2");
	glewInit();	//Initializes the glew and prepares the drawing pipeline.

	initializeGraphics();

	glutDisplayFunc(display);

	glutKeyboardFunc(keyboard);

	glutIdleFunc(idle);

	glutPassiveMotionFunc(mouse);

	initializeNetwork();

	glutMainLoop();

}
