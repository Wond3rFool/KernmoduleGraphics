#include <iostream>
#include <fstream>

#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "model.h"

int init(GLFWwindow*& window);

void createShaders();
void loadFile(const char* filename, char*& output);
GLuint loadTexture(const char* path, int comp = 0, GLint wrapTypeS = GL_CLAMP_TO_EDGE, GLint wrapTypeT = GL_CLAMP_TO_EDGE);
GLuint loadCubemap(std::vector<string> filenames, int comp = 0);
void createProgram(GLuint& programID, const char* vertex, const char* fragment);


void renderSkybox(glm::mat4 view, glm::mat4 projection);
void renderStarBox(glm::mat4 view, glm::mat4 projection);
void renderPlanet(glm::mat4 view, glm::mat4 projection);
void renderMoon(glm::mat4 parentWorld, glm::mat4 view, glm::mat4 projection);
void createGeometry(GLuint& vao, GLuint &EBO, int& size, int& numIndices);
void renderTerrain(glm::mat4 view, glm::mat4 projection);
unsigned int GeneratePlane(const char* heightmap,unsigned char* &data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID);
void renderModel(Model* model, glm::mat4 view, glm::mat4 projection, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);

void createFrameBuffer(int width, int height, unsigned& frameBufferID, unsigned int& colorBufferID, unsigned int& depthBufferID, unsigned int& depthTextureID);
void renderToBuffer(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int shader);
void renderBlur(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int depthBufferFrom, glm::mat4 viewInverse, glm::mat4 previousView);
void renderQuad();

void handleInput(GLFWwindow* window, float deltaTime);



bool keys[1024];

GLuint simpleProgram, skyProgram, terrainProgram, imgProgram, blurProgram;
GLuint chrabbProgram, modelProgram, starProgram, planetProgram, moonProgram;

const int WIDTH = 1280, HEIGHT = 720;

// world data
glm::vec3 lightDirection = glm::normalize(glm::vec3(1, 0, 0));
glm::vec3 cameraPosition = glm::vec3(1, 1, 1), cameraForward(0,0,1),cameraUp(0,1,0);


GLuint boxVAO, boxEBO;
int boxSize, boxIndexCount;

float lastX, lastY;
bool firstMouse = true;
float camYaw, camPitch;
glm::quat camQuat;

//terrain data
GLuint terrainVAO, terrainIndexCount, heightmapID, heightNormalID;
unsigned char* heightmapTexture;
unsigned int dirtID, sandID, grassID, rockID, snowID;

GLuint dirt, sand, grass, rock, snow, cubeMap;
GLuint day, night, clouds, moon;

Model* backpack;
Model* sphere;
Model* cottage;

