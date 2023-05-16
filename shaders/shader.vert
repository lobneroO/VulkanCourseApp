 #version 450
 
 layout(location = 0) out vec3 fragColour;
 
 //define triangle inside shader for now
 //to be able to draw without resource loading
 //(this is not the standard way to do it)
 vec3 positions[3] = vec3[](
	vec3(0.0, -0.4, 0.0),
	vec3(0.4, 0.4, 0.0),
	vec3(-0.4, 0.4, 0.0)
 );
 
 //triangle vertex colours
 vec3 colours[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
 );
 
 void main()
 {
	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
	fragColour = colours[gl_VertexIndex];
 }