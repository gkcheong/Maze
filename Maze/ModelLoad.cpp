//CSCI 5607 OpenGL Tutorial (HW 1/2)
//5 - Model Load

#include "glad/glad.h"  //Include order can matter here
#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
//#include <SDL_ttf.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm>
using namespace std;



bool saveOutput = false;
float timePast = 0;
bool gameMode = true; //true if still playing false if won
//TTF_Font *gFont = NULL;

bool fullscreen = false;
int screen_width = 800;
int screen_height = 600;
int atlas_width = 0;
int atlas_height = 0;
//globals cause im lazy
int width = 0;
int height = 0;

int sizeOfLab = 20;

void Win2PPM(int width, int height);
GLuint InitShader(const char* vertexName, const char* fragmentName);



struct Character {
	float ax; //advance.x
	float ay; //advance.y
	float bw; //bitmap.width
	float bh; //bitmap.height
	float bl; //bitmap_left
	float bt; //bitmap_top
	float tx; //xoffset
};

Character characters[128];


void RenderText(GLuint shaderProgram, string text, GLfloat x, GLfloat y, GLfloat scale, 
	glm::vec3 color, GLuint vaoFont, GLuint vboFont, GLuint tex3) {
	glUseProgram(shaderProgram);
	glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
	GLint uniProj = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.x, color.y, color.z);
	glBindVertexArray(vaoFont);

	string::const_iterator c;
	for (c = text.begin(); c != text.end(); ++c) {
		float x2 = x + characters[*c].bl *scale;
		float y2 = -y - characters[*c].bt*scale;
		float w = characters[*c].bw *scale;
		float h = characters[*c].bh *scale;

		x += characters[*c].ax * scale;
		y += characters[*c].ay * scale;

		/* Skip glyphs that have no pixels */
		if (!w || !h)
			continue;

		float vertices[6][4] = {
			{x2, -y2, characters[*c].tx, 0},
			{x2 + w, -y2, characters[*c].tx + characters[*c].bw / atlas_width,0},
			{x2, -y2 - h, characters[*c].tx, 0 + characters[*c].bh / atlas_height},
			{x2 + w, -y2, characters[*c].tx + characters[*c].bw / atlas_width,0},
			{x2, -y2 -h, characters[*c].tx, characters[*c].bh / atlas_height },
			{x2 + w, -y2 -h, characters[*c].tx + characters[*c].bw / atlas_width ,0 + characters[*c].bh / atlas_height }
		};
		/*float vertices[6][4] = {
			{ x2, -y2, 0, 1 },
			{ x2 + w, -y2, 1,1 },
			{ x2, -y2 - h, 0,0 },
			{ x2 + w, -y2, 1,1 },
			{ x2, -y2 - h, 0,0 },
			{ x2 + w, -y2 - h, 1,0 }
		};*/
		/*Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;

		GLfloat vertices[6][4] = {
			{xpos, ypos + h, 0.0, 0.0},
			{xpos, ypos, 0.0, 1.0},
			{xpos + w, ypos, 1.0, 1.0},
			{xpos, ypos + h, 0.0,0.0},
			{xpos + w, ypos, 1.0, 1.0},
			{xpos + w, ypos + h, 1.0, 0.0}
		};
		*/
		//glActiveTexture(GL_TEXTURE3);
		//glBindTexture(GL_TEXTURE_2D, tex3);
		glUniform1i(glGetUniformLocation(shaderProgram, "text"), 3);

		glBindBuffer(GL_ARRAY_BUFFER, vboFont);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		GLint vertAttrib = glGetAttribLocation(shaderProgram, "vertex");
		glVertexAttribPointer(vertAttrib, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glEnableVertexAttribArray(vertAttrib);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
class Object {
public:
	float objY;
	float objX;
	float objZ;
	char model; //which model to use
	virtual void drawObject(GLuint& shaderProgram, float numTris[]) = 0;
};

class Key : public Object {
public:
	void drawObject(GLuint& shaderProgram, float numTris[]);
	bool rayIntersection(float charX, float charDirX, float charY, float charDirY);
	void setColor(GLuint& shaderProgram);
	bool hasKey = false;
	bool isUsed = false;
	float angle = 0;
	char name;
};

class Goal : public Object {
public:
	bool rayIntersection(float charX, float charDirX, float charY, float charDirY);
	void drawObject(GLuint& shaderProgram, float numTris[]);
};


void Goal::drawObject(GLuint& shaderProgram, float numTris[]) {
	//use the pretzel model for now
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(objX, objY, objZ));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, -1);

	GLint inColorID = glGetUniformLocation(shaderProgram, "inColor");
	glUniform3f(inColorID, 1.0f, 1.0f, 1.0f);
	if (Object::model == 's') { //use square model
		glDrawArrays(GL_TRIANGLES, 0, numTris[0]); //(Primitives, Which VBO, Number of vertices)
	}
	else if(Object::model == 'k'){ //use key model
		glDrawArrays(GL_TRIANGLES, numTris[1], numTris[2]); //(Primitives, Which VBO, Number of vertices)
	}
	else { //use flag model
		glDrawArrays(GL_TRIANGLES, numTris[2], numTris[3]);
	}
}

bool Goal::rayIntersection(float charX, float charDirX, float charY, float charDirY) {
	float txmin;
	float txmax;
	float tymin;
	float tymax;
	//float tzmin;
	//float tzmax;

	float temp = 1.0 / (charDirX - charX);
	if (temp >= 0) {
		txmin = temp * ((objX - 0.20*sizeOfLab) - charX);
		txmax = temp * ((objX + 0.20*sizeOfLab) - charX);
	}
	else {
		txmax = temp * ((objX - 0.20*sizeOfLab) - charX);
		txmin = temp * ((objX + 0.20*sizeOfLab) - charX);
	}
	temp = 1.0 / (charDirY - charY);
	if (temp >= 0) {
		tymin = temp * ((objY - 0.20*sizeOfLab) - charY);
		tymax = temp * ((objY + 0.20*sizeOfLab) - charY);
	}
	else {
		tymax = temp * ((objY - 0.20*sizeOfLab) - charY);
		tymin = temp * ((objY + 0.20*sizeOfLab) - charY);
	}

	/*temp = 1.0 / ray.d[2];
	if (temp >= 0) {
	tzmin = temp * (zmin - ray.p[2]);
	tzmax = temp * (zmax - ray.p[2]);
	}
	else {
	tzmax = temp * (zmin - ray.p[2]);
	tzmin = temp * (zmax - ray.p[2]);
	}*/

	/*if (txmin > tymax || txmin > tzmax || tymin > txmax || tymin > tzmax
	|| tzmin > txmax || tzmin > tymax) {
	return false;
	}
	else {
	return true;
	}*/
	if (txmin > tymax || tymin > txmax) {
		return false;
	}
	else {
		return true;
	}
}
class Wall : public Object {
public:
	void drawObject(GLuint& shaderProgram, float numTris[]);
};

