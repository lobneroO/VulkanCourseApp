#version 450

//in layouts
layout(location = 0) in vec3 fragColour; 

//out layouts (note locations are not the same as ins!)
//it equals the attachments however! location 0 writes to attachment 0
layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
}