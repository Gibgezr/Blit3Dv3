#include "Blit3D.h"

logger oLog("Blit3D.log", false);

void(*Blit3DDoFileDrop)(int, const char**);
void(*Blit3DDoInput)(int, int, int, int);
void(*Blit3DCursorPosition)(double, double);
void(*Blit3DMouseButton)(int, int, int);
void(*Blit3DScrollwheel)(double, double);

namespace B3D
{
	std::mutex loopMutex;

	std::atomic<bool> quitLooping; //global var for multi-threaded loop control
};

void SimpleThreadUpdate(void(*Update)(double))
{
	double time = glfwGetTime();
	double prevTime = time;
	double elapsedTime = 0;

	for(;;)
	{
		B3D::loopMutex.lock();
		if(B3D::quitLooping)
		{
			//time to get out of here
			B3D::loopMutex.unlock();
			return;
		}
		time = glfwGetTime();
		elapsedTime = time - prevTime;
		prevTime = time;

		Update(elapsedTime);
		B3D::loopMutex.unlock();
	}
}

void MultiThreadUpdate(void(*Update)(double))
{
	double time = glfwGetTime();
	double prevTime = time;
	double elapsedTime = 0;

	for(;;)
	{
		if(B3D::quitLooping)
		{
			//time to get out of here
			return;
		}
		time = glfwGetTime();
		elapsedTime = time - prevTime;
		prevTime = time;

		Update(elapsedTime);
	}
}

static void drop_callback(GLFWwindow* window, int count, const char** paths)
{
	if(Blit3DDoFileDrop) Blit3DDoFileDrop(count, paths);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(Blit3DDoInput) Blit3DDoInput(key, scancode, action, mods);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if(Blit3DCursorPosition) Blit3DCursorPosition(xpos, ypos);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if(Blit3DMouseButton) Blit3DMouseButton(button, action, mods);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if(Blit3DScrollwheel) Blit3DScrollwheel(xoffset, yoffset);
}

Blit3D::Blit3D(Blit3DWindowModel windowMode, int width, int height)
{
	sManager = NULL;
	tManager = NULL;	

	Init = NULL;
	Update = NULL;
	Draw = NULL;
	DeInit = NULL;
	DoInput = NULL;
	Blit3DDoInput = NULL;
	Sync = NULL;
	DoCursor = NULL;
	Blit3DCursorPosition = NULL;
	DoMouseButton = NULL;
	Blit3DMouseButton = NULL;
	DoScrollwheel = NULL;
	Blit3DScrollwheel = NULL;
	DoJoystick = NULL;
	Blit3DDoFileDrop = NULL;

	winMode= windowMode;
	screenWidth = (float)width;
	screenHeight = (float)height;

	nearplane = 0.1f;
	farplane = 10000.f;

	shader2d = NULL;
	window = NULL;
}

Blit3D::Blit3D()
{
	sManager = NULL;
	tManager = NULL;

	Init = NULL;
	Update = NULL;
	Draw = NULL;
	DeInit = NULL;
	DoInput = NULL;
	Blit3DDoInput = NULL;
	Sync = NULL;
	DoCursor = NULL;
	Blit3DCursorPosition = NULL;
	DoMouseButton = NULL;
	Blit3DMouseButton = NULL;
	DoScrollwheel = NULL;
	Blit3DScrollwheel = NULL;
	DoJoystick = NULL;
	Blit3DDoFileDrop = NULL;

	winMode = Blit3DWindowModel::BORDERLESSFULLSCREEN;
	screenWidth = 1920.f;
	screenHeight = 1080.f;

	nearplane = 0.1f;
	farplane = 10000.f;

	shader2d = NULL;
	window = NULL;
}


Blit3D::~Blit3D()
{
	//free all font memory first
	for (std::unordered_set<AngelcodeFont *>::iterator itr = fontSet.begin(); itr != fontSet.end(); itr++)
	{
		delete *itr;
	}
	fontSet.clear(); // clear the elements 

	//free all sprite memory
	for(std::unordered_set<Sprite *>::iterator itr = spriteSet.begin(); itr != spriteSet.end(); itr++)
	{
		delete *itr;
	}
	spriteSet.clear(); // clear the elements 

	//free the managers and all of their associated memory
	if (tManager) delete tManager;
	if (sManager) delete sManager;
}

void Blit3D::Quit()
{
	glfwSetWindowShouldClose(window, GL_TRUE);
}