class Door : public Wall {
public:
	void drawObject(GLuint& shaderProgram, float numTris[]);
	bool rayIntersection(float charX, float charDirX, float charY, float charDirY);
	void setColor(GLuint& shaderProgram);
	bool isOpened = false;
	float timeOpened = 0;
	float timeEnd = 0; //5 seconds of opening
	char name;
};

class Floor : public Object {
public:
	void drawObject(GLuint& shaderProgram, float numTris[]);
};


void Key::setColor(GLuint& shaderProgram) {
	GLint inColorID = glGetUniformLocation(shaderProgram, "inColor");
	switch (name) {
	case 'a':
		glUniform3f(inColorID, 1.0f, 0.32f, 0.32f);
		break;
	case 'b':
		glUniform3f(inColorID, 0.13f, 0.55f, 0.13f);
		break;
	case 'c':
		glUniform3f(inColorID, 0.0f, 0.0f, 1.0f);
		break;
	case 'd':
		glUniform3f(inColorID, 1.00f, 1.0f, 0.0f);
		break;
	case 'e':
		glUniform3f(inColorID, 0.8f, 0.36f, 0.36f);
		break;
	}
}

bool Key::rayIntersection(float charX, float charDirX, float charY, float charDirY) {
	float txmin;
	float txmax;
	float tymin;
	float tymax;
	//float tzmin;
	//float tzmax;

	float temp = 1.0 / (charDirX - charX);
	if (temp >= 0) {
		txmin = temp * ((objX - 0.20*sizeOfLab) - charX);
		txmax = temp * ((objX + 0.20*sizeOfLab) - charX);
	}
	else {
		txmax = temp * ((objX - 0.20*sizeOfLab) - charX);
		txmin = temp * ((objX + 0.20*sizeOfLab) - charX);
	}
	temp = 1.0 / (charDirY - charY);
	if (temp >= 0) {
		tymin = temp * ((objY - 0.20*sizeOfLab) - charY);
		tymax = temp * ((objY + 0.20*sizeOfLab) - charY);
	}
	else {
		tymax = temp * ((objY - 0.20*sizeOfLab) - charY);
		tymin = temp * ((objY + 0.20*sizeOfLab) - charY);
	}

	/*temp = 1.0 / ray.d[2];
	if (temp >= 0) {
	tzmin = temp * (zmin - ray.p[2]);
	tzmax = temp * (zmax - ray.p[2]);
	}
	else {
	tzmax = temp * (zmin - ray.p[2]);
	tzmin = temp * (zmax - ray.p[2]);
	}*/

	/*if (txmin > tymax || txmin > tzmax || tymin > txmax || tymin > tzmax
	|| tzmin > txmax || tzmin > tymax) {
	return false;
	}
	else {
	return true;
	}*/
	if (txmin > tymax || tymin > txmax) {
		return false;
	}
	else {
		return true;
	}
}

void Key::drawObject(GLuint& shaderProgram, float numTris[]) {
	//dont draw object
	if (isUsed) {
		//do not draw object
		return;
	}
	//use the pretzel model for now
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(objX, objY, objZ));
	model = glm::scale(model, glm::vec3(0.7f, 0.7f, 1.0f));
	model = glm::rotate(model, 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, -angle, glm::vec3(1.0f, 0.0f, 0.0f)); //trial and error fine tuned
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, -1);

	setColor(shaderProgram);

	if (Object::model == 's') { //use square model
		glDrawArrays(GL_TRIANGLES, 0, numTris[0]); //(Primitives, Which VBO, Number of vertices)
	}
	else if (Object::model == 'k') { //use key model
		glDrawArrays(GL_TRIANGLES, numTris[1], numTris[2]); //(Primitives, Which VBO, Number of vertices)
	}
	else { //use flag model
		glDrawArrays(GL_TRIANGLES, numTris[2], numTris[3]);
	}

}
void Wall::drawObject(GLuint& shaderProgram, float numTris[]) {
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(objX, objY, objZ));
	model = glm::scale(model, glm::vec3(float(sizeOfLab), float(sizeOfLab), float(sizeOfLab)));
	//model = glm::rotate(model,timePast * 3.14f/2,glm::vec3(0.0f, 1.0f, 1.0f));
	//model = glm::rotate(model,timePast * 3.14f/4,glm::vec3(1.0f, 0.0f, 0.0f));
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, 0);

	

	if (Object::model == 's') { //use square model
		glDrawArrays(GL_TRIANGLES, 0, numTris[0]); //(Primitives, Which VBO, Number of vertices)
	}
	else if (Object::model == 'k') { //use key model
		glDrawArrays(GL_TRIANGLES, numTris[1], numTris[2]); //(Primitives, Which VBO, Number of vertices)
	}
	else { //use flag model
		glDrawArrays(GL_TRIANGLES, numTris[2], numTris[3]);
	}
}



void Door::setColor(GLuint& shaderProgram) {
	GLint inColorID = glGetUniformLocation(shaderProgram, "inColor");
	switch (name) {
	case 'A':
		glUniform3f(inColorID, 1.0f, 0.32f, 0.32f);
		break;
	case 'B' :
		glUniform3f(inColorID, 0.13f, 0.55f, 0.13f);
		break;
	case 'C':
		glUniform3f(inColorID, 0.0f, 0.0f, 1.0f);
		break;
	case 'D':
		glUniform3f(inColorID, 1.00f, 1.0f, 0.0f);
		break;
	case 'E':
		glUniform3f(inColorID,0.8f, 0.36f, 0.36f);
		break;
	}
}

