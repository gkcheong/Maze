// Shader sources
// vertex source
#version 150 core

in vec3 position;
//in vec3 inColor;
in vec3 inNormal;
in vec2 inTexcoord;

out vec3 Color;
out vec3 normal;
out vec3 pos;
out vec3 camPos;
out vec2 texcoord;

uniform vec3 inColor; //one color for the entire object
uniform vec3 inCamPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
	Color = inColor;
	gl_Position = proj * view * model * vec4(position,1.0);
	pos = (model * vec4(position,1.0)).xyz;
	vec4 norm4 = transpose(inverse(model)) * vec4(inNormal,0.0);  //Just model, than noramlize normal
	normal = normalize(norm4.xyz);
	texcoord = inTexcoord;
	camPos = inCamPos;
}