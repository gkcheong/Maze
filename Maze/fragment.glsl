//fragment shader
#version 150 core

in vec3 Color;
in vec3 normal;
in vec3 pos;
in vec2 texcoord;
in vec3 camPos;

out vec4 outColor;

const vec3 lightDir = normalize(vec3(0,1,1));
const vec3 lightDirColor = vec3(0.5,0.5,0.5);
const float ambient = .3;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform int texID;

void main() {
	vec3 color;
	if (texID == -1) {
		color = Color;
	}
	else if (texID == 0) {
		color = texture(tex0, texcoord).rgb;
	}
	else if (texID == 1) { 
		color = texture(tex1, texcoord).rgb;
	}
	else {
		color = 0.5*texture(tex2, texcoord).rgb + (1-0.5)*Color;
	}
     vec3 diffuseC = lightDirColor*color*max(dot(lightDir,normal),0.0);
     vec3 ambC = color*ambient;
     vec3 reflectDir = reflect(lightDir,normal);
     vec3 viewDir = normalize(camPos-pos);
     float spec = max(dot(reflectDir,viewDir),0.0);
     if (dot(lightDir,normal) <= 0.0)spec = 0;
     vec3 specC = lightDirColor*pow(spec,16);
     outColor = vec4(diffuseC+ambC+specC, 1.0f);

}