#include "glad/glad.h"  //Include order can matter here
#ifdef __APPLE__
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int screenWidth = 800; 
int screenHeight = 600;  
bool saveOutput = false;
float timePast = 0;

//SJG: Store the object coordinates
//You should have a representation for the state of each object
float objx=0, objy=-1.4, objz=0.8;


bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;
void Win2PPM(int width, int height);

int main(int argc, char *argv[]){
    SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)
    
    //Ask SDL to get a recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	

	
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL);
	float aspect = screenWidth/(float)screenHeight; //aspect ratio (needs to be updated if the window is resized)
  
	//The above window cannot be resized which makes some code slightly easier.
	//Below show how to make a full screen window or allow resizing
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screenWidth, screenHeight, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL); //Boarderless window "fake" full screen

	
	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);
	
	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)){
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}
	
	
	//Load Model 1
	ifstream modelFile;
	modelFile.open("models/cube.txt");
	int numLines = 0;
	modelFile >> numLines;
	float* model1 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model1[i];
	}
	printf("%d\n",numLines);
	int numTris1 = numLines/8;
	modelFile.close();
	
	//Load Model 2
	modelFile.open("models/sphere.txt");
	numLines = 0;
	modelFile >> numLines;
	float* model2 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model2[i];
	}
	printf("%d\n",numLines);
	int numTris2 = numLines/8;
	modelFile.close();
	
	
	//SJG: I load each model in a different array, then concatenate everything in one big array
	//     There are better options, but this works.
	//Concatenate model arrays
	float* modelData = new float[(numTris1+numTris2)*8];
	copy(model1, model1+numTris1*8, modelData);
	copy(model2, model2+numTris2*8, modelData+numTris1*8);
	int totalNumTris = numTris1+numTris2;
	
	SDL_Surface* surface = SDL_LoadBMP("wood.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    //Load the texture into memory
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    SDL_FreeSurface(surface);
	

	//Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

	int shaderProgram = InitShader("vertexTex.glsl", "fragmentTex.glsl");

	glBufferData(GL_ARRAY_BUFFER, totalNumTris*8*sizeof(float), modelData, GL_STATIC_DRAW);

	//Tell OpenGL how to map shader inputs to data
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
		//Attribute, vals/attrib., type, normalized?, stride, offset
		//Binds to VBO current GL_ARRAY_BUFFER 
	glEnableVertexAttribArray(posAttrib);

	//GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);

	GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));

	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one

	glEnable(GL_DEPTH_TEST);  

	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;
	while (!quit){
		while (SDL_PollEvent(&windowEvent)){  //inspect all events in the queue
			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) 
			quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f){ //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
			}
			
			//SJG: Use key input to change the state of the object
			//     We can use the ".mod" flag to see if modifiers such as shift are pressed
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP){ //If "up key" is pressed
				if (windowEvent.key.keysym.mod & KMOD_SHIFT) objx -= .1; //Is shift pressed?
				else objz += .1;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN){ //If "down key" is pressed
				if (windowEvent.key.keysym.mod & KMOD_SHIFT) objx += .1; //Is shift pressed?
				else objz -= .1;
			}
				if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT){ //If "up key" is pressed
				objy -= .1;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT){ //If "down key" is pressed
				objy += .1;
			}
		}
		
		
		if (!saveOutput) timePast = SDL_GetTicks()/1000.f; 
		if (saveOutput) timePast += .07; //Fix framerate at 14 FPS


		glUseProgram(shaderProgram); 
	
		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
					
		
		glm::mat4 view = glm::lookAt(
		glm::vec3(3.f, 0.f, 0.f),  //Cam Position  (this should change as user moves camera)
		glm::vec3(0.0f, 0.0f, 0.0f),  //Look at point
		glm::vec3(0.0f, 0.0f, 1.0f)); //Up
		
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
		
		
		glm::mat4 proj = glm::perspective(3.14f/4, aspect, 1.0f, 10.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
		
		glm::mat4 model;
		model = glm::rotate(model,timePast * .5f * 3.14f/2,glm::vec3(0.0f, 1.0f, 1.0f));
		model = glm::rotate(model,timePast * .5f * 3.14f/4,glm::vec3(1.0f, 0.0f, 0.0f));
		//model = glm::scale(model,glm::vec3(.2f,.2f,.2f)); //An example of scale
		
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
		
		glBindVertexArray(vao);  //Bind the VAO we want for drawing (we only have 1 in this scene)
		
		//SJG: Here we draw only the first object (start at 0, draw numTris1 triangles)  
		glDrawArrays(GL_TRIANGLES, 0, numTris1); //(Primitives, Which VBO, Number of vertices)
		
		//SJG: Let's change the model matrix before the next call to show how to change diffent
		//     models independently  
		model = glm::mat4();
		model = glm::scale(model,glm::vec3(.8f,.8f,.8f));
		model = glm::translate(model,glm::vec3(objx,objy,objz));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
		//SJG: The 2nd model, starts at an offset of numTris1 and has numTris2 triangles
		//      My approach here is not very scalable. You should do better
		glDrawArrays(GL_TRIANGLES, numTris1, numTris2); //(Primitives, Which VBO, Number of vertices)
			
			
		if (saveOutput) Win2PPM(screenWidth,screenHeight);
		

		SDL_GL_SwapWindow(window); //Double buffering
	}
	
	//Clean Up
	glDeleteProgram(shaderProgram);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile)
{
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName)
{
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	} else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders
	
	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}
	
	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	
	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);
	glUseProgram(program);

	return program;
}

void Win2PPM(int width, int height){
	char outdir[10] = "out/"; //Must be defined!
	int i,j;
	FILE* fptr;
    static int counter = 0;
    char fname[32];
    unsigned char *image;
    
    /* Allocate our buffer for the image */
    image = (unsigned char *)malloc(3*width*height*sizeof(char));
    if (image == NULL) {
      fprintf(stderr,"ERROR: Failed to allocate memory for image\n");
    }
    
    /* Open the file */
    sprintf(fname,"%simage_%04d.ppm",outdir,counter);
    if ((fptr = fopen(fname,"w")) == NULL) {
      fprintf(stderr,"ERROR: Failed to open file for window capture\n");
    }
    
    /* Copy the image into our buffer */
    glReadBuffer(GL_BACK);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,image);
    
    /* Write the PPM file */
    fprintf(fptr,"P6\n%d %d\n255\n",width,height);
    for (j=height-1;j>=0;j--) {
      for (i=0;i<width;i++) {
         fputc(image[3*j*width+3*i+0],fptr);
         fputc(image[3*j*width+3*i+1],fptr);
         fputc(image[3*j*width+3*i+2],fptr);
      }
    }
    
    free(image);
    fclose(fptr);
    counter++;
}