int main()
{
	static double previousT = 0;

	//initialize window init.
	GLFWwindow* window;
	int res = init(window);
	if (res != 0) return res;

	createShaders();
	createGeometry(boxVAO, boxEBO, boxSize, boxIndexCount);

	terrainVAO = GeneratePlane("textures/Heightmap2.png", heightmapTexture, GL_RGBA, 4, 250.0f, 5.0f, terrainIndexCount, heightmapID);
	heightNormalID = loadTexture("textures/heightnormal.png");

	GLuint boxTex = loadTexture("textures/container2.png");
	GLuint boxNormal = loadTexture("textures/container2_normal.png");

	dirt = loadTexture("textures/dirt.jpg");
	sand = loadTexture("textures/sand.jpg");
	grass = loadTexture("textures/grass.png", 4);
	rock = loadTexture("textures/rock.jpg");
	snow = loadTexture("textures/snow.jpg");

	day = loadTexture("textures/day.jpg");
	night = loadTexture("textures/night.jpg");
	clouds = loadTexture("textures/clouds.jpg", 0, GL_REPEAT, GL_CLAMP_TO_EDGE);
	moon = loadTexture("textures/2k_moon.jpg");

	std::vector<string> filenames =
	{
		"textures/space-cubemap/right.png",
		"textures/space-cubemap/left.png",
		"textures/space-cubemap/top.png",
		"textures/space-cubemap/bottom.png",
		"textures/space-cubemap/front.png",
		"textures/space-cubemap/back.png"
	};

	cubeMap = loadCubemap(filenames);

	backpack = new Model("models/backpack/backpack.obj");
	sphere = new Model("models/sphere/uv_sphere.obj");
	cottage = new Model("models/cottage/cottage.obj", false);

	glViewport(0, 0, WIDTH, HEIGHT);

	/*needed for simple cube
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::rotate(world, glm::radians(45.0f), glm::vec3(0,1,0));
	world = glm::scale(world, glm::vec3(1, 1, 1));
	world = glm::translate(world, glm::vec3(0, 0, 0));

	// Set ambient light color and intensity
	glm::vec3 ambientColor = glm::vec3(0.0f, 0.0f, 0.3f);
	float ambientIntensity = 0.3f;
	*/

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.1f, 10000.0f);
	glm::mat4 previousViewProjectionMatrix = glm::mat4(1.0f);
	

	unsigned int motionBlurFBO, motionBlurColorBuffer, motionBlurDepthBuffer, motionBlurDepthTexture;
	unsigned int motionBlurFBO2, motionBlurColorBuffer2, motionBlurDepthBuffer2, motionBlurDepthTexture2;
	unsigned int previousScene, previousColor, previousDepth, previousDepthText;
	unsigned int currentScene, currentColor, currentDepth, curentDepthTex;
	//unsigned int sceneColorBuffer;
	createFrameBuffer(WIDTH, HEIGHT, motionBlurFBO, motionBlurColorBuffer, motionBlurDepthBuffer, motionBlurDepthTexture);
	createFrameBuffer(WIDTH, HEIGHT, motionBlurFBO2, motionBlurColorBuffer2, motionBlurDepthBuffer2, motionBlurDepthTexture2);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		double t = glfwGetTime();
		float deltaTime = t - previousT;
		previousT = t;

		//input
		handleInput(window, deltaTime);
		glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraForward, cameraUp);
		
		glm::mat4 currentViewProjectionMatrix = projection * view;
		 // Store the previous frame's view-projection matrix

		// Calculate the inverse of the current view-projection matrix for later use
		glm::mat4 inverseViewProjectionMatrix = glm::inverse(currentViewProjectionMatrix);

		// Before rendering each frame, update the previousViewProjectionMatrix
		

		glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//renderSkybox(view, projection);
		renderStarBox(view, projection);
		renderPlanet(view, projection);
		renderTerrain(view, projection);
		renderModel(backpack, view, projection, glm::vec3(100,100,100), glm::vec3(0, t, 0), glm::vec3(10, 10, 10));
		renderModel(cottage, view, projection, glm::vec3(1000,100,1000), glm::vec3(0, 0, 0), glm::vec3(100, 100, 100));

		/* Simple cube, if want fix have a lightPosition. Look at cameraPosition for example.
		float currentTime = glfwGetTime();
		glUseProgram(simpleProgram);
		glUniform1f(glGetUniformLocation(simpleProgram, "time"), currentTime);

		glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
		glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glUniform3fv(glGetUniformLocation(simpleProgram, "ambientColor"), 1, glm::value_ptr(ambientColor));
		glUniform1f(glGetUniformLocation(simpleProgram, "ambientIntensity"), ambientIntensity);

		glUniform3fv(glGetUniformLocation(simpleProgram, "lightPosition"), 1, glm::value_ptr(lightPosition));
		glUniform3fv(glGetUniformLocation(simpleProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
		

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, boxTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, boxNormal);

		glBindVertexArray(triangleVAO);
		//glDrawArrays(GL_TRIANGLES, 0, triangleSize);
		glDrawElements(GL_TRIANGLES, triangleIndixCount, GL_UNSIGNED_INT, 0);
		*/
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		renderBlur(motionBlurFBO2, motionBlurColorBuffer, motionBlurDepthTexture, inverseViewProjectionMatrix, previousViewProjectionMatrix);

		renderToBuffer(0, motionBlurColorBuffer2, imgProgram);

		previousViewProjectionMatrix = currentViewProjectionMatrix;

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	std::cout << "Message here";

	glfwTerminate();
	return 0;
}
void renderSkybox(glm::mat4 view, glm::mat4 projection)
{
	//OpenGL setup
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH);
	glDepthFunc(GL_ALWAYS);

	glUseProgram(skyProgram);

	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, cameraPosition);
	world = glm::scale(world, glm::vec3(100, 100, 100));

	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(skyProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(skyProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	//rendering
	glBindVertexArray(boxVAO);
	glDrawElements(GL_TRIANGLES, boxIndexCount, GL_UNSIGNED_INT, 0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

void renderStarBox(glm::mat4 view, glm::mat4 projection)
{
	//OpenGL setup
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH);
	glDepthFunc(GL_ALWAYS);

	glUseProgram(starProgram);

	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, cameraPosition);
	world = glm::scale(world, glm::vec3(100, 100, 100));

	glUniformMatrix4fv(glGetUniformLocation(starProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(starProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(starProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(starProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(starProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	//rendering
	glBindVertexArray(boxVAO);
	glDrawElements(GL_TRIANGLES, boxIndexCount, GL_UNSIGNED_INT, 0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

void renderTerrain(glm::mat4 view, glm::mat4 projection)
{
	//glDepthFunc(GL_ALWAYS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(terrainProgram);

	glm::mat4 world = glm::mat4(1.0f);

	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//float t = glfwGetTime();
	//lightDirection = glm::normalize(glm::vec3(sin(t), -0.5f, glm::cos(t)));
	glUniform3fv(glGetUniformLocation(terrainProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(terrainProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, heightmapID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, heightNormalID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, dirt);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, sand);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, grass);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, rock);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, snow);

	//Rendering
	glBindVertexArray(terrainVAO);
	glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);
}

void handleInput(GLFWwindow* window, float deltaTime)
{
	static bool w, s, a, d, space, ctrl;
	static double cursorX = -1, cursorY = -1, lastCursorX, lastCursorY;
	static float pitch, yaw;
	static float speed = 1000.0f;

	float sensitivity = 50.0f * deltaTime;
	float step = speed * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)				w = true;
	else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE)		w = false;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)				s = true;
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE)		s = false;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)				a = true;
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE)		a = false;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)				d = true;
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)		d = false;

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)				space = true;
	else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)		space = false;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)		ctrl = true;
	else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE)	ctrl = false;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (cursorX == -1) {
		glfwGetCursorPos(window, &cursorX, &cursorY);
	}

	lastCursorX = cursorX;
	lastCursorY = cursorY;
	glfwGetCursorPos(window, &cursorX, &cursorY);

	glm::vec2 mouseDelta(cursorX - lastCursorX, cursorY - lastCursorY);

	yaw -= mouseDelta.x * sensitivity;
	pitch += mouseDelta.y * sensitivity;

	if (pitch < -90.0f) pitch = -90.0f;
	else if (pitch > 90.0f) pitch = 90.0f;

	if (yaw < -180.0f) yaw += 360;
	else if (yaw > 180.0f) yaw -= 360;

	glm::vec3 euler(glm::radians(pitch), glm::radians(yaw), 0);
	glm::quat q(euler);

	// update camera position / forward & up
	glm::vec3 translation(0, 0, 0);

	//implement movement
	if (w) translation.z += speed * deltaTime;
	if (s) translation.z -= speed * deltaTime;
	if (a) translation.x += speed * deltaTime;
	if (d) translation.x -= speed * deltaTime;
	if (space) translation.y += speed * deltaTime;
	if (ctrl) translation.y -= speed * deltaTime;

	cameraPosition += q * translation;

	cameraUp =  q * glm::vec3(0, 1, 0);
	cameraForward = q * glm::vec3(0, 0, 1);
}

