 #version 450
 
 layout(location = 0) in vec3 aPos;
 layout(location = 1) in vec3 aColour;
 
 layout(binding = 0) uniform UboViewProjectionSetup {
	mat4 Projection;
	mat4 View;
 } uViewProjectionMatrix;

  layout(binding = 1) uniform UboModelSetup {
	mat4 Model;
 } uModelMatrix;
 
 layout(location = 0) out vec3 vColour;
 
 void main()
 {
	gl_Position = uViewProjectionMatrix.Projection * uViewProjectionMatrix.View * uModelMatrix.Model * vec4(aPos, 1.0);
	
	vColour = aColour;
 }
