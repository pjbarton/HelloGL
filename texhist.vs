#version 430

layout(location = 2) in vec2 in_Position;
layout(location = 3) in vec2 in_TextureCoord;

out vec2 pass_TextureCoord;

void main(void) {
	gl_Position = vec4(in_Position, 0.0, 1.0);
	pass_TextureCoord = in_TextureCoord;
}