int init(GLFWwindow*& window)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	window = glfwCreateWindow(WIDTH, HEIGHT, "Kernmodule", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	return 0;
}

unsigned int GeneratePlane(const char* heightmap, unsigned char*& data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID) {
	int width, height, channels;
	data = nullptr;
	if (heightmap != nullptr)
	{
		data = stbi_load(heightmap, &width, &height, &channels, comp);
		if (data)
		{
			glGenTextures(1, &heightmapID);
			glBindTexture(GL_TEXTURE_2D, heightmapID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	int stride = 8;
	float* vertices = new float[(width * height) * stride];
	unsigned int* indices = new unsigned int[(width - 1) * (height - 1) * 6];

	int index = 0;
	for (int i = 0; i < (width * height); i++)
	{
		int x = i % width;
		int z = i / width;

		float texHeight = (float)data[i * comp];

		vertices[index++] = x * xzScale;
		vertices[index++] = (texHeight / 255.0f) * hScale;
		vertices[index++] = z * xzScale;

		vertices[index++] = 0;
		vertices[index++] = 1;
		vertices[index++] = 0;

		vertices[index++] = x / (float)width;
		vertices[index++] = z / (float)height;
	}

	// OPTIONAL TODO: Calculate normal
	// TODO: Set normal

	index = 0;
	for (int i = 0; i < (width - 1) * (height - 1); i++)
	{
		int x = i % (width - 1);
		int z = i / (width - 1);

		int vertex = z * width + x;

		indices[index++] = vertex;
		indices[index++] = vertex + width;
		indices[index++] = vertex + width + 1;

		indices[index++] = vertex;
		indices[index++] = vertex + width + 1;
		indices[index++] = vertex + 1;
	}

	unsigned int vertSize = (width * height) * stride * sizeof(float);
	indexCount = ((width - 1) * (height - 1) * 6);

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	// vertex information!
	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, 0);
	glEnableVertexAttribArray(0);
	// normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);
	// uv
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 6));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	delete[] vertices;
	delete[] indices;

	//stbi_image_free(data);

	return VAO;
}

