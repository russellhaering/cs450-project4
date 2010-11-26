varying vec3 N;

void main()
{
	// Calculate the normal
	N = normalize(gl_NormalMatrix * gl_Normal);

	// Set the vertex position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
