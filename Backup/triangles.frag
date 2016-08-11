#version 430 core

in vec4 fragmentColor;
out vec4 color;

void main()
{
	//color = vec4(1.0, 0.0, 0.0, 1.0);
	color = fragmentColor;
}