void createGeometry(GLuint& vao, GLuint&EBO, int& size, int& numIndices)
{
	// need 24 vertices for normal/uv-mapped Cube
	float vertices[] = {
		// positions            //colors            // tex coords   // normals          //tangents      //bitangents
		0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,        0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,        0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,        0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,        0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   2.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   2.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 2.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
		-0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 2.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

		-0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   -1.f, 1.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   -1.f, 0.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

		-0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, -1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
		0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, -1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

		-0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   3.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   3.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
		-0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

		-0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
		-0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

		-0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
		0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

		0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

		0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   2.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
		0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   2.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f
	};

	unsigned int indices[] = {  // note that we start from 0!
		// DOWN
		0, 1, 2,   // first triangle
		0, 2, 3,    // second triangle
		// BACK
		14, 6, 7,   // first triangle
		14, 7, 15,    // second triangle
		// RIGHT
		20, 4, 5,   // first triangle
		20, 5, 21,    // second triangle
		// LEFT
		16, 8, 9,   // first triangle
		16, 9, 17,    // second triangle
		// FRONT
		18, 10, 11,   // first triangle
		18, 11, 19,    // second triangle
		// UP
		22, 12, 13,   // first triangle
		22, 13, 23,    // second triangle
	};

	int stride = (3 + 3 + 2 + 3 + 3 + 3) * sizeof(float);

	size = sizeof(vertices) / stride;
	numIndices = sizeof(indices) / sizeof(int);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3* sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
	glEnableVertexAttribArray(4);

	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, (void*)(14 * sizeof(float)));
	glEnableVertexAttribArray(5);

	size = sizeof(vertices) / sizeof(vertices[0]); // Update the size
}

void createShaders()
{
	createProgram(simpleProgram, "shaders/simpleVertex.shader", "shaders/simpleFragment.shader");
	glUseProgram(simpleProgram);
	glUniform1i(glGetUniformLocation(simpleProgram, "mainTex"), 0);
	glUniform1i(glGetUniformLocation(simpleProgram, "normalTex"), 1);

	createProgram(skyProgram, "shaders/skyboxVertex.shader", "shaders/skyboxFragment.shader");
	createProgram(terrainProgram, "shaders/terrainVertex.shader", "shaders/terrainFragment.shader");

	createProgram(imgProgram, "shaders/VertImage.shader", "shaders/FragImage.shader");
	createProgram(chrabbProgram, "shaders/VertImage.shader", "shaders/FragChAbb.shader");

	createProgram(modelProgram, "shaders/modelVertex.shader", "shaders/modelFragment.shader");

	createProgram(starProgram, "shaders/skyboxVertex.shader", "shaders/starFragment.shader");

	createProgram(planetProgram, "shaders/planetVertex.shader", "shaders/planetFragment.shader");

	createProgram(moonProgram, "shaders/planetVertex.shader", "shaders/moonFragment.shader");

	createProgram(blurProgram, "shaders/VertImage.shader", "shaders/motionBlur.shader");

	glUseProgram(terrainProgram);
	glUniform1i(glGetUniformLocation(terrainProgram, "mainTex"), 0);
	glUniform1i(glGetUniformLocation(terrainProgram, "normalTex"), 1);

	glUniform1i(glGetUniformLocation(terrainProgram, "dirt"), 2);
	glUniform1i(glGetUniformLocation(terrainProgram, "sand"), 3);
	glUniform1i(glGetUniformLocation(terrainProgram, "grass"), 4);
	glUniform1i(glGetUniformLocation(terrainProgram, "rock"), 5);
	glUniform1i(glGetUniformLocation(terrainProgram, "snow"), 6);

	glUseProgram(modelProgram);
	glUniform1i(glGetUniformLocation(modelProgram, "diffuse1"), 0);
	glUniform1i(glGetUniformLocation(modelProgram, "specular1"), 1);
	glUniform1i(glGetUniformLocation(modelProgram, "normal1"), 2);
	glUniform1i(glGetUniformLocation(modelProgram, "roughness1"), 3);
	glUniform1i(glGetUniformLocation(modelProgram, "ao1"), 4);

	glUseProgram(planetProgram);
	glUniform1i(glGetUniformLocation(planetProgram, "day"), 0);
	glUniform1i(glGetUniformLocation(planetProgram, "night"), 1);
	glUniform1i(glGetUniformLocation(planetProgram, "clouds"), 2);
}

