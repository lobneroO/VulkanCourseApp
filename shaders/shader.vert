 #version 450
 
 layout(location = 0) in vec3 aPos;
 layout(location = 1) in vec3 aColour;
 
 layout(binding = 0) uniform MatrixSetup {
	mat4 Projection;
	mat4 View;
	mat4 Model;
 } uModelViewProjectionMatrix;
 
 layout(location = 0) out vec3 vColour;
 
 void main()
 {
	gl_Position = uModelViewProjectionMatrix.Projection * uModelViewProjectionMatrix.View * uModelViewProjectionMatrix.Model * vec4(aPos, 1.0);
	
	vColour = aColour;
 }