void Door::drawObject(GLuint& shaderProgram, float numTris[]) {
	if (isOpened && (timeEnd-timeOpened) > 5) {
		//do not draw object
		return;
	}
	else {
		setColor(shaderProgram);
		if (isOpened) {
			timeEnd = timePast;
			if (timeEnd > timeOpened) {
				objZ = 0.5*sizeOfLab - sizeOfLab / 5.0 * (timeEnd - timeOpened);
			}
		}

		//not opened draw with overwritten colors

		glm::mat4 model;
		model = glm::translate(model, glm::vec3(objX, objY, objZ));
		model = glm::scale(model, glm::vec3(float(sizeOfLab), float(sizeOfLab), float(sizeOfLab)));
		//model = glm::rotate(model,timePast * 3.14f/2,glm::vec3(0.0f, 1.0f, 1.0f));
		//model = glm::rotate(model,timePast * 3.14f/4,glm::vec3(1.0f, 0.0f, 0.0f));
		GLint uniModel = glGetUniformLocation(shaderProgram, "model");
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

		GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
		glUniform1i(uniTexID, 2);



		if (Object::model == 's') { //use square model
			glDrawArrays(GL_TRIANGLES, 0, numTris[0]); //(Primitives, Which VBO, Number of vertices)
		}
		else if (Object::model == 'k') { //use key model
			glDrawArrays(GL_TRIANGLES, numTris[1], numTris[2]); //(Primitives, Which VBO, Number of vertices)
		}
		else { //use flag model
			glDrawArrays(GL_TRIANGLES, numTris[2], numTris[3]);
		}
	}
}

bool Door::rayIntersection(float charX,float charDirX,float charY,float charDirY) {
	float txmin;
	float txmax;
	float tymin;
	float tymax;
	//float tzmin;
	//float tzmax;

	float temp = 1.0 / (charDirX - charX);
	if (temp >= 0) {
		txmin = temp * ((objX-0.5*sizeOfLab) - charX);
		txmax = temp * ((objX+0.5*sizeOfLab) - charX);
	}
	else {
		txmax = temp * ((objX-0.5*sizeOfLab) - charX);
		txmin = temp * ((objX+0.5*sizeOfLab) - charX);
	}
	temp = 1.0 / (charDirY - charY);
	if (temp >= 0) {
		tymin = temp * ((objY-0.5*sizeOfLab) - charY);
		tymax = temp * ((objY + 0.5*sizeOfLab) - charY);
	}
	else {
		tymax = temp * ((objY - 0.5*sizeOfLab) - charY);
		tymin = temp * ((objY + 0.5*sizeOfLab) - charY);
	}

	/*temp = 1.0 / ray.d[2];
	if (temp >= 0) {
		tzmin = temp * (zmin - ray.p[2]);
		tzmax = temp * (zmax - ray.p[2]);
	}
	else {
		tzmax = temp * (zmin - ray.p[2]);
		tzmin = temp * (zmax - ray.p[2]);
	}*/

	/*if (txmin > tymax || txmin > tzmax || tymin > txmax || tymin > tzmax
		|| tzmin > txmax || tzmin > tymax) {
		return false;
	}
	else {
		return true;
	}*/
	if (txmin > tymax  || tymin > txmax) {
		return false;
	}
	else {
		return true;
	}
}

void Floor::drawObject(GLuint& shaderProgram, float numTris[]) {
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(objX, objY, objZ));
	//compressed in z axis
	model = glm::scale(model, glm::vec3(float(sizeOfLab), float(sizeOfLab), float(sizeOfLab)));
	//model = glm::rotate(model,timePast * 3.14f/2,glm::vec3(0.0f, 1.0f, 1.0f));
	//model = glm::rotate(model,timePast * 3.14f/4,glm::vec3(1.0f, 0.0f, 0.0f));
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	glUniform1i(uniTexID, 1);

	if (Object::model == 's') { //use square model
		glDrawArrays(GL_TRIANGLES, 0, numTris[0]); //(Primitives, Which VBO, Number of vertices)
	}
	else if (Object::model == 'k') { //use key model
		glDrawArrays(GL_TRIANGLES, numTris[1], numTris[2]); //(Primitives, Which VBO, Number of vertices)
	}
	else { //use flag model
		glDrawArrays(GL_TRIANGLES, numTris[2], numTris[3]);
	}
}



