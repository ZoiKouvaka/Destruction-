

//*********************************
#define _CRT_SECURE_NO_WARNINGS

#define STB_IMAGE_IMPLEMENTATION

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include "stb_image.h"
#include <time.h>
#include <cstdlib>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
using namespace std;

float currentZoom = 45.0f; //initialization of currentZoom. Acts like a counter and used to zoom in and zoom out
glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() {
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
	return ProjectionMatrix;
}

float* downList = (float*) malloc(sizeof(float)); // array that keeps the z coordinate of every ball
glm::mat4* modelList = (glm::mat4*) malloc(sizeof(glm::mat4)); //array that keeps the model of every ball
glm::mat4* craterList = (glm::mat4*)malloc(sizeof(glm::mat4)); // array that keeps the model of every crater
float fast = 0.01; //speed of the balls
//*******************************************************************************
// Η παρακάτω συνάρτηση είναι από http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
// H συνάρτηση loadOBJ φορτώνει ένα αντικείμενο από το obj αρχείο του και φορτώνει και normals kai uv συντεταγμένες
// Την χρησιμοποιείτε όπως το παράδειγμα που έχω στην main
// Very, VERY simple OBJ loader.
// 

bool loadOBJ(
	const char* path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec2>& out_uvs,
	std::vector<glm::vec3>& out_normals
) {
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}
	fclose(file);
	return true;
}

void sphereMaker(int xrand, int yrand, int numOfSpheres) //function that takes as parameter the x,y axis of the ball and the number of balls and create the models of balls and craters 
{
	downList = (float*)realloc(downList, (numOfSpheres * sizeof(float))); //when a new ball is created, the size of downList must be increased to keep the coordinate of the new ball
	modelList = (glm::mat4*)realloc(modelList, (numOfSpheres * sizeof(glm::mat4))); //when a new ball is created, the size of modelList must be increased to keep the model of the new ball
	craterList= (glm::mat4*)realloc(modelList, (numOfSpheres * sizeof(glm::mat4))); //when a new ball is created, the size of craterList must be increased to keep the  model of the new crater
	glm::mat4 newModel = glm::translate(glm::mat4(1.0f), glm::vec3(xrand, yrand, 20)); //move the ball to the random x and y axis and z=20
	downList[numOfSpheres - 1] = 20; //all balls first are in z=20
	modelList[numOfSpheres - 1] = newModel; //all balls first are moved in the random x,y coordinates
	int divCratx = xrand / 5; 
	int divCraty = yrand / 5;
	int craterx = divCratx * 5; //computation of coordinate x of crater 
	int cratery = divCraty * 5; //computation of coordinate y of crater 
	if (craterx == 100) //if x coordinate is in the edge of grid
	{
		craterx = 95;
	}
	if (yrand == 100) //if y coordinate is in the edge of grid
	{
		cratery = 95;
	}
	glm::mat4 newCraterModel = glm::translate(glm::mat4(1.0f), glm::vec3(craterx, cratery, 0.0f)); //move the crater on the right square
	craterList[numOfSpheres - 1] = newCraterModel; //all craters are where their ball fell
}

//************************************
// Η LoadShaders είναι black box για σας
//************************************
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}


