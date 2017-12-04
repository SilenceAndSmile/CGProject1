// Std. Includes
#include <iostream>
#include <string>
#include <cmath>

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other Libs
#include <SOIL.h>
#include <learnopengl/filesystem.h>

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void do_movement();

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Camera
Camera  camera(glm::vec3(0.0f, 0.0f, 3.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool    keys[1024];

// Light attributes
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

//自定义的相关参数
const int n = 10;	//水面区域日后将会被划分为n行n列，以方便进行动态渲染
int time = glfwGetTime(); //控制水面正弦波的初始波动位置的常量

float vertices[512];	//水面顶点数组
int indices[512];	//水面索引数组

void VertexIndex(float *vertices, int *indices, int n);	//水面顶点数组及索引计算
void WaterSin(float *vertices, int n);	//水面正弦波数组的计算

							// The MAIN function, from here we start the application and run the game loop
int main()
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// GLFW Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	// OpenGL options
	glEnable(GL_DEPTH_TEST);	//开启深度测试
	glEnable(GL_POINT_SMOOTH);	//启用抗锯齿,对点和线
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);  //GL_NICEST代表图形显示质量优先
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_BLEND);	//启用混合
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	//源的每一个像素的alpha都等于自己的alpha，目标的每一个像素的alpha等于1减去该位置源像素的alpha。 因此不论叠加多少次，亮度不发生变化，规范的讲，指的是把渲染的图像融合到目标区域。


	// 水池与水面模型着色器,共用着色器可以有效减少代码冗余
	Shader lightingShader("lighting_maps.vs", "lighting_maps.frag");

	//加载pool.obj水池模型
	Model ourModel("pool.obj");

	// 加载贴图
	GLuint diffuseMap, diffuseMap2;
	glGenTextures(1, &diffuseMap);
	glGenTextures(1, &diffuseMap2);
	int width, height;
	unsigned char* image, *image2;
	// 漫反射贴图，其实就相当于给我们的水池外边贴上一幅图片，使其看起来像是“木质”的
	//水池贴图
	image = SOIL_load_image("pool_map.png", &width, &height, 0, SOIL_LOAD_RGB);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);

	//水面贴图
	image2 = SOIL_load_image("water.png", &width, &height, 0, SOIL_LOAD_RGB);
	glBindTexture(GL_TEXTURE_2D, diffuseMap2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image2);
	//纹理过滤函数，也即是将图象从纹理空间映射到帧缓冲空间
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);	
	// 将图片设置为纹理
	lightingShader.Use();
	glUniform1i(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0);

	//在动态绘制之前调用函数计算顶点数组值，提高程序效率
	VertexIndex(vertices, indices, n);

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Calculate deltatime of current frame
		GLfloat currentFrame = glfwGetTime();
		deltaTime += currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
		do_movement();

		while (deltaTime > 0.15) //控制每0.15s让水面“动一次”
		{
			// Clear the colorbuffer
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f); //将背景设置为浅灰色
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//把相应的摄像机位置坐标传给片段着色器，也即是使用使用摄像机对象的位置坐标代替观察者的位置
			lightingShader.Use();
			GLint lightPosLoc = glGetUniformLocation(lightingShader.Program, "light.position");
			GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
			glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(viewPosLoc, camera.Position.x, camera.Position.y, camera.Position.z);
			// 设置光线属性
			glUniform3f(glGetUniformLocation(lightingShader.Program, "light.ambient"), 0.2f, 0.2f, 0.2f);
			glUniform3f(glGetUniformLocation(lightingShader.Program, "light.diffuse"), 0.5f, 0.5f, 0.5f);
			glUniform3f(glGetUniformLocation(lightingShader.Program, "light.specular"), 1.0f, 1.0f, 1.0f);
			// 材质属性
			glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 32.0f);

			// 摄像机转换矩阵
			glm::mat4 view;
			view = camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(camera.Zoom, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
			// 得到统一位置  
			GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
			GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
			GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");
			// 将矩阵转递给着色器  
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

			// 混合环境贴图到纹理
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseMap);

			//绘制已加载的模型
			glm::mat4 model;
			model = glm::translate(model, glm::vec3(0.0f, -1.75f, -2.0f)); // 转换它（模型坐标）使得其（模型）可以同时（在程序初始运行时）被我们观察到正面和顶部
			model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// 缩小模型规模使其在场景中显得更加真实
			glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			ourModel.Draw(lightingShader);

			//以下为水面部分
			WaterSin(vertices, n);
			time++;		//time是作为全局变量直接传递到WaterSin函数当中的

			GLuint VBO, VAO, EBO;
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0 * sizeof(float)));
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			//激活并使用水面图片纹理
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseMap2);
			lightingShader.Use();

			glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 512, GL_UNSIGNED_INT, 0);
			//线框模式用于检查三角形索引法绘制是否正确
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBindVertexArray(0);
			glfwSwapBuffers(window);	//向屏幕绘制一次，即交换一次缓冲区
			deltaTime = 0.0;	//重设计时器
		}
	}

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