bool checkCollision(float x, float y, float charX, float charY, char* map, int direction,
		vector<Door*>& doorList, vector<Key*>& keyList, Goal* goal, bool& freeHand) {

	//holding nothing
	float finalX = charX + direction*1.5*x + 0.5*sizeOfLab;
	//add some offset to give for a bounding box
	//prevents `staring' into the wall
	float finalY = charY + direction*1.5*y + 0.5*sizeOfLab;

	//holding something
	if (!freeHand) {
		//prevents the thing you are holding to collide with the wall
		finalX += direction*5.0*x;
		finalY += direction*5.0*y;

	}
	
	finalX /= sizeOfLab;
	finalY /= sizeOfLab;
	//if collide
	//might not need floor
	if (floor(finalY) < 0 || floor(finalY) >= height || floor(finalX) < 0 || floor(finalX) >= width) {
		//out of bounds
		printf("Out of bounds at at %i,%i\n", int((width - 1) -floor(finalX)), int(floor(finalY)));
		return true;
	}
	switch (map[int(floor(finalY))*width + ((width - 1) - int(floor(finalX)))]) {
	case 'W' :
		printf("Collided with wall at %f,%f\n", finalX, finalY);
		return true;
		break;
	case 'G' :
		if (charX + direction*1.5*x < goal->objX + 0.1 * sizeOfLab
			&& charX + direction*1.5*x > goal->objX - 0.1 * sizeOfLab) {
			if (charY + direction*1.5*y < goal->objY + 0.1 * sizeOfLab
				&& charY + direction*1.5*y > goal->objY - 0.1 * sizeOfLab) {
				return true;
			}
		}
		printf("Reached Goal\n");
		//gameMode = false;
		return false; 
		//we let the player step into the goal but remove all controls after
	case 'A' :
	case 'B' :
	case 'C' :
	case 'D' :
	case 'E' :
		printf("Collided with door at %f,%f\n", finalX, finalY);
		for (int i = 0; i < doorList.size(); ++i) {
			//find the door we are dealing with
			if (doorList[i]->name == map[int(floor(finalY))*width + ((width - 1) - int(floor(finalX)))]) {
				//found door
				//if already open skip all checks
				if (doorList[i]->isOpened && (doorList[i]->timeEnd - doorList[i]->timeOpened) > 5) {
					printf("Door is opened\n");
					return false;
				}
				
				/*
				//commented out can no longer open doors by bumping into them
				//if holding a key
				//now iterate through key
				if (!freeHand) {
					for (int j = 0; j < keyList.size(); ++j) {
						if (doorList[i]->name == char(toupper(keyList[i]->name))) {
							freeHand = true; //free hand
							keyList[i]->hasKey = false; //remove key
							keyList[i]->isUsed = true;
							doorList[i]->isOpened = true;
						}
					}
				}*/

				//door is closed
				printf("Door is closed\n");
				return true;
				
			}
		}
		//should not reach here
		printf("Error with door\n");
		break;
	case 'a' :
	case 'b' :
	case 'c' :
	case 'd' :
	case 'e' :
		for (int i = 0; i < keyList.size(); ++i) {
			if (keyList[i]->name == map[int(floor(finalY))*width + ((width - 1) - int(floor(finalX)))]) {
				//printf("Do we at least have the same name?\n");
				//printf("Has key?%i\n", keyList[i]->hasKey);
				//printf("Which key?%c\n", keyList[i]->name);
				//printf("Free hand?%i\n", freeHand);
				if (keyList[i]->hasKey) {
					printf("Key is taken\n");
					return false;
				}
				else if (keyList[i]->isUsed) {
					printf("Key is used\n");
					return false;
				}
				if (charX + direction*1.5*x < keyList[i]->objX + 0.2 * sizeOfLab 
					&& charX + direction*1.5*x > keyList[i]->objX - 0.2 * sizeOfLab) {
					if (charY + direction*1.5*y < keyList[i]->objY + 0.2 * sizeOfLab 
						&& charY + direction*1.5*y > keyList[i]->objY - 0.2 * sizeOfLab) {
						return true;
					}
					else {
						return false;
					}
				}
				else {
					return false;
				}
				
			}
		}
		//should not reach here
		printf("Error with key\n");
		break;
	}
	return false;
	/*else if (map[int(floor(finalY))*width + ((width-1) - int(floor(finalX)))] == 'W') {
		printf("Collided with wall at %f,%f\n", finalX, finalY);
		//printf("the character is %c\n", map[19] );
		return true;
	}
	else if (map[int(floor(finalY))*width + ((width - 1) - int(floor(finalX)))] == 'A') {

	} 
	else {
		//if free
		return false;
	}*/

}

