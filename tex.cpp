#version 430 core
// Input layout qualifier declaring a 16 x 16 (x 1) local
// workgroup size

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f) uniform image2D output_image;

void main(void)
{
	    imageStore(output_image,
		    ivec2(gl_GlobalInvocationID.xy),
		    vec4(vec2(gl_LocalInvocationID.xy) / vec2(gl_WorkGroupSize.xy), 0.0, 0.0));
}