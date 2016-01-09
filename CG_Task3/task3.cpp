#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "../_commonHeaders/commons.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

GLuint screenWidth = 1280, screenHeight = 720;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
Camera staticProjectionCamera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = screenWidth / 2, lastY = screenHeight / 2;
bool firstMouse = true;
bool isDynamicTexture = false;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

void setupStaticProjectionCamera()
{
	staticProjectionCamera.Position = glm::vec3(0.534791, -0.205869, 2.341877);
	staticProjectionCamera.Yaw = -104.5;
	staticProjectionCamera.Pitch = -17.75;
	staticProjectionCamera.Front = glm::vec3(-0.238461, -0.304864, -0.922060);
	staticProjectionCamera.Up = glm::vec3(-0.076332, 0.952396, -0.295154);
	staticProjectionCamera.Right = glm::vec3(0.968148, 0.000000, -0.250380);
}

GLint FILTER_TYPE = GL_NEAREST;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Task 3", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	glewExperimental = GL_TRUE;
	glewInit();

	glViewport(0, 0, screenWidth, screenHeight);

	glEnable(GL_DEPTH_TEST);

	Shader shader("shaders/shader.vs", "shaders/shader.frag");
	Shader bufferShader("shaders/gBuffer.vs", "shaders/gBuffer.frag");
	Shader frustumShader("shaders/frustum.vs", "shaders/frustum.frag");

	Model ourModel("models/nanosuit.obj");

	auto terrainSize = 50.0f;
	GLfloat squareVertecies[] =
	{
		-terrainSize,		0.0f,	-terrainSize,		0.0f,	0.0f,
		terrainSize,		0.0f,	-terrainSize,		1.0f,	0.0f,
		-terrainSize,		0.0f,	terrainSize,		0.0f,	1.0f,
		terrainSize,		0.0f,	terrainSize,		1.0f,	1.0f,
	};

	GLuint indices[] = 
	{
		0, 1, 2,
		2, 3, 1
	};

	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertecies), &squareVertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), static_cast<GLvoid*>(nullptr));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<GLvoid*>(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);

	auto smileTexture = loadTextureWithoutRepeat("textures/smile.png");
	auto grassTexture = loadTexture("textures/grass.png");

	setupStaticProjectionCamera();

	float angle = 0.0f;

	GLuint gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	GLuint gAlbedoSpec;

	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	GLuint rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gAlbedoSpec, 0);

	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float pFar = 100.0f;
	float pNear = 0.1f;

	glm::vec3 ftl = glm::vec3(-1, +1, pFar); //far top left
	glm::vec3 fbr = glm::vec3(+1, -1, pFar); //far bottom right
	glm::vec3 fbl = glm::vec3(-1, -1, pFar); //far bottom left
	glm::vec3 ftr = glm::vec3(+1, +1, pFar); //far top right
	glm::vec3 ntl = glm::vec3(-1, +1, pNear); //near top left
	glm::vec3 nbr = glm::vec3(+1, -1, pNear); //near bottom right
	glm::vec3 nbl = glm::vec3(-1, -1, pNear); //near bottom left
	glm::vec3 ntr = glm::vec3(+1, +1, pNear); //near top right

	glm::vec3 frustum_coords[36] = {
		// near
		ntl, nbl, ntr, // 1 triangle
		ntr, nbl, nbr,
		// right
		nbr, ftr, ntr,
		ftr, nbr, fbr,
		// left
		nbl, ftl, ntl,
		ftl, nbl, fbl,
		// far
		ftl, fbl, fbr,
		fbr, ftr, ftl,
		//bottom
		nbl, fbr, fbl,
		fbr, nbl, nbr,
		//top
		ntl, ftr, ftl,
		ftr, ntl, ntr
	};

	GLuint camVAO, camVBO;

	glGenVertexArrays(1, &camVAO);
	glGenBuffers(1, &camVBO);

	glBindVertexArray(camVAO);
	glBindBuffer(GL_ARRAY_BUFFER, camVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frustum_coords), &frustum_coords, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), static_cast<GLvoid*>(nullptr));
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		Do_Movement();

		glm::vec3 flyingPosition = glm::vec3(3.0f * cos(angle), 0.0f, 3.0f * sin(angle));
		glm::mat4 flyingView = glm::lookAt(flyingPosition, glm::vec3(0.0f, 0.0f, 0.0f), camera.Up);
		glm::mat4 projection = glm::perspective(camera.Zoom, float(screenWidth) / float(screenHeight), pNear, pFar);

		glm::mat4 model;
		model = glm::translate(model, glm::vec3(0.0f, -2.5f, -1.5f));
		model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glViewport(0, 0, screenWidth, screenHeight);

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bufferShader.Use();

		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(flyingView));
		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		
		ourModel.Draw(bufferShader);

		glBindTexture(GL_TEXTURE_2D, grassTexture);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.Use();

		glm::mat4 view = camera.GetViewMatrix();

		glm::mat4 projView = staticProjectionCamera.GetViewMatrix();
		glm::mat4 projProjection = glm::perspective(staticProjectionCamera.Zoom, 1.0f, pNear, pFar);

	
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projProjection"), 1, GL_FALSE, glm::value_ptr(projProjection));
		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projView"), 1, GL_FALSE, glm::value_ptr(projView));

		glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		
		glUniform1i(glGetUniformLocation(shader.Program, "projectiveTexture"), 14);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, isDynamicTexture ? gAlbedoSpec : smileTexture);
		if (isDynamicTexture) glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FILTER_TYPE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FILTER_TYPE == GL_LINEAR_MIPMAP_LINEAR ? GL_LINEAR : FILTER_TYPE);

		ourModel.Draw(shader);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassTexture);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);

		//draw debug terrain to view rotating model 'movie'
		{
			model = glm::translate(model, glm::vec3(0, terrainSize,-terrainSize));
			model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(0, 0, 1));
			model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(1, 0, 0));
			glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

		//finally draw static camera
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		frustumShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(frustumShader.Program, "InvProjProjection"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projProjection)));
		glUniformMatrix4fv(glGetUniformLocation(frustumShader.Program, "InvProjModelView"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projView * model)));
		glUniformMatrix4fv(glGetUniformLocation(frustumShader.Program, "Projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(frustumShader.Program, "ModelView"), 1, GL_FALSE, glm::value_ptr(view * model));

		glBindVertexArray(camVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		//And flying one
		glUniformMatrix4fv(glGetUniformLocation(frustumShader.Program, "InvProjProjection"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
		glUniformMatrix4fv(glGetUniformLocation(frustumShader.Program, "InvProjModelView"), 1, GL_FALSE, glm::value_ptr(glm::inverse(flyingView * model)));
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);


		angle += deltaTime * 0.5;
		if (angle > glm::two_pi<float>())
			angle = 0;
		
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void Do_Movement()
{
	if (keys[GLFW_KEY_SPACE])
		staticProjectionCamera = camera;
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
	
	if (action == GLFW_RELEASE) return;

	if (key == GLFW_KEY_G)
		isDynamicTexture = !isDynamicTexture;
	if (key == GLFW_KEY_1)
		FILTER_TYPE = GL_NEAREST;
	if (key == GLFW_KEY_2)
		FILTER_TYPE = GL_LINEAR;
	if (key == GLFW_KEY_3)
		FILTER_TYPE = GL_LINEAR_MIPMAP_LINEAR;
	if (key == GLFW_KEY_P)
	{
		std::cout << staticProjectionCamera.Pitch << staticProjectionCamera.Yaw <<
			glm::to_string(staticProjectionCamera.Position) << glm::to_string(staticProjectionCamera.Front) <<
			glm::to_string(staticProjectionCamera.Up) << glm::to_string(staticProjectionCamera.Right) << std::endl;
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset / 8);
}