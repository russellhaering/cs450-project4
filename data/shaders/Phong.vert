varying vec3 N, v;

void main()
{
	// Calculate the normal
	N = normalize(gl_NormalMatrix * gl_Normal);

	// And the vertex position
	v = vec3(gl_ModelViewMatrix * gl_Vertex);

	// Set the vertex position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