void do_movement()
{
	// Camera controls
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

void VertexIndex(float *vertices, int *indices, int n)	//水面顶点数组及索引计算
{
	//pool.obj中的坐标轴与数学上的三维空间坐标轴不同，Y轴是在竖直方向，Z轴是在水平方向,为了与模型把持一致，我们以下的顶点数组中采用这种坐标系
	//坐标范围是X轴(-3,3),Z轴(-3,3)
	float valueX = -3;
	float valueZ = 3;	//以上两个参数是为了顶点数组从水池左上角开始计算水面顶点数组，这样比较符合日常直觉
	int index = 0;
	float number =6.0 / (n - 1);	//得到单行/单列的格网数
	//顶点数组计算 注意valueX与valueZ的方向与二维笛卡尔坐标的X和Y方向一致
	for (int i = 0; i < n*n; i++)	//计算的顺序是先保持Z值不变从左到右，然后再移动到下一行进行重复（即优先从左到右，再从上到下）
	{
		vertices[index++] = valueX;	
		vertices[index++] = 0;	//该部分的值后续是要动态计算的,因而此处赋值多少都可以
		vertices[index++] = valueZ;

		valueX += number;	//移动到下一列
		if ((i+1)%n==0)	//代表一行已经计算完毕，故而要将valueX（行首的X值）恢复到3,而对Z进行一次减法，使之移动到下一行
		{
			valueX = -3;
			valueZ -= number;
		}
	}
	index = 0;
	//顶点索引计算，详细参见索引图
	for (int i = 0; i < n*(n-1)+1; i++) {
		if ((i + 1) % n != 0) //避免每一行的最后一个顶点与下一行第一个顶点之间产生索引索引
		{
			//三角形A
			indices[index++] = i;
			indices[index++] = i + 1;
			indices[index++] = i + n;
			//三角形B
			indices[index++] = i + 1;
			indices[index++] = i + n;
			indices[index++] = i + n + 1;
		}
	}
}

void WaterSin(float *vertices, int n)	//水面正弦波数组的计算
{
	for (int i = 0; i < n*n; i++) {
		float d = sqrt((vertices[i * 3] - 3)*(vertices[i * 3] - 3) + (vertices[i * 3 + 2] - 3)*(vertices[i * 3 + 2] - 3));//计算X与Z的二范数
		//注意，由于1.5是真实的最高坐标，因而我们最后要让正弦波叠加后的最大值比1.5稍低一些从而使得水面看起来不会“溢出去”
		const int WaveNumber = 20;
		float wave[WaveNumber];	//将WaveNumber个波都计算到wave数组中
		float sum = 0;
		for (int j = 0; j < WaveNumber; j++) {
			wave[j]= 0.007*sin(d*2/WaveNumber*3.14 + time*0.2*3.14);
			sum += wave[j];	//sum即为物理模型下的最终海波
		}
		vertices[i * 3 + 1] = sum+1.15;
	}
}