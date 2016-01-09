#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "../_commonHeaders/commons.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint screenWidth = 1280, screenHeight = 768;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
bool firstMouse = true;
GLfloat lastX = screenWidth / 2, lastY = screenHeight / 2;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

bool drawAsWireframe = false;

GLuint vao = 0, vbo;

void initQuad()
{
	GLfloat quadVertices[] = 
	{
		// Positions        // Texture Coords
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), static_cast<GLvoid*>(nullptr));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<GLvoid*>(3 * sizeof(GLfloat)));
	glBindVertexArray(0);
}

void createGBufTex(GLuint &texid, GLenum internalFormat, GLenum format, GLenum type)
{
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, screenWidth, screenHeight, 0, format, type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Task 4", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	glewInit();

	glViewport(0, 0, screenWidth, screenHeight);

	glEnable(GL_DEPTH_TEST);

	Shader bufferShader("shaders/bufferShader.vs", "shaders/bufferShader.frag");
	Shader lightShader("shaders/lightCalculation.vs", "shaders/lightCalculation.frag");

	lightShader.Use();

	glUniform1i(glGetUniformLocation(bufferShader.Program, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(bufferShader.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(bufferShader.Program, "gAlbedoSpec"), 2);

	bufferShader.Use();

	Model ourModel("models/nanosuit.obj");

	initQuad();

#pragma region light sources setup

	
	vector<glm::vec3> pointLightPositions = { glm::vec3(0.0f, 4.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(-1.5f, 2.5f, 0.0f), glm::vec3(1.5f, 2.5f, 0.0f) };
	int lightsN = pointLightPositions.size();
	vector<glm::vec3> pointLightsAmbients(lightsN, glm::vec3(0.1f, 0.1f, 0.1f));
	vector<glm::vec3> pointLightsDiffuses(lightsN, glm::vec3(0.7f, 0.7f, 0.7f));
	vector<glm::vec3> pointLightsSpeculars(lightsN, glm::vec3(0.7f, 0.7f, 0.7f));
	vector<float> pointLightsConstans(lightsN, 1.0f);
	vector<float> pointLightsLinear(lightsN, 0.7f);
	vector<float> pointLightsQuadratic(lightsN, 1.6f);

	glm::vec3 dirLightDirection(-0.2f, -1.0f, -0.3f);
	glm::vec3 dirLightAmbient = glm::vec3(0.05f, 0.05f, 0.05f);
	glm::vec3 dirLightDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::vec3 dirLightSpecular = glm::vec3(0.5f, 0.5f, 0.5f);

#pragma endregion 

#pragma region set gBuffer
	GLuint gBuffer, gPosition, gNormal, gAlbedoSpec, rboDepth;

	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	createGBufTex(gPosition, GL_RGB16F, GL_RGB, GL_FLOAT);
	createGBufTex(gNormal, GL_RGB16F, GL_RGB, GL_FLOAT);
	createGBufTex(gAlbedoSpec, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gPosition, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gNormal, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gAlbedoSpec, 0);

	GLuint attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	else
		std::cout << "Framebuffer complete" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion 

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		Do_Movement();

		glPolygonMode(GL_FRONT_AND_BACK, drawAsWireframe ? GL_LINE : GL_FILL);

		glm::mat4 projection = glm::perspective(camera.Zoom, float(screenWidth) / screenHeight, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model;
		model = glm::scale(model, glm::vec3(0.25f));

#pragma region draw scene as normal

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bufferShader.Use();

		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

		ourModel.Draw(bufferShader);

#pragma endregion 

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#pragma region draw with light sources

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightShader.Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

		for (GLuint i = 0; i < pointLightPositions.size(); i++)
		{
			glUniform3fv(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].position").c_str()), 1, &pointLightPositions[i][0]);

			glUniform3fv(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].ambient").c_str()), 1, &pointLightsAmbients[i][0]);
			glUniform3fv(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].diffuse").c_str()), 1, &pointLightsDiffuses[i][0]);
			glUniform3fv(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].specular").c_str()), 1, &pointLightsSpeculars[i][0]);

			glUniform1f(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].constant").c_str()), pointLightsConstans[i]);
			glUniform1f(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].linear").c_str()), pointLightsLinear[i]);
			glUniform1f(glGetUniformLocation(lightShader.Program, ("pointLights[" + std::to_string(i) + "].quadratic").c_str()), pointLightsQuadratic[i]);
		}

		glUniform3fv(glGetUniformLocation(lightShader.Program, "dirLight.direction"), 1, glm::value_ptr(dirLightDirection));
		glUniform3fv(glGetUniformLocation(lightShader.Program, "dirLight.ambient"), 1, glm::value_ptr(dirLightAmbient));
		glUniform3fv(glGetUniformLocation(lightShader.Program, "dirLight.diffuse"), 1, glm::value_ptr(dirLightDiffuse));
		glUniform3fv(glGetUniformLocation(lightShader.Program, "dirLight.specular"), 1, glm::value_ptr(dirLightSpecular));
		
		glUniform3fv(glGetUniformLocation(lightShader.Program, "viewPos"), 1, glm::value_ptr(camera.Position));
	
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, lightsN);
		glBindVertexArray(0);

#pragma endregion

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void Do_Movement()
{
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
	
	if (key == GLFW_KEY_V && action == GLFW_PRESS)
		drawAsWireframe = !drawAsWireframe;

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
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
	camera.ProcessMouseScroll(yoffset / 6);
}
