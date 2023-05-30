#version 450

//out layouts (note locations are not the same as ins!)
//it equals the attachments however! location 0 writes to attachment 0
layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(0.7, 0.99, 0.2, 1.0);
}