#define _CRT_SECURE_NO_WARNINGS
#include <string>
using namespace std;

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

#include "../_commonHeaders/commons.h"

#include <Windows.h>
#include <Commctrl.h>

GLuint screenWidth = 800, screenHeight = 600;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

float zoom = 2.0;
float xC = -1.5;
float yC = -1.0;
GLfloat lastX = 0, lastY = 0, startX = 0, startY = 0;
GLfloat currentX, currentY;
int maxIters = 200;
int radius = 4;

bool firstMouse = true;
bool mouse_pressed = false;
bool start_grab = false;
bool finish_grab = true;

PAINTSTRUCT ps;
HINSTANCE instance;
HWND iterationsTrackbar;
HWND iterationsLabel;
HWND radiusTrackbar;
HWND radiusLabel;

HWND WINAPI CreateTrackbar(
	HWND hwndDlg,  
	UINT iMin,    
	UINT iMax,    
	UINT defaultValue, 
	UINT x,
	UINT y,
	UINT height = 25,
	UINT width = 200)
{

	InitCommonControls();

	auto hwndTrack = CreateWindowEx(
		0,                                
		TRACKBAR_CLASS,                  
		"Trackbar Control",              
		WS_CHILD |
		WS_VISIBLE |
		TBS_AUTOTICKS |
		TBS_ENABLESELRANGE,       
		x, y,                     
		width, height,            
		hwndDlg,                  
		nullptr,                  
		instance,                 
		nullptr                   
		);

	SendMessage(hwndTrack, TBM_SETRANGE,
		WPARAM(TRUE),                  
		LPARAM(MAKELONG(iMin, iMax))); 

	SendMessage(hwndTrack, TBM_SETPAGESIZE,
		0, LPARAM(4));                   

	SendMessage(hwndTrack, TBM_SETPOS,
		WPARAM(TRUE),                   
		LPARAM(defaultValue));

	SetFocus(hwndTrack);

	return hwndTrack;
}

void updateUI()
{
	wchar_t buf[4];
	maxIters = SendMessage(iterationsTrackbar, TBM_GETPOS, 0, 0);
	radius = SendMessage(radiusTrackbar, TBM_GETPOS, 0, 0);
	wsprintfW(buf, L"%ld", maxIters);
	SetWindowTextW(iterationsLabel, buf);
	wsprintfW(buf, L"%ld", radius);
	SetWindowTextW(radiusLabel, buf);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	instance = GetModuleHandle(nullptr);
	auto window = glfwCreateWindow(screenWidth, screenHeight, "Task 1", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	auto winHwnd = glfwGetWin32Window(window);

	// Set the required callback functions
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	glewExperimental = GL_TRUE;
	glewInit();

	glViewport(0, 0, screenWidth, screenHeight);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	wchar_t buf[4];

	iterationsTrackbar =  CreateTrackbar(winHwnd, 0, 300, maxIters, 10, 10);
	wsprintfW(buf, L"%ld", maxIters);
	iterationsLabel = CreateWindowW(L"STATIC", buf, WS_CHILD | WS_VISIBLE, 220, 10, 30, 20, winHwnd, nullptr, NULL, NULL);
	radiusTrackbar = CreateTrackbar(winHwnd, 1, 6, radius, 10, 45);
	wsprintfW(buf, L"%ld", radius);
	radiusLabel = CreateWindowW(L"STATIC", buf, WS_CHILD | WS_VISIBLE, 220, 45, 30, 20, winHwnd, nullptr, NULL, NULL);

	Shader fractalShader("shaders/fractal.vs", "shaders/fractal.frag");

	GLfloat squareVertecies[] =
	{
		-1.0f,	-1.0f,	0.0f,
		1.0f,	-1.0f,	0.0f,
		-1.0f,	1.0f,	0.0f,
		-1.0f,	1.0f,	0.0f,
		1.0f,	1.0f,	0.0f,
		1.0f,	-1.0f,	0.0f
	};

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertecies), &squareVertecies, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), static_cast<GLvoid*>(nullptr));
	glBindVertexArray(0);

	auto texture = loadTexture1D("textures/texture.png");
	
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		updateUI();

		// Check and call events
		glfwPollEvents();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindTexture(GL_TEXTURE_1D, texture);

		fractalShader.Use();
		glUniform3f(glGetUniformLocation(fractalShader.Program, "coords_zoom"), xC, yC, zoom);
		glUniform2f(glGetUniformLocation(fractalShader.Program, "limits"), maxIters, radius);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) 
		mouse_pressed = action == GLFW_PRESS;	
}


void updateCoords(float multiplier)
{
	xC += (currentX / screenWidth) * zoom * (1 - multiplier);
	yC += (currentY / screenHeight) * zoom * (1 - multiplier);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	float multiplier = yoffset < 0 ? 1.25 : 0.75;
	updateCoords(multiplier);
	zoom *= multiplier;
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mouse_pressed && !start_grab)
	{
		startX = xpos;
		startY = ypos;
		start_grab = true;
		finish_grab = false;
	}

	if (!mouse_pressed && !finish_grab)
	{
		lastX = xpos;
		lastY = ypos;
		start_grab = false;
		finish_grab = true;

		if (lastX < startX) swap(lastX, startX);
		if (lastY < startY) swap(lastY, startY);
		float width = lastX - startX;
		float height = lastY - startY;
		currentX = (lastX + startX) / 2;
		currentY = (lastY + startY) / 2;
		float newZoom = width * zoom / screenWidth;
		updateCoords(newZoom / zoom);
		zoom = newZoom;
	}
	currentX = xpos;
	currentY = ypos;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}