void Blit3D::SetInit(void(*func)(void))
{
	Init = func;
}

void Blit3D::SetUpdate(void(*func)(double))
{
	Update = func;
}

void Blit3D::SetDraw(void(*func)(void))
{
	Draw = func;
}

void Blit3D::SetDeInit(void(*func)(void))
{
	DeInit = func;
}

void Blit3D::SetDoFileDrop(void(*func)(int, const char **))
{
	DoFileDrop = func;
	Blit3DDoFileDrop = DoFileDrop;
}

void Blit3D::SetDoInput(void(*func)(int, int, int, int))
{
	DoInput = func;
	Blit3DDoInput = DoInput;
}

void Blit3D::SetSync(void(*func)(void))
{
	Sync = func;
}

void Blit3D::SetDoCursor(void(*func)(double, double))
{
	DoCursor = func;
	Blit3DCursorPosition = DoCursor;
}

void Blit3D::SetDoMouseButton(void(*func)(int, int, int))
{
	DoMouseButton = func;
	Blit3DMouseButton = DoMouseButton;
}

void Blit3D::SetDoScrollwheel(void(*func)(double, double))
{
	DoScrollwheel = func;
	Blit3DScrollwheel = DoScrollwheel;
}

bool Blit3D:: PollJoystick(int joystickNumber, B3D::JoystickState &joystickState)
{
	if(!glfwJoystickPresent(joystickNumber - 1)) return false;

	//first fetch axis states
	joystickState.axisStates = glfwGetJoystickAxes(joystickNumber - 1, &joystickState.axisCount);
	//now fetch button states
	joystickState.buttonStates = glfwGetJoystickButtons(joystickNumber - 1, &joystickState.buttonCount);

	return true;
}

bool Blit3D::CheckJoystick(int joystickNumber)
{
	return (glfwJoystickPresent(joystickNumber - 1) != 0);
}

void Blit3D::SetDoJoystick(void(*func)(void))
{
	DoJoystick = func;
}

