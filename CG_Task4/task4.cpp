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

Camera camera(glm::vec3(0.0f, 20.0f, 40.0f));
bool keys[1024];
bool firstMouse = true;
GLfloat lastX = screenWidth / 2, lastY = screenHeight / 2;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//bool drawAsWireframe = false;
int drawMode = 5;
GLfloat speed = 10;
const double radius = 10;
const double offset = 16;
bool drawAsWireframe = false;
bool enableGamma = false;
bool drawSpheres = true;

GLuint vao, vbo;

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

double get_random_for_ambient()
{
	int rand1 = rand();
	return (rand1 % 100 + 26) * 0.0008;
}

glm::vec3 get_random_ambient()
{
	double ambient = get_random_for_ambient();
	return glm::vec3(ambient, ambient, ambient);
}

double random_diffuse_specular()
{
	int rand1 = rand();
	return (rand1 % 100 + 100) * 0.005;
}

glm::vec3 get_random_color()
{
	return glm::vec3(random_diffuse_specular(), random_diffuse_specular(), random_diffuse_specular());
}

void setLightsPosition(vector<glm::vec3>& pointLightsPositions, double step)
{
	double angle = 0;
	double angleStep = AI_MATH_TWO_PI_F / pointLightsPositions.size();

	for (auto i = 0; i < pointLightsPositions.size(); ++i)
	{
		auto theta = i % 2 == 0 ? step : -step;
		pointLightsPositions[i] = glm::vec3(
			radius * sin(angle) * cos(step), 
			offset + radius * cos(angle) * cos(step),
			radius * sin(theta));
		angle += angleStep;
	}
}