int main(int argc, char *argv[]) {

	float charX = 2.5;
	float charY = 2.5f;
	float charZ = -10.0f;
	float charDirX = 0.0f;
	float charDirY = 1.0f;
	float charRightX = 1.0f;
	float charRightY = 0.0;

	vector<Object*> objList;
	vector<Door*> doorList; //"Shared vector like in c+++11 
	//does not own object so does not delete
	vector<Key*> keyList;
	ifstream mapData;
	if (argc == 1) {
		mapData.open("SpiralOfTurmoil.txt");
	}
	else {
		mapData.open(argv[1]);
	}
	
	if (!mapData.is_open()) {
		cout << "Failure to open map file" << endl;
	}

	mapData >> width >> height;

	//need to change this eventually, what surrounds the map?
	char* map = new char[width*height];
	Goal* goal = new Goal;
	//dont need this
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			mapData >> map[i*width + j];
			if (map[i*width + j] == 'W'|| map[i*width + j] == 'w') {//w fake passasble walls
				//initialize a wall at height and width
				Wall* temp = new Wall;
				temp->objY = i*sizeOfLab;
				temp->objX = ((width -1 ) - j)*sizeOfLab;
				temp->objZ = 0.5*sizeOfLab;
				temp->model = 's';
				objList.push_back(temp);
			}
			else if (map[i*width + j] == 'S' || map[i*width + j] == 's') {
				//put starting locations
				charX = ((width - 1) - j) * sizeOfLab;
				charY = i * sizeOfLab;
				charZ = 7;
				charDirX = charX;
				charDirY = charY-1;
				charRightX = charX - 1;
				charRightY = charY;
				printf("Character initialized at %f,%f\n", charX, charY);
				printf("Character facing point %f,%f\n", charDirX, charDirY);
				printf("Character's right %f,%f\n", charRightX, charRightY);
			}
			else if (map[i*width + j] == 'A' || map[i*width + j] == 'B'
				|| map[i*width + j] == 'C' || map[i*width + j] == 'D'
				|| map[i*width + j] == 'E') {
				//initialize a door at height and width
				Door* temp = new Door;
				temp->objY = i*sizeOfLab;
				temp->objX = ((width - 1) - j)*sizeOfLab;
				temp->objZ = 0.5*sizeOfLab;
				temp->isOpened = false;
				temp->name = map[i*width + j];
				temp->model = 's';
				objList.push_back(temp);
				doorList.push_back(temp);
			}
			else if (map[i*width + j] == 'a' || map[i*width + j] == 'b'
				|| map[i*width + j] == 'c' || map[i*width + j] == 'd'
				|| map[i*width + j] == 'e') {
				Key* temp = new Key;
				temp->objY = i*sizeOfLab;
				temp->objX = ((width - 1) - j)*sizeOfLab;
				temp->objZ = 7;
				temp->name = map[i*width + j];
				temp->model = 'k';
				objList.push_back(temp);
				keyList.push_back(temp);
			}
			else if (map[i*width + j] == 'G') {
				goal->objY = i*sizeOfLab;
				goal->objX = ((width - 1) - j)*sizeOfLab;
				goal->objZ = 7;
				goal->model = 'f';
				objList.push_back(goal);
			}
			else {
				//do nothing
			}
		}
	}
	mapData.close();

	//add floors. We will use squares for now
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			Floor* temp = new Floor;
			temp->objY = i * sizeOfLab;
			temp->objX = ((width - 1) - j) * sizeOfLab;
			temp->objZ = -0.5*sizeOfLab;
			temp->model = 's';
			objList.push_back(temp);
		}
	}

	//add boundary. Also squares
	for (int i = -1; i < height + 1; i += (height + 1)) {
		for (int j = 0; j < width; ++j) {
			Wall * temp = new Wall;
			temp->objY = i*sizeOfLab;
			temp->objX = ((width - 1) - j)*sizeOfLab;
			temp->objZ = 0.5*sizeOfLab;
			temp->model = 's';
			objList.push_back(temp);
			printf("Boundary initialized at %f,%f\n", temp->objX, temp->objY);
		}
	}

	for (int i = -1; i < height + 1; ++i) {
		for (int j = -1; j < width + 1; j += (width + 1)) {
			Wall * temp = new Wall;
			temp->objY = i*sizeOfLab;
			temp->objX = ((width - 1) - j)*sizeOfLab;
			temp->objZ = 0.5*sizeOfLab;
			temp->model = 's';
			objList.push_back(temp);
			printf("Boundary initialized at %f,%f\n", temp->objX, temp->objY);
		}
	}

  SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

  //Ask SDL to get a recent version of OpenGL (3.2 or greater)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  //Create a window (offsetx, offsety, width, height, flags)
  SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screen_width, screen_height, SDL_WINDOW_OPENGL);
  float aspect = screen_width/(float)screen_height; //aspect ratio (needs to be updated if the window is resized)

  //The above window cannot be resized which makes some code slightly easier.
	//Below show how to make a full screen window or allow resizing
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screen_width, screen_height, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screen_width, screen_height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL); //Boarderless window "fake" full screen


	
	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);
	
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

  
	
	//load Texture 0 (brick) //Wall
	SDL_Surface* surface = SDL_LoadBMP("tex/brick.bmp");
	if (surface == NULL) {
		printf("Surface 0 not loaded: %s", SDL_GetError());
		exit(1);
	}

	GLuint tex0;
	glGenTextures(1, &tex0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//load texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
		GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);

	//load Texture 1 (wood) //floor
	surface = SDL_LoadBMP("tex/wood.bmp");

	GLuint tex1;
	glGenTextures(1, &tex1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
		GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	SDL_FreeSurface(surface);

	
	surface = SDL_LoadBMP("tex/grunge.bmp");
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex2;
	glGenTextures(1, &tex2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex2);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
		GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);
	
	GLuint shaderProgram = InitShader("vertex.glsl", "fragment.glsl");
	GLuint fontProgram = InitShader("vertexFont.glsl", "fragmentFont.glsl");
  

	//Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context
	

	//GLuint vbo[2];
	//glGenBuffers(2, vbo);  //Create 1 buffer called vbo
	//glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	

	ifstream modelFile;
	modelFile.open("models/cube.txt");
	int numLines[4];
	numLines[0] = 0;
	modelFile >> numLines[0];
	float* modelData1 = new float[numLines[0]];
	for (int i = 0; i < numLines[0]; i++) {
		modelFile >> modelData1[i];
	}
	printf("Model line count: %d\n", numLines[0]);
	modelFile.close();
	float numTris[4];
	numTris[0] = numLines[0] / 8;
	//glBufferData(GL_ARRAY_BUFFER, numTris[0]*8 * sizeof(float), modelData1, GL_STATIC_DRAW); //upload vertices to vbo

	//glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

	//upload the second buffer
	modelFile.open("models/knot.txt");
	modelFile >> numLines[1];
	float* modelData2 = new float[numLines[1]];
	for (int i = 0; i < numLines[1]; i++) {
		modelFile >> modelData2[i];
	}
	printf("Model2 line count: %d\n", numLines[1]);
	modelFile.close();
	numTris[1] = numLines[1] / 8;

	modelFile.open("models/key.txt");
	modelFile >> numLines[2];
	float* modelData3 = new float[numLines[2]];
	for (int i = 0; i < numLines[2]; i++) {
		modelFile >> modelData3[i];
	}
	printf("Model3 line count: %d\n", numLines[2]);
	modelFile.close();
	numTris[2] = numLines[2] / 8;

	modelFile.open("models/flag.txt");
	modelFile >> numLines[3];
	float* modelData4 = new float[numLines[3]];
	for (int i = 0; i < numLines[3]; i++) {
		modelFile >> modelData4[i];
	}
	printf("Model4 line count: %d\n", numLines[3]);
	modelFile.close();
	numTris[3] = numLines[3] / 8;

	//glBufferData(GL_ARRAY_BUFFER, numTris[1] *8* sizeof(float), modelData2, GL_STATIC_DRAW); //upload vertices to vbo
	float* modelData = new float[numLines[0] + numLines[1] + numLines[2] + numLines[3]];
	copy(modelData1, modelData1 + numLines[0], modelData);
	copy(modelData2, modelData2 + numLines[1], modelData + numLines[0]);
	copy(modelData3, modelData3 + numLines[2], modelData + numLines[1]);
	copy(modelData4, modelData4 + numLines[3], modelData + numLines[2]);
	delete[] modelData1;
	delete[] modelData2;
	delete[] modelData3;
	delete[] modelData4;
	int totalNumTris = numTris[0] + numTris[1] + numTris[2] + numTris[3];
	//printf("Tris1: %f\n", numTris[0]);
	//printf("Tris2: %f\n", numTris[1]);
	//printf("Total Tris: %d\n", totalNumTris);
	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, totalNumTris * 8*sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

  //Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, normalized?, stride, offset
	  //Binds to VBO current GL_ARRAY_BUFFER 
	glEnableVertexAttribArray(posAttrib);
	
	//GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);
	GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

	GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

  glBindVertexArray(0); //Unbind the VAO

  //Maybe we need a second VAO, e.g., some of our models are stored in a second format
  //GLuint vao2;  
	//glGenVertexArrays(1, &vao2); //Create the VAO
  //glBindVertexArray(vao2); //Bind the above created VAO to the current context
  //  Creat VBOs ... 
  //  Set-up attributes ...
  //glBindVertexArray(0); //Unbind the VAO
	

	glEnable(GL_DEPTH_TEST);  
	
	/*TTF_Init(); //initialize the font writing
	gFont = TTF_OpenFont("lazy.ttf", 50); //load font
	if (gFont == NULL) {
		printf("Fail to load  font\n");
	}
	else {
		printf("Font loaded successfully\n");
	}
	SDL_Color textColor = { 255,255,255 };
	SDL_Color bgColor = { 0,255,255 };
	surface = TTF_RenderText_Shaded(gFont, "Press E to interact lfhjlasdkjfk", textColor, bgColor);
	*/
	/*SDL_Rect Message_rect; //create a rect
	Message_rect.x = 100;  //controls the rect's x coordinate 
	Message_rect.y = 100; // controls the rect's y coordinte
	Message_rect.w = 400; // controls the width of the rect
	Message_rect.h = 400; // controls the height of the rect
	*/
	/*GLint colors = surface->format->BytesPerPixel;
	GLenum texture_format;
	if (colors == 4) {   // alpha
		if (surface->format->Rmask == 0x000000ff)
			texture_format = GL_RGBA;
		else
			texture_format = GL_BGRA;
	}
	else {             // no alpha
		if (surface->format->Rmask == 0x000000ff)
			texture_format = GL_RGB;
		else
			texture_format = GL_BGR;
	}

	GLuint tex2;
	glGenTextures(1, &tex2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex2);
	*/
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//load texture into memory
	/*glTexImage2D(GL_TEXTURE_2D, 0, colors, surface->w, surface->h, 0,
		texture_format, GL_UNSIGNED_BYTE, surface->pixels);
	//glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);*/
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		printf("ERROR: failure in init free type\n");
	}

	FT_Face face;
	if (FT_New_Face(ft, "arial.ttf", 0, &face)) {
		printf("Error: loading font failure\n");
	}
	FT_Set_Pixel_Sizes(face, 0, 24);
	FT_GlyphSlot g = face->glyph;
	

	//
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //might break everything
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	for (int i = 0; i < 128; i++) {
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			printf("Fail to load glyph\n");
			continue;
		}
		g = face->glyph;
		atlas_width += g->bitmap.width;
		atlas_height = max(atlas_height, (int)g->bitmap.rows);
	}
	printf("this is atlas width %i \n", atlas_width);
	printf("this is atlas height %i \n", atlas_height);
	GLuint tex3;
	glActiveTexture(GL_TEXTURE3);
	glGenTextures(1, &tex3);
	glBindTexture(GL_TEXTURE_2D, tex3);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_width, atlas_height , 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	int xoffset = 0;
	for (int i = 0; i < 128; i++) {
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			printf("Fail to load glyph\n");
			continue;
		}
		g = face->glyph;
		characters[i].ax = g->advance.x >> 6;
		characters[i].ay = g->advance.y >> 6;
		characters[i].bw = g->bitmap.width;
		characters[i].bh = g->bitmap.rows;
		characters[i].bl = g->bitmap_left;
		characters[i].bt = g->bitmap_top;
		characters[i].tx = (float)xoffset / atlas_width;
		glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
		/*if (i == 65) {
			printf("bitmap value\n");
			for (int j = 0; j < g->bitmap.width; ++j) {
				for (int k = 0; k < g->bitmap.rows; ++k) {
					printf("%i ", *(g->bitmap.buffer + (j*g->bitmap.rows) + k));
				}
				printf("\n");
			}
		}*/
		xoffset += g->bitmap.width;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D); //if i turned this off then there will be no font written out??
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	//glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	//glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	

	/*for (GLubyte c = 0; c < 128; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			printf("Fail to load glyph\n");
			continue;
		}
		glActiveTexture(GL_TEXTURE5);
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, 
			face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);*/


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glm::mat4 projection = glm::ortho(0.0f, float(screen_width), 0.0f, float(screen_height));

	GLuint vaoFont, vboFont;
	glGenVertexArrays(1, &vaoFont);
	glBindVertexArray(vaoFont);
	glGenBuffers(1, &vboFont);
	glBindBuffer(GL_ARRAY_BUFFER, vboFont);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	printf("The amount of keys loaded: %i\n", keyList.size());
	for (int i = 0; i < keyList.size(); i++) {
		printf("This key is : %c\n", keyList[i]->name);
	}
	bool freeHand = true;
	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	while (true){

		//game mode controls
		if (gameMode) {
			//need second if since SDL_PollEvents eats user inputs
			if (SDL_PollEvent(&windowEvent)) {
				if (windowEvent.type == SDL_QUIT) break;
				//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
				//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
				if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
					break; //Exit event loop
				if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) { //If "f" is pressed
					fullscreen = !fullscreen;
					SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen
				}
				if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT) {
					//charX += 0.1;
					//disgusting tank controls-rotate right
					float x = (charDirX - charX);
					float y = (charDirY - charY);
					float theta = 0.05;
					charDirX = x*cos(-theta) - y*sin(-theta);
					charDirX += charX;
					charDirY = x*sin(-theta) + y *cos(-theta);
					charDirY += charY;

					float x2 = (charRightX - charX);
					float y2 = (charRightY - charY);
					charRightX = x2*cos(-theta) - y2*sin(-theta);
					charRightX += charX;
					charRightY = x2*sin(-theta) + y2 *cos(-theta);
					charRightY += charY;

					//printf("The key pressed is right: face right %f,%f\n", charRightX, charRightY);
					if (!freeHand) {//carrying a key
									//find out which key and take it
						for (int i = 0; i < keyList.size(); i++) {
							if (keyList[i]->hasKey) {

								keyList[i]->objX = charDirX + x + x2;
								keyList[i]->objY = charDirY + y + y2;
								keyList[i]->angle += -theta;
								break;
							}
						}
					}
				}
				if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT) {
					//charX -= 0.1;
					//disgusting tank controls-rotate left
					float x = (charDirX - charX);
					float y = (charDirY - charY);
					float theta = 0.05;
					charDirX = x*cos(theta) - y*sin(theta);
					charDirX += charX;
					charDirY = x*sin(theta) + y*cos(theta);
					charDirY += charY;

					float x2 = (charRightX - charX);
					float y2 = (charRightY - charY);
					charRightX = x2*cos(theta) - y2*sin(theta);
					charRightX += charX;
					charRightY = x2*sin(theta) + y2 *cos(theta);
					charRightY += charY;

					if (!freeHand) {//carrying a key
									//find out which key and take it
						for (int i = 0; i < keyList.size(); i++) {
							if (keyList[i]->hasKey) {
								
								//orders matter here. Rotating key
								keyList[i]->objX = charDirX + x + x2;
								keyList[i]->objY = charDirY + y + y2;
								keyList[i]->angle += theta;
								break;
							}
						}
					}
				}
				if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP) {
					//disgusting tank controls- move at viewing dir
					//charY += 0.1;
					//printf("The key pressed is up %f,%f\n", charX, charY);
					float x = (charDirX - charX);
					float y = (charDirY - charY);
					float x2 = (charRightX - charX);
					float y2 = (charRightY - charY);
					
					if (checkCollision(x, y, charX, charY, map, 1, doorList, keyList,goal, freeHand)) {
						//do nothing
					}
					else {
						charX += 1 * x;
						charDirX += 1 * x;
						charY += 1 * y;
						charDirY += 1 * y;
						charRightX += 1 * x;
						charRightY += 1 * y;

						//printf("The key pressed is up: face right %f,%f\n", charRightX, charRightY);
						if (!freeHand) {//carrying a key
										//find out which key and take it
							for (int i = 0; i < keyList.size(); i++) {
								if (keyList[i]->hasKey) {
									keyList[i]->objX = charDirX + x + x2;
									keyList[i]->objY = charDirY + y + y2;
									break;
								}
							}
						}
					}

				}
				if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN) {
					//charY -= 0.1;
					//disgusting tank controls- move back at viewing dir
					float x = (charDirX - charX);
					float y = (charDirY - charY);
					float x2 = (charRightX - charX);
					float y2 = (charRightY - charY);
					if (checkCollision(x, y, charX, charY, map, -1, doorList, keyList,goal, freeHand)) {
						//do nothing
					}
					else {

						charX -= 1 * x;
						charDirX -= 1 * x;
						charY -= 1 * y;
						charDirY -= 1 * y;
						charRightX -= 1 * x;
						charRightY -= 1 * y;

						if (!freeHand) {//carrying a key
										//find out which key and take it
							for (int i = 0; i < keyList.size(); i++) {
								if (keyList[i]->hasKey) {
									keyList[i]->objX = charDirX + x + x2;
									keyList[i]->objY = charDirY + y + y2;
									break;
								}
							}
						}
					}

				}
				if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_e) {
					//printf("We opened door\n");
					if (!freeHand) {
						char keyName;
						//what key do we have
						int keyId = 0;
						for (int j = 0; j < keyList.size(); ++j) {
							if (keyList[j]->hasKey) {
								keyName = char(toupper(keyList[j]->name));
								keyId = j;
							}
						}
						//Are we standing near a door with the same name?
						for (int i = 0; i < doorList.size(); ++i) {
							if (doorList[i]->name == keyName) {
								printf("the key name is %c\n", keyName);
								if (charX < doorList[i]->objX + 1 * sizeOfLab && charX > doorList[i]->objX - 1 * sizeOfLab) {
									if (charY < doorList[i]->objY + 1 * sizeOfLab && charY > doorList[i]->objY - 1 * sizeOfLab) {
										//Are we facing the door?
										if (doorList[i]->rayIntersection(charX, charDirX, charY, charDirY)) {
											printf("the %c key is used\n", keyName);
											freeHand = true; //free hand
											keyList[keyId]->hasKey = false; //remove key
											keyList[keyId]->isUsed = true;
											doorList[i]->isOpened = true;
											doorList[i]->timeOpened = timePast;
											printf("the %c key is status %i,%i\n", keyName, keyList[i]->isUsed, freeHand);
											break;
										}
									}
								}
							}
						}
					}//if(!freeHand)
					else {//we are picking up a key
						for (int i = 0; i < keyList.size(); ++i) {
							//we are standing in the same square as the key
							if (charX < keyList[i]->objX + 0.5 * sizeOfLab && charX > keyList[i]->objX - 0.5 * sizeOfLab) {
								if (charY < keyList[i]->objY + 0.5 * sizeOfLab && charY > keyList[i]->objY - 0.5 * sizeOfLab) {
									//Are we facing the door?
									if (keyList[i]->rayIntersection(charX, charDirX, charY, charDirY)) {
										keyList[i]->hasKey = true;
										freeHand = false;
										float x = (charDirX - charX);
										float y = (charDirY - charY);
										float x2 = (charRightX - charX);
										float y2 = (charRightY - charY);
										printf("And so the player takes the key\n");
										keyList[i]->objX = charX + 2*x + x2;
										keyList[i]->objY = charY + 2*y + y2;
										keyList[i]->objZ += 1;
										break;
									}
								}
							}					
						}
					}
					//if nothing else then must have hit the goal
					if (charX < goal->objX + 0.5 * sizeOfLab && charX > goal->objX - 0.5 * sizeOfLab) {
						if (charY < goal->objY + 0.5 * sizeOfLab && charY > goal->objY - 0.5 * sizeOfLab) {
							if (goal->rayIntersection(charX, charDirX, charY, charDirY)) {
								gameMode = false;
							}
						}
					}
					//printf("Is hand freed?%i\n", freeHand);
				}//key E
			}
		}
	  //menu mode controls. i.e. victory
	  else {
		  if (SDL_PollEvent(&windowEvent)) {
			  if (windowEvent.type == SDL_QUIT) break;
			  //List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			  //Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			  if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				  break; //Exit event loop
			  if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) { //If "f" is pressed
				  fullscreen = !fullscreen;
				  SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen
			  }
		  }
	  }
	  
      // Clear the screen to default color
      glClearColor(.2f, 0.4f, 0.8f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	  
		glUseProgram(shaderProgram);
	  for (int i = 0; i < objList.size(); i++) {
		  if (!saveOutput) timePast = SDL_GetTicks() / 1000.f;
		  if (saveOutput) timePast += .07; //Fix framerate at 14 FPS
		  
		  GLint camPos = glGetUniformLocation(shaderProgram, "inCamPos");
		  glUniform3f(camPos, charX, charY, charZ);

		  glm::mat4 view = glm::lookAt(
			  glm::vec3(charX, charY, charZ),  //Cam Position
			  glm::vec3(charDirX, charDirY, charZ),  //Look at point
			  glm::vec3(0.0f, 0.0f, 1.0f)); //Up
		  GLint uniView = glGetUniformLocation(shaderProgram, "view");
		  glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		  //give a 90 deg horizontal fov
		  glm::mat4 proj = glm::perspective(1.29f, aspect, 0.3f, 500.0f); //FOV, aspect, near, far
		  GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
		  glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

		  GLint inColorID = glGetUniformLocation(shaderProgram, "inColor");
		  glUniform3f(inColorID, 0.7f, 0.7f, 0.7f);

		  glActiveTexture(GL_TEXTURE0);
		  glBindTexture(GL_TEXTURE_2D, tex0);
		  glUniform1i(glGetUniformLocation(shaderProgram, "tex0"), 0);

		  glActiveTexture(GL_TEXTURE1);
		  glBindTexture(GL_TEXTURE_2D, tex1);
		  glUniform1i(glGetUniformLocation(shaderProgram, "tex1"), 1);

		  glActiveTexture(GL_TEXTURE2);
		  glBindTexture(GL_TEXTURE_2D, tex2);
		  glUniform1i(glGetUniformLocation(shaderProgram, "tex2"), 2);
		  
		  glBindVertexArray(vao);
		  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
		  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		  glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
		  
		  
		  objList[i]->drawObject(shaderProgram, numTris);
		  
	  }

	  //check if we are near interactable object
	  if (!freeHand) {
		  //Are we standing near a door
		  for (int i = 0; i < doorList.size(); ++i) {
			  if (doorList[i]->isOpened) {
				  continue;
				}
			if (charX < doorList[i]->objX + 1 * sizeOfLab && charX > doorList[i]->objX - 1 * sizeOfLab) {
				if (charY < doorList[i]->objY + 1 * sizeOfLab && charY > doorList[i]->objY - 1 * sizeOfLab) {
					//Are we facing the door?
					if (doorList[i]->rayIntersection(charX, charDirX, charY, charDirY)) {
						RenderText(fontProgram, "Press E to interact", 100, 100, 1.0f, glm::vec3(1.0f, 1.0f, 0.5f), vaoFont, vboFont, tex3);
					}
				}
			}  
		  }
	  }//if(!freeHand)
	  else {//we are picking up a key
		  for (int i = 0; i < keyList.size(); ++i) {
			  //we are standing in the same square as the key
			  if (keyList[i]->isUsed) {
				  continue;
			  }
			  if (charX < keyList[i]->objX + 0.5 * sizeOfLab && charX > keyList[i]->objX - 0.5 * sizeOfLab) {
				  if (charY < keyList[i]->objY + 0.5 * sizeOfLab && charY > keyList[i]->objY - 0.5 * sizeOfLab) {
					  //Are we facing the key?
					  if (keyList[i]->rayIntersection(charX, charDirX, charY, charDirY)) {
						  RenderText(fontProgram, "Press E to interact", 100, 100, 1.0f, glm::vec3(1.0f, 1.0f, 0.5f), vaoFont, vboFont, tex3);
					  }
				  }
			  }
		  }
	  }
	  if (gameMode) {
		  if (charX < goal->objX + 0.5 * sizeOfLab && charX > goal->objX - 0.5 * sizeOfLab) {
			  if (charY < goal->objY + 0.5 * sizeOfLab && charY > goal->objY - 0.5 * sizeOfLab) {
				  if (goal->rayIntersection(charX, charDirX, charY, charDirY)) {
					  RenderText(fontProgram, "Press E to interact", 100, 100, 1.0f, glm::vec3(1.0f, 1.0f, 0.5f), vaoFont, vboFont, tex3);
				  }
			  }
		  }
	  }
	  
	  if (!gameMode) {
		  RenderText(fontProgram, "Victory! A winner is you", screen_width/3, screen_height/2, 1.0f, glm::vec3(1.0f, 1.0f, 0.5f), vaoFont, vboFont, tex3);
		  RenderText(fontProgram, "Press Esc to quit", screen_width / 2.7, screen_height / 2.2, 1.0f, glm::vec3(1.0f, 1.0f, 0.5f), vaoFont, vboFont, tex3);
		  RenderText(fontProgram, "Maps: SpiralOfTurmoil, FourLeafClover, MontyHall", screen_width / 5.2, screen_height / 2.5, 1.0f, glm::vec3(1.0f, 1.0f, 0.5f), vaoFont, vboFont, tex3);
	  }
      if (saveOutput) Win2PPM(screen_width,screen_height);
      

      SDL_GL_SwapWindow(window); //Double buffering
	}
	

  //Clean Up
	glDeleteProgram(shaderProgram);

  glDeleteBuffers(1, vbo);
  glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();

	//TTF_CloseFont(gFont);
	//TTF_Quit();
	for (int i = 0; i < objList.size(); ++i) {
		delete objList[i];
	}
	delete[] map;
	return 0;
}