void Blit3D::ShowCursor(bool show)
{
	if(show) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

int Blit3D::Run(Blit3DThreadModel threadType)
{
	// start GL context and O/S window using the GLFW helper library
	if(!glfwInit())
	{
		oLog(Level::Severe) << "Could not start GLFW3";
		return 1;
	}

	// uncomment these lines if on Apple OS X
	/*glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

	GLint samples = 8;
	glGetIntegerv(GL_SAMPLES, &samples);
	if(samples)
	{
		oLog() << "Context reports FSAA is available with " << samples << "samples";
	}
	else
		oLog(Level::Info) << "Context reports FSAA is unavailable";

	if(samples)
	{
		oLog(Level::Info) << "Requesting FSAA with " << samples << "samples";
		glEnable(GL_MULTISAMPLE);
	}
	else
		oLog(Level::Info) << "Requesting that FSAA not be available";

	glfwWindowHint(GLFW_SAMPLES, samples);

	GLFWmonitor** monitors;
	int count;

	monitors = glfwGetMonitors(&count);
	if(count > 1)
	{
		oLog(Level::Info) << count << " monitors detected, running on primary one";
	}
	else
	{
		oLog(Level::Info) << "1 monitor detected.";
	}
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	switch(winMode)
	{
	case Blit3DWindowModel::FULLSCREEN:
		window = glfwCreateWindow(mode->width, mode->height, "Blit3D", glfwGetPrimaryMonitor(), NULL);
		trueScreenHeight = screenHeight = static_cast<float>(mode->height);
		trueScreenWidth = screenWidth = static_cast<float>(mode->width);
		break;
	case Blit3DWindowModel::DECORATEDWINDOW:
		window = glfwCreateWindow((int)screenWidth, (int)screenHeight, "Blit3D", NULL, NULL);
		trueScreenHeight = static_cast<float>(screenHeight);
		trueScreenWidth = static_cast<float>(screenWidth);
		break;
	case Blit3DWindowModel::BORDERLESSFULLSCREEN:	
		glfwWindowHint(GLFW_DECORATED, false);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		window = glfwCreateWindow(mode->width, mode->height, "Blit3D", NULL, NULL);
		trueScreenHeight = screenHeight = static_cast<float>(mode->height);
		trueScreenWidth = screenWidth = static_cast<float>(mode->width);
		break;
	case Blit3DWindowModel::BORDERLESSFULLSCREEN_1080P:
		glfwWindowHint(GLFW_DECORATED, false);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		window = glfwCreateWindow(mode->width, mode->height, "Blit3D", NULL, NULL);
		trueScreenHeight = static_cast<float>(mode->height);
		trueScreenWidth = static_cast<float>(mode->width);
		screenHeight = 1080.f; //we lie!
		screenWidth = 1920.f;
		break;
	}
	
	if(!window)
	{
		oLog(Level::Severe) << "Could not open requested window with GLFW3";
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);

	//set some callbacks for input modes
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);	
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetDropCallback(window, drop_callback);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	oLog(Level::Info) << "Renderer: " << renderer;
	oLog(Level::Info) << "OpenGL version supported: " << version;

	sManager = new ShaderManager();
	tManager = new TextureManager();

	projectionMatrix = glm::mat4(1.f);
	viewMatrix = glm::mat4(1.f);

	//glEnable(GL_CULL_FACE); // enables face culling    
	glCullFace(GL_BACK); // tells OpenGL to cull back faces (the sane default setting)
	glFrontFace(GL_CCW); // tells OpenGL which faces are considered 'front' (use GL_CW or GL_CCW)

	//enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	//clear colour: r,g,b,a 	

	glfwSwapInterval(1); //cap FPS

	//load default 2D shader
	std::string vert2d = "#version 330 \n"
		"uniform mat4 projectionMatrix; \n"
		"uniform mat4 viewMatrix; \n"
		"uniform mat4 modelMatrix; \n"
		"in vec3 in_Position; \n"
		"in vec2 in_Texcoord; \n"
		"uniform float in_Alpha = 1.f; \n"
		"uniform float in_Scale_X = 1.f; \n"
		"uniform float in_Scale_Y = 1.f; \n"
		"out vec2 v_texcoord; \n"
		"out vec4 gl_Position; \n"
		"void main(void)\n"
		"{\n"
			"gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(in_Position.x * in_Scale_X, in_Position.y * in_Scale_Y, in_Position.z, 1.0); \n"
			"v_texcoord = in_Texcoord; \n"
		"}";

	std::string frag2d = "#version 330 \n" 
		"uniform sampler2D mytexture; \n" 
		"in vec2 v_texcoord; \n" 
		"uniform float in_Alpha; \n" 
		"out vec4 out_Color; \n" 
		"void main(void)" 
		"{ \n" 
		"vec4 myTexel = texture2D(mytexture, v_texcoord); \n" 
		"out_Color = myTexel * in_Alpha; \n" 
		"}";

	shader2d = sManager->UseShader("shader2d_built_in.vert", "shader2d_built_in.frag", vert2d, frag2d); //load/compile/link
	//attributes
	shader2d->bindAttribLocation(0, "in_Position");
	shader2d->bindAttribLocation(1, "in_Texcoord");

	//2d orthographic projection
	SetMode(Blit3DRenderMode::BLIT2D);

	if(Init != NULL) Init();
	else
	{
		oLog(Level::Severe) << "No Init() provided";
		goto error;
	}

	if(Update == NULL)
	{
		oLog(Level::Severe) << "No Update() provided";
		goto error;
	}

	if(Draw == NULL)
	{
		oLog(Level::Severe) << "No Draw() provided";
		goto error;
	}

	if(DoInput == NULL)
	{
		oLog(Level::Severe) << "No DoInput() provided";
		goto error;
	}
	
	double time = glfwGetTime();
	double prevTime = time;
	double elapsedTime = 0;
	B3D::quitLooping = false;

	//event loop
	switch(threadType)
	{
	case Blit3DThreadModel::SIMPLEMULTITHREADED:
	{
		std::thread t1(SimpleThreadUpdate, Update);

		while(!glfwWindowShouldClose(window))
		{

			Draw();
			// put the stuff we've been drawing onto the display
			glfwSwapBuffers(window);

			B3D::loopMutex.lock();
			if(Sync != NULL) Sync();

			// update other events like input handling 
			glfwPollEvents();
			if(DoJoystick) DoJoystick();
			B3D::loopMutex.unlock();
		}

		B3D::quitLooping = true;
		t1.join();
	}
		break;

	case Blit3DThreadModel::MULTITHREADED:
	{
		std::thread t2(MultiThreadUpdate, Update);

		while(!glfwWindowShouldClose(window))
		{

			Draw();
			// put the stuff we've been drawing onto the display
			glfwSwapBuffers(window);

			// update other events like input handling 
			glfwPollEvents();
			if(DoJoystick) DoJoystick();
		}

		B3D::quitLooping = true;
		t2.join();
	}
		break;

	case Blit3DThreadModel::SINGLETHREADED:
		//event loop
		while(!glfwWindowShouldClose(window))
		{
			time = glfwGetTime();
			elapsedTime = time - prevTime;
			prevTime = time;
						
			Update(elapsedTime);

			Draw();
			// put the stuff we've been drawing onto the display
			glfwSwapBuffers(window);

			// update other events like input handling 
			glfwPollEvents();
			if(DoJoystick) DoJoystick();
		}
		break;
	}

error:
	if(DeInit != NULL) DeInit();

	// close GL context and any other GLFW resources
	glfwTerminate();

	oLog(Level::Info) << "Terminated Blit3D";
	return 0;
}

Sprite *Blit3D::MakeSprite(GLfloat startX, GLfloat startY, GLfloat width, GLfloat height, std::string TextureFileName)
{
	//use a lock gaurd to lock until function returns
	std::lock_guard<std::mutex> lock(spriteMutex);

	//create a new sprite from a bitmap file
	Sprite *sprite =  new Sprite(startX, startY, width, height, TextureFileName, tManager, shader2d);

	//add sprite pointer to the set tracking all allocated sprites
	spriteSet.insert(sprite);

	return sprite;
}

Sprite*Blit3D::MakeSprite(RenderBuffer *rb)
{
	//use a lock gaurd to lock until function returns
	std::lock_guard<std::mutex> lock(spriteMutex);

	//create a new sprite from a renderbuffer
	Sprite *sprite = new Sprite(rb, tManager, shader2d);

	spriteSet.insert(sprite);

	return sprite;
}

void Blit3D::DeleteSprite(Sprite *sprite)
{
	//use a lock gaurd to lock until function returns
	std::lock_guard<std::mutex> lock(spriteMutex);

	std::unordered_set<Sprite *>::iterator it = spriteSet.find(sprite);
	if (it != spriteSet.end())
	{
		//delete the sprite and remove from set
		delete *it;
		spriteSet.erase(it);
	}
	else
	{
		oLog(Level::Warning) << "DeleteSprite() called on non-existant Sprite * " << sprite;
	}
}

BFont *Blit3D::MakeBFont(std::string TextureFileName, std::string widths_file, float fontsize)
{
	return new BFont(TextureFileName, widths_file, fontsize, tManager, shader2d);
}

AngelcodeFont *Blit3D::MakeAngelcodeFontFromBinary32(std::string filename)
{
	//use a lock gaurd to lock until function returns
	std::lock_guard<std::mutex> lock(spriteMutex);

	//create new font
	AngelcodeFont *afont = new AngelcodeFont(filename, tManager, shader2d);
	
	fontSet.insert(afont);
	
	return afont;
}

void Blit3D::DeleteFont(AngelcodeFont *font)
{
	//use a lock gaurd to lock until function returns
	std::lock_guard<std::mutex> lock(fontMutex);

	std::unordered_set<AngelcodeFont *>::iterator it = fontSet.find(font);
	if (it != fontSet.end())
	{
		//delete the sprite and remove from set
		delete *it;
		fontSet.erase(it);
	}
	else
	{
		oLog(Level::Warning) << "DeleteFont() called on non-existant font * " << font;
	}
}

RenderBuffer *Blit3D::MakeRenderBuffer(int width, int height, std::string name)
{
	return new RenderBuffer(width, height, tManager, name, this);
}

void Blit3D::SetMode(Blit3DRenderMode newMode)
{
	if(mode == newMode) return;

	mode = newMode;

	if(mode == Blit3DRenderMode::BLIT3D)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);// Enable Depth Testing for 3D!
		//3D perspective projection
		projectionMatrix = glm::mat4(1.f) * glm::perspective(glm::radians(45.0f), (GLfloat)(screenWidth) / (GLfloat)(screenHeight), nearplane, farplane);
	
		shader2d->use();
		//send matrices to the shader
		shader2d->setUniform("projectionMatrix", projectionMatrix);
		//TODO: make a backup of view matrix and projection matrix.
		shader2d->setUniform("viewMatrix", viewMatrix);

		//send alpha to the shader
		shader2d->setUniform("in_Alpha", 1.f);
		shader2d->setUniform("in_Scale_X", 1.f);
		shader2d->setUniform("in_Scale_Y", 1.f);
	}
	else
	{
		glDisable(GL_CULL_FACE); //not needed for 2D, and this allows flipping sprites by negative scaling
		glDisable(GL_DEPTH_TEST);	// Disable Depth Testing for 2D!
		//2d orthographic projection
		projectionMatrix = glm::mat4(1.f) * glm::ortho(0.f, (float)screenWidth, 0.f, (float)screenHeight, 0.f, 1.f);

		shader2d->use();
		//send matrices to the shader
		shader2d->setUniform("projectionMatrix", projectionMatrix);
//TODO: make a backup of view matrix and projection matrix.
		shader2d->setUniform("viewMatrix", viewMatrix);

		//send alpha to the shader
		shader2d->setUniform("in_Alpha", 1.f);	
		shader2d->setUniform("in_Scale_X", 1.f);
		shader2d->setUniform("in_Scale_Y", 1.f);
	}

}

void Blit3D::SetMode(Blit3DRenderMode newMode, GLSLProgram *shader)
{
	if(mode == newMode) return;

	mode = newMode;

	if(mode == Blit3DRenderMode::BLIT3D)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);// Enable Depth Testing for 3D!
		//3D perspective projection
		projectionMatrix = glm::mat4(1.f) * glm::perspective(glm::radians(45.0f), (GLfloat)(screenWidth) / (GLfloat)(screenHeight), nearplane, farplane);
		
		shader->use();

		//send matrices to the shader
		shader->setUniform("projectionMatrix", projectionMatrix);
		//TODO: make a backup of view matrix and projection matrix.
		shader->setUniform("viewMatrix", viewMatrix);

	}
	else
	{
		glDisable(GL_CULL_FACE); //not needed for 2D, and this allows flipping sprites by negative scaling
		glDisable(GL_DEPTH_TEST);	// Disable Depth Testing for 2D!
		//2d orthographic projection
		projectionMatrix = glm::mat4(1.f) * glm::ortho(0.f, (float)screenWidth, 0.f, (float)screenHeight, 0.f, 1.f);

		shader->use();

		//send matrices to the shader
		shader->setUniform("projectionMatrix", projectionMatrix);
		//TODO: make a backup of view matrix and projection matrix.
		shader->setUniform("viewMatrix", viewMatrix);

		//send alpha to the shader
		shader->setUniform("in_Alpha", 1.f);
		shader->setUniform("in_Scale_X", 1.f);
		shader->setUniform("in_Scale_Y", 1.f);
	}

}

Blit3DRenderMode Blit3D::GetMode(void)
{
	return mode;
}

void Blit3D::Reshape(GLSLProgram *shader)
{
	glViewport(0, 0, (GLsizei)(screenWidth), (GLsizei)(screenHeight));						// Reset The Current Viewport

	projectionMatrix = glm::mat4(1.0); //glLoadIdentity

	if(mode == Blit3DRenderMode::BLIT3D)
	{
		glEnable(GL_DEPTH_TEST);// Enable Depth Testing for 3D!

		projectionMatrix *= glm::perspective(glm::radians(45.0f), (GLfloat)(screenWidth) / (GLfloat)(screenHeight), nearplane, farplane);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);	// Disable Depth Testing for 2D!

		projectionMatrix *= glm::ortho(0.f, (GLfloat)(screenWidth), 0.f, (GLfloat)(screenHeight), 0.f, 1.f); // identical to glOrtho();
	}

	//the projection matrix must be reset in the active shader!
	shader->setUniform("projectionMatrix", projectionMatrix);
	
}

void Blit3D::ReshapFBO(int FBOwidth, int FBOheight, GLSLProgram *shader)
{
	glViewport(0, 0, (GLsizei)(FBOwidth), (GLsizei)(FBOheight));						// Reset The Current Viewport

	projectionMatrix = glm::mat4(1.0); //glLoadIdentity

	if(mode == Blit3DRenderMode::BLIT3D)
	{
		glEnable(GL_DEPTH_TEST);// Enable Depth Testing for 3D!
		
		projectionMatrix *= glm::perspective(glm::radians(45.0f), (float)FBOwidth / (float)FBOheight, nearplane, farplane);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);	// Disable Depth Testing for 2D!

		projectionMatrix *= glm::ortho(0.f, (float)FBOwidth, 0.f, (float)FBOheight, 0.f, 1.f); // identical to glOrtho();
	}
	//send projection matrix
	if(shader == NULL) shader = shader2d;
	shader->setUniform("projectionMatrix", projectionMatrix);
}