void drawQuad()
{
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
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
	Shader directLightShader("shaders/directLight.vs", "shaders/directLight.frag");
	Shader pointLightShader("shaders/pointLight.vs", "shaders/pointLight.frag");
	Shader resultShader("shaders/textureDraw.vs", "shaders/textureDraw.frag");
	Shader sphereShader("shaders/sphereDraw.vs", "shaders/sphereDraw.frag");

	glUniform1i(glGetUniformLocation(directLightShader.Program, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(directLightShader.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(directLightShader.Program, "gAlbedoSpec"), 2);

	glUniform1i(glGetUniformLocation(pointLightShader.Program, "gPosition"), 0);
	glUniform1i(glGetUniformLocation(pointLightShader.Program, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(pointLightShader.Program, "gAlbedoSpec"), 2);

	bufferShader.Use();

	Model ourModel("models/nanosuit.obj");
	Model sphereModel("models/sphere.obj");

	initQuad();

#pragma region light sources setup

	glm::vec3 dirLightDirection(-0.2f, -1.0f, -0.3f);
	glm::vec3 dirLightAmbient = glm::vec3(0.05f, 0.05f, 0.05f);
	glm::vec3 dirLightDiffuse = glm::vec3(0.2f, 0.2f, 0.2f);
	glm::vec3 dirLightSpecular = glm::vec3(0.2f, 0.2f, 0.2f);

	size_t lightsNumber = 10; //set number of light sources

	vector<glm::vec3> pointLightsPositions(lightsNumber);
	vector<glm::vec3> pointLightsAmbients(lightsNumber);
	vector<glm::vec3> pointLightsDiffuses(lightsNumber);
	vector<glm::vec3> pointLightsSpeculars(lightsNumber);


	for (auto i = 0; i < lightsNumber; ++i)
	{
		pointLightsAmbients[i] = get_random_ambient();
		pointLightsDiffuses[i] = pointLightsSpeculars[i] = get_random_color();
	}

	double step = 0;
	setLightsPosition(pointLightsPositions, step);

	vector<float> pointLightsConstant(lightsNumber, 1.0f);
	vector<float> pointLightsLinear(lightsNumber, 0.8f);
	vector<float> pointLightsQuadratic(lightsNumber, 1.9f);

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

#pragma region set output texture for light calculation

	GLuint lightBuffer, lightDepth, lightColor;
	glGenFramebuffers(1, &lightBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, lightBuffer);

	createGBufTex(lightColor, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	glGenRenderbuffers(1, &lightDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, lightDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, lightDepth);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lightColor, 0);

	GLuint attachments2[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments2);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer for lighting not complete!" << std::endl;
	else
		std::cout << "Framebuffer for lighting complete" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		step += deltaTime * 0.5;

		glfwPollEvents();
		Do_Movement();

		glPolygonMode(GL_FRONT_AND_BACK, drawAsWireframe ? GL_LINE : GL_FILL);

		glm::mat4 projection = glm::perspective(camera.Zoom, float(screenWidth) / screenHeight, 0.1f, 1000.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model;
		model = scale(model, glm::vec3(2));

#pragma region draw scene as normal

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bufferShader.Use();

		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(bufferShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

		ourModel.Draw(bufferShader);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#pragma endregion 

		glBindFramebuffer(GL_FRAMEBUFFER, lightBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#pragma region directional light calculation
		
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		directLightShader.Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

		glUniform3fv(glGetUniformLocation(directLightShader.Program, "dirLight.direction"), 1, glm::value_ptr(dirLightDirection));
		glUniform3fv(glGetUniformLocation(directLightShader.Program, "dirLight.ambient"), 1, glm::value_ptr(dirLightAmbient));
		glUniform3fv(glGetUniformLocation(directLightShader.Program, "dirLight.diffuse"), 1, glm::value_ptr(dirLightDiffuse));
		glUniform3fv(glGetUniformLocation(directLightShader.Program, "dirLight.specular"), 1, glm::value_ptr(dirLightSpecular));

		glUniform3fv(glGetUniformLocation(directLightShader.Program, "viewPos"), 1, glm::value_ptr(camera.Position));

		drawQuad();

#pragma endregion 

#pragma region calculate point lights

		setLightsPosition(pointLightsPositions, step);
		pointLightShader.Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

		glUniformMatrix4fv(glGetUniformLocation(pointLightShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(pointLightShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniform2f(glGetUniformLocation(pointLightShader.Program, "screenSize"), screenWidth, screenHeight);

		for (GLuint i = 0; i < pointLightsPositions.size(); i++)
		{
			glUniform3fv(glGetUniformLocation(pointLightShader.Program, "pointLight.position"), 1, &pointLightsPositions[i][0]);

			glUniform3fv(glGetUniformLocation(pointLightShader.Program, "pointLight.ambient"), 1, &pointLightsAmbients[i][0]);
			glUniform3fv(glGetUniformLocation(pointLightShader.Program, "pointLight.diffuse"), 1, &pointLightsDiffuses[i][0]);
			glUniform3fv(glGetUniformLocation(pointLightShader.Program, "pointLight.specular"), 1, &pointLightsSpeculars[i][0]);

			glUniform1f(glGetUniformLocation(pointLightShader.Program, "pointLight.constant"), pointLightsConstant[i]);
			glUniform1f(glGetUniformLocation(pointLightShader.Program, "pointLight.linear"), pointLightsLinear[i]);
			glUniform1f(glGetUniformLocation(pointLightShader.Program, "pointLight.quadratic"), pointLightsQuadratic[i]);

			const GLfloat lightThreshold = 5.0;
			const GLfloat maxBrightness = fmaxf(fmaxf(pointLightsDiffuses[i].r, pointLightsDiffuses[i].g), pointLightsDiffuses[i].b);
			float linear = pointLightsLinear[i];
			float quadratic = pointLightsQuadratic[i];
			float constant = pointLightsConstant[i];
			GLfloat radius = (-linear + static_cast<float>(sqrt(linear * linear - 4 * quadratic * (constant - (256.0 / lightThreshold) * maxBrightness)))) / (2 * quadratic);
			model = glm::mat4();
			model = translate(model, pointLightsPositions[i]);
			model = scale(model, glm::vec3(radius));

			glUniformMatrix4fv(glGetUniformLocation(pointLightShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			sphereModel.Draw(pointLightShader);
		}

#pragma endregion

#pragma region draw result
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_BLEND);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		resultShader.Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, lightColor);

		glUniform1i(glGetUniformLocation(resultShader.Program, "mode"), drawMode);
		glUniform1i(glGetUniformLocation(resultShader.Program, "enableGamma"), enableGamma);
	
		drawQuad();

#pragma endregion

#pragma region draw spheres

		if (drawSpheres)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			sphereShader.Use();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			for (auto i = 0; i < lightsNumber; ++i)
			{
				const GLfloat lightThreshold = 5.0;
				const GLfloat maxBrightness = fmaxf(fmaxf(pointLightsDiffuses[i].r, pointLightsDiffuses[i].g), pointLightsDiffuses[i].b);
				float linear = pointLightsLinear[i];
				float quadratic = pointLightsQuadratic[i];
				float constant = pointLightsConstant[i];
				GLfloat radius = (-linear + static_cast<float>(sqrt(linear * linear - 4 * quadratic * (constant - (256.0 / lightThreshold) * maxBrightness)))) / (2 * quadratic);
				model = glm::mat4();
				model = translate(model, pointLightsPositions[i]);
				model = scale(model, glm::vec3(radius));

				glUniformMatrix4fv(glGetUniformLocation(sphereShader.Program, "mvp"), 1, GL_FALSE, value_ptr(projection * view * model));
				glUniform3fv(glGetUniformLocation(sphereShader.Program, "sphereColor"), 1, &pointLightsDiffuses[i][0]);

				sphereModel.Draw(sphereShader);
			}
		}

#pragma endregion 

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void Do_Movement()
{
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime * speed);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime * speed);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime * speed);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime * speed);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	
	if (key == GLFW_KEY_V && action == GLFW_PRESS)
		drawAsWireframe = !drawAsWireframe;

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
		enableGamma = !enableGamma;

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		drawSpheres = !drawSpheres;

	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		drawMode = 1;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		drawMode = 2;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		drawMode = 3;
	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
		drawMode = 4;
	if (key == GLFW_KEY_5 && action == GLFW_PRESS)
		drawMode = 5;

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