void camera_function()
{
	float zTurn = 0;//the angle of z rotation
	float xTurn = 0;//the angle of x rotation

	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) //if <<X>> is pressed rotation at x axis
	{
		xTurn += 0.001;
		ViewMatrix = glm::rotate(ViewMatrix, xTurn, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) //if <<W>> is pressed rotation at x axis
	{
		xTurn -= 0.001;
		ViewMatrix = glm::rotate(ViewMatrix, xTurn, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) //if <<Q>> is pressed rotation at z axis
	{
		zTurn += 0.001;
		ViewMatrix = glm::rotate(ViewMatrix, zTurn, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) //if <<Z>> is pressed rotation at z axis
	{
		zTurn -= 0.001;
		ViewMatrix = glm::rotate(ViewMatrix, zTurn, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) //if <<+>> is pressed zoom out
	{
		currentZoom -= 0.01;
	}
	if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) //if <<->> is pressed zoom in
	{
		currentZoom += 0.01;
	}
	ProjectionMatrix = glm::perspective(glm::radians(currentZoom), 4.0f / 4.0f, 0.1f, 1000.0f); //new ProjectionMatrix

}


int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1000, 10000, u8"Εργασία 1Γ –Καταστροφή", NULL, NULL); //change the title of the window


	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	 
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); //set background to black
		
	GLuint VertexArrayID; // VertexArray of the grid
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	GLuint VertexArrayID1; //VertexArray of the ball
	glGenVertexArrays(1, &VertexArrayID1);
	glBindVertexArray(VertexArrayID1);
	GLuint VertexArrayID2; //VertexArray of the crater
	glGenVertexArrays(1, &VertexArrayID2);
	glBindVertexArray(VertexArrayID2);
	// Create and compile our GLSL program from the shaders
	
	GLuint programID = LoadShaders("ProjCVertexShader.vertexshader", "ProjCFragmentShader.fragmentshader"); //load shaders


	GLuint MatrixID = glGetUniformLocation(programID, "MVP");// Get a handle for the "MVP" uniform

	int width, height, nrChannels; // width,height,nrChannels of grid
	unsigned char* data = stbi_load("ground1.jpg", &width, &height, &nrChannels, 0); //load the texture of grid
	if (data)
	{

	}
	else
	{
		std::cout << "Failed to load texture" << std::endl; //error loading the texture
	}
	int width1, height1, nrChannels1;  // width,height,nrChannels of ball
	unsigned char* data1 = stbi_load("fire.jpg", &width1, &height1, &nrChannels1, 0); //load the texture of ball
	if (data1)
	{

	}
	else
	{
		std::cout << "Failed to load texture" << std::endl; //error loading the texture
	}
	int width2, height2, nrChannels2; // width,height,nrChannels of crater
	unsigned char* data2 = stbi_load("crater2.jpg", &width2, &height2, &nrChannels2, 0); //load the texture of crater
	if (data2)
	{

	}
	else
	{
		std::cout << "Failed to load texture" << std::endl; //error loading the texture
	}
	GLuint textureID; //texture for grid
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Get a handle for the "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	// Read the .obj file of grid
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	bool res = loadOBJ("grid.obj", vertices, uvs, normals); //load .obj file of grid
	//------------------------------------
	GLuint texture1; //texture for ball
	glGenTextures(1, &texture1);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, texture1);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Get a handle for the "myTextureSampler1" uniform
	GLuint Texture1 = glGetUniformLocation(programID, "myTextureSampler1");
	// Read our .obj file of ball
	std::vector<glm::vec3> vertices1;
	std::vector<glm::vec3> normals1;
	std::vector<glm::vec2> uvs1;
	bool res1 = loadOBJ("ball.obj", vertices1, uvs1, normals1); //load .obj file of ball
	//----------------------------------------------------------------
	GLuint texture2; //texture for crater
	glGenTextures(1, &texture2);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, texture2);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, data2);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Get a handle for the "myTextureSampler2" uniform
	GLuint Texture2 = glGetUniformLocation(programID, "myTextureSampler2");
	// Read our .obj file of crater
	std::vector<glm::vec3> vertices2;
	std::vector<glm::vec3> normals2;
	std::vector<glm::vec2> uvs2;
	bool res2 = loadOBJ("grid2.obj", vertices2, uvs2, normals2); //load .obj file of crater
	// Projection matrix : 30Â° Field of View, 4:4 ratio, display range : 0.1 unit <-> 1000 units
	glm::mat4 Projection = glm::perspective(glm::radians(30.0f), 4.0f / 4.0f, 0.1f, 1000.0f);
	// Camera matrix
	glm::mat4 View = glm::lookAt(
		glm::vec3(-30, -30,30), // Camera is at (-30,-30,30)
		glm::vec3(50, 50, 0), // and looks at the the center of grid
		glm::vec3(0, 0, 1)  // (set to 0,0,1)
	);
	ViewMatrix = View;
	ProjectionMatrix = Projection;
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model1 = glm::mat4(1.0f);
	glm::mat4 Model2 = glm::mat4(1.0f);
	glm::mat4 Model3 = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP = Projection * View * Model1;

	// Load it into a VBO
	GLuint vertexbuffer; // vertexbuffer for grid
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer; //uvbuffer for grid
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
	//--------------------------------------------------------------------------------------
	GLuint vertexbuffer1;  // vertexbuffer for ball
	glGenBuffers(1, &vertexbuffer1);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer1);
	glBufferData(GL_ARRAY_BUFFER, vertices1.size() * sizeof(glm::vec3), &vertices1[0], GL_STATIC_DRAW);

	GLuint uvbuffer1; //uvbuffer for ball
	glGenBuffers(1, &uvbuffer1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer1);
	glBufferData(GL_ARRAY_BUFFER, uvs1.size() * sizeof(glm::vec2), &uvs1[0], GL_STATIC_DRAW);
	//------------------------------------------------------------------------------------
	GLuint vertexbuffer2; // vertexbuffer for crater
	glGenBuffers(1, &vertexbuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
	glBufferData(GL_ARRAY_BUFFER, vertices2.size() * sizeof(glm::vec3), &vertices2[0], GL_STATIC_DRAW);

	GLuint uvbuffer2;  //uvbuffer for crater
	glGenBuffers(1, &uvbuffer2);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer2);
	glBufferData(GL_ARRAY_BUFFER, uvs2.size() * sizeof(glm::vec2), &uvs2[0], GL_STATIC_DRAW);
	srand(time(0)); 
	


	int numOfSpheres = 0; //number of balls
	int numOfCraters = 0; //number of craters
	int loopCounter = 0; //number of loops remaining for being able to create a ball pressing "b"
	do {

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Use our shader
		glUseProgram(programID);
		//-------------------GRID--------------------------------------------
		camera_function(); //we call camera_function
		MVP = getProjectionMatrix() * getViewMatrix()*Model1; //create MVP
	    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);
		glEnableVertexAttribArray(0); // 1rst attribute buffer : vertices
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);
		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size()); //draw the grid
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//-------------------END GRID--------------------------------------------
		//-------------------BALL--------------------------------------------
		


		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && loopCounter==0) { //if b is pressed and waited 100 loops(or 1st ever ball created) then create new ball
			loopCounter = 100; //ball was created; now 100 loops are remaining for possible next one
			numOfSpheres++; //incease number of balls
			int xrand = rand() % 101; //find the random x coordinate
			int yrand = rand() % 101; //find the random y coordinate
			sphereMaker(xrand, yrand, numOfSpheres); //call the sphereMaker to create the balls and craters
		} 

		for (int i = 0; i < numOfSpheres; i++) //loop to move the balls based on their coordinates 
		{
			if (downList[i] >= 0) //if z coordinate must be >=0
			{
				if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) { //if u is pressed the ball moves with greater speed
					fast = fast + 0.0001; //speed of ball is increased by 0.0001
				}
				else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && fast>0.0001 && downList[i]>0.0001) { //if p is pressed the ball moves with lower speed
					fast = fast - 0.0001; //speed of ball is descreased by 0.0001
				}
				downList[i] -= fast; //we descrease the z coordinate of the ball to make the ball drop
			}
			Model2 = modelList[i]; //get the model of the current ball
			Model2 = glm::translate(Model2, glm::vec3(0, 0, downList[i]));  //and move the ball downwards
			camera_function(); //we call camera_function
			MVP= getProjectionMatrix() * getViewMatrix() * Model2; //create MVP
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Set our "myTextureSampler1" sampler to use Texture Unit 0
			glUniform1i(Texture1, 0);
			glEnableVertexAttribArray(0); // 1rst attribute buffer : vertices
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer1);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);
			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer1);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);
			if (downList[i] > 0) { //if ball is above the ground or on the ground
				glDrawArrays(GL_TRIANGLES, 0, vertices1.size()); //draw the ball
			}
			else if(downList[i] < 0 && downList[i] != -2000) { //else if the ball has just landed and hasn't created a crater
				numOfCraters++; //increase number of craters
				downList[i] = -2000; // -2000 tells that ball has already created its crater 

			}
		}
		//-------------------END BALL--------------------------------------------
		//-------------------CRATER--------------------------------------------
		for (int i = 0; i < numOfCraters; i++) //loop to draw the craters
		{
			Model3 = craterList[i];  //get the model of the current crater
			camera_function(); //we call camera_function
			MVP = getProjectionMatrix() * getViewMatrix() * Model3; //create MVP

			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Set our "myTextureSampler2" sampler to use Texture Unit 0
			glUniform1i(Texture2, 0);
			glEnableVertexAttribArray(0); // 1rst attribute buffer : vertices
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);
			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer2);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);
			glDrawArrays(GL_TRIANGLES, 0, vertices2.size()); //draw the crater

		}
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//-------------------END CRATER--------------------------------------------
		// Swap buffers
		glfwSwapBuffers(window); // Swap buffers
		glfwPollEvents();
		
		if (loopCounter > 0) 
		{
			loopCounter--; //descrease the loop counter 
		}
		
	} // Check if the space key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteBuffers(1, &vertexbuffer1);
	glDeleteBuffers(1, &uvbuffer1);
	glDeleteVertexArrays(1, &VertexArrayID1);
	glDeleteBuffers(1, &vertexbuffer2);
	glDeleteBuffers(1, &uvbuffer2);
	glDeleteVertexArrays(1, &VertexArrayID2);
	glDeleteProgram(programID);
	glDeleteTextures(1, &textureID);
	glDeleteTextures(1, &texture1);
	glDeleteTextures(1, &texture2);
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
	
}