void createProgram(GLuint& programID, const char* vertex, const char* fragment)
{
	char* vertexSrc;
	char* fragmentSrc;

	loadFile(vertex, vertexSrc);
	loadFile(fragment, fragmentSrc);

	GLuint vertexShaderID, fragmentShaderID;

	vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderID, 1, &vertexSrc, nullptr);
	glCompileShader(vertexShaderID);

	int succes;
	char infoLog[512];
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &succes);

	if (!succes)
	{
		glGetShaderInfoLog(vertexShaderID, 512, nullptr, infoLog);
		std::cout << "ERROR COMPILING VERTEX SHADER\n" << infoLog << std::endl;
	}

	fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderID, 1, &fragmentSrc, nullptr);
	glCompileShader(fragmentShaderID);


	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &succes);
	if (!succes)
	{
		glGetShaderInfoLog(fragmentShaderID, 512, nullptr, infoLog);
		std::cout << "ERROR COMPILING FRAGMENT SHADER\n" << infoLog << std::endl;
	}

	programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	glGetProgramiv(programID, GL_LINK_STATUS, &succes);
	if (!succes)
	{
		glad_glGetProgramInfoLog(programID, 512, nullptr, infoLog);
		std::cout << "ERROR LINKING PROGRAM\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	delete[] vertexSrc;
	delete[] fragmentSrc;
}

void loadFile(const char* filename, char*& output)
{
	std::ifstream file(filename, std::ios::binary);

	if (file.is_open())
	{
		file.seekg(0, file.end);
		int length = file.tellg();
		file.seekg(0, file.beg);

		output = new char[length + 1];

		file.read(output, length);

		output[length] = '\0';
		file.close();
	}
	else
	{
		output = NULL;
	}
}

