#version 430 core

uniform sampler2D texture_sampler;
//uniform int detID;
uniform float detMin;
uniform float detMax;

in vec2 pass_TextureCoord;
out vec4 out_Color;

void main(void) 
{

	out_Color = texture(texture_sampler, pass_TextureCoord);
	//out_Color = texture(texture_sampler, pass_TextureCoord) + vec4(0.2, 0.2, 0.2, 1.0); // brighten
	
	float lum = dot(vec3(0.0, 1.0, 0.0), out_Color.rgb);

	//if (lum < detID/256.0 || lum > (detID+1)/256.0)
	//if ( lum < detMin || lum > detMax )
		//discard;

	//out_Color = vec4(0.0, 1.0, 0.0, 1.0);
}