//Write out PPM image from screen
void Win2PPM(int width, int height){
	char outdir[10] = "out/"; //Must be defined!
	int i,j;
	FILE* fptr;
    static int counter = 0;
    char fname[32];
    unsigned char *image;
    
    /* Allocate our buffer for the image */
    image = new unsigned char[3*width*height*sizeof(char)];
    if (image == NULL) {
      fprintf(stderr,"ERROR: Failed to allocate memory for image\n");
    }
    
    /* Open the file */
    sprintf(fname,"%simage_%04d.ppm",outdir,counter);
    if ((fptr = fopen(fname,"w")) == NULL) {
      fprintf(stderr,"ERROR: Failed to open file to write image\n");
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

//Taken from test.cpp by CSCI5607
// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {
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

GLuint InitShader(const char* vertexName, const char* fragmentName) {
	//Load the vertex Shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLchar *vText, *fText;
	vText = readShaderSource(vertexName);
	if (vText == NULL) {
		printf("Failed to read from vertex shader file %s\n", vertexName);
		exit(1);
	} 
	/*else {
	 printf("Vertex Shader:\n=====================\n");
	 printf("%s\n", vText);
	 printf("=====================\n\n");
	}*/
 
	const char* vv = vText;
	glShaderSource(vertexShader, 1, &vv, NULL);
	glCompileShader(vertexShader);

	//Let's double check the shader compiled 
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char buffer[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Compilation Error",
			"Failed to Compile: Check Consol Output.",
			NULL);
		printf("Vertex Shader Compile Failed. Info:\n\n%s\n", buffer);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	fText = readShaderSource(fragmentName);
	if (fText == NULL) {
		printf("Failed to read from fragment shader file %s\n", fragmentName);
		exit(1);
	}
	/*else{
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fText);
		printf("=====================\n\n");
	}*/
	const char* ff = fText;
	glShaderSource(fragmentShader, 1, &ff, NULL);
	glCompileShader(fragmentShader);

	//Double check the shader compiled 
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char buffer[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Compilation Error",
			"Failed to Compile: Check Consol Output.",
			NULL);
		printf("Fragment Shader Compile Failed. Info:\n\n%s\n", buffer);
	}

	//Join the vertex and fragment shaders together into one program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
	glLinkProgram(shaderProgram); //run the linker

	glUseProgram(shaderProgram); //Set the active shader (only one can be used at a time)

	return shaderProgram;
}