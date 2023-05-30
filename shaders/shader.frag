#version 450

layout (location = 0) in vec3 vColour;

//out layouts (note locations are not the same as ins!)
//it equals the attachments however! location 0 writes to attachment 0
layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(vColour, 1.0);
}