GLuint loadTexture(const char* path, int comp, GLint wrapTypeS, GLint wrapTypeT) 
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapTypeS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapTypeT);

	int width, height, numChannels;
	unsigned char* data = stbi_load(path, &width, &height, &numChannels, comp);
	if(data)
	{
		if (comp != 0) numChannels = comp;

		if(numChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		else if (numChannels == 4) 
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error loading texture: " << path << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}

GLuint loadCubemap(std::vector<string> filenames, int comp)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, numChannels;
	for (int i = 0; i < filenames.size(); i++) 
	{
		unsigned char* data = stbi_load(filenames[i].c_str(), &width, &height, &numChannels, comp);
		if (data)
		{
			if (comp != 0) numChannels = comp;

			if (numChannels == 3)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			else if (numChannels == 4)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		}
		else
		{
			std::cout << "Error loading texture: " << filenames[i].c_str() << std::endl;
		}
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureID;
}

void createFrameBuffer(int width, int height, unsigned& frameBufferID, unsigned int& colorBufferID, unsigned int& depthBufferID, unsigned int& depthTextureID)
{
	// Generate frame buffer
	glGenFramebuffers(1, &frameBufferID);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);

	// Generate color buffer
	glGenTextures(1, &colorBufferID);
	glBindTexture(GL_TEXTURE_2D, colorBufferID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferID, 0);

	// Generate depth texture
	glGenTextures(1, &depthTextureID);
	glBindTexture(GL_TEXTURE_2D, depthTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureID, 0);

	// Generate depth buffer
	glGenRenderbuffers(1, &depthBufferID);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBufferID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferID);

	// Check frame buffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "FrameBuffer not complete! " << std::endl;
	}

	// Unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToBuffer(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int shader) 
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferTo);

	glUseProgram(shader);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBufferFrom);

	//render something
	renderQuad();
	glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void renderBlur(unsigned int frameBufferTo, unsigned int colorBufferFrom, unsigned int depthBufferFrom, glm::mat4 viewInverse, glm::mat4 previousView)
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferTo);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(blurProgram);
	// Bind and activate textures as needed
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthBufferFrom); // Bind your depth texture
	glUniform1i(glGetUniformLocation(blurProgram, "depthTexture"), 0); // Texture unit 0

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, colorBufferFrom); // Bind your scene texture
	glUniform1i(glGetUniformLocation(blurProgram, "sceneSampler"), 1); // Texture unit 1

	// Set matrices as appropriate
	glUniformMatrix4fv(glGetUniformLocation(blurProgram, "g_ViewProjectionInverseMatrix"), 1, GL_FALSE, glm::value_ptr(viewInverse));
	glUniformMatrix4fv(glGetUniformLocation(blurProgram, "g_previousViewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(previousView));

	// Set other uniforms
	glUniform1i(glGetUniformLocation(blurProgram, "g_numSamples"), 12);

	//render something
	renderQuad();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad() 
{
	if (quadVAO == 0) 
	{
		float quadVertices[] =
		{
			-1.0f,	1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,	1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3* sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void renderModel(Model* model, glm::mat4 view, glm::mat4 projection, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(modelProgram);

	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, pos);
	world = world * glm::toMat4(glm::quat(rot));
	world = glm::scale(world, scale);

	glUniformMatrix4fv(glGetUniformLocation(modelProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(modelProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(modelProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(modelProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(modelProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));


	model->Draw(modelProgram);

	glDisable(GL_BLEND);
}

void renderPlanet(glm::mat4 view, glm::mat4 projection)
{
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(planetProgram);

	glm::mat4 world = glm::mat4(1.0f);
	world = glm::translate(world, glm::vec3(3000, 500, 100));
	world = glm::scale(world, glm::vec3(200,200,200));
	world = glm::rotate(world, glm::radians(23.0f), glm::vec3(1, 0, 0));
	world = glm::rotate(world, glm::radians((float)glfwGetTime()), glm::vec3(0, 1, 0));

	glUniformMatrix4fv(glGetUniformLocation(planetProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(planetProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(planetProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(planetProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(planetProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	glUniform1f(glGetUniformLocation(planetProgram, "time"), (float)glfwGetTime());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, day);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, night);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, clouds);


	sphere->Draw(modelProgram);

	glDisable(GL_BLEND);

	glm::mat4 parent = glm::mat4(1.0f);
	parent = glm::translate(parent, glm::vec3(3000, 500, 100));

	renderMoon(parent, view, projection);
}

void renderMoon(glm::mat4 parentWorld, glm::mat4 view, glm::mat4 projection) 
{
	glEnable(GL_DEPTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glUseProgram(moonProgram);

	glm::mat4 world = glm::mat4(1.0f);
	world *= parentWorld;
	world = glm::rotate(world, glm::radians((float)glfwGetTime()), glm::vec3(0, 1, 0));
	world = glm::translate(world, glm::vec3(0, 0, 6033));
	world = glm::scale(world, glm::vec3(50, 50, 50));

	glUniformMatrix4fv(glGetUniformLocation(moonProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
	glUniformMatrix4fv(glGetUniformLocation(moonProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(moonProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform3fv(glGetUniformLocation(moonProgram, "lightDirection"), 1, glm::value_ptr(lightDirection));
	glUniform3fv(glGetUniformLocation(moonProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

	glUniform1f(glGetUniformLocation(moonProgram, "time"), (float)glfwGetTime());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, moon);


	sphere->Draw(moonProgram);
}



