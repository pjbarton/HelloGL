#version 430 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;
out vec4 fragmentColor;

uniform mat4 utransform;


void main()
{
	fragmentColor = vec4(1.0, 0.0, 0.0, 1.0);

	//fragmentColor = vec4(vColor);
	gl_Position = utransform * vPosition;
}