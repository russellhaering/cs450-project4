varying vec3 N, E;

void main()
{
	// Calculate the normal
	N = normalize(gl_NormalMatrix * gl_Normal);

	// And the vertex position
	vec3 v = vec3(gl_ModelViewMatrix * gl_Vertex);

	// The eye normal
	E = normalize(-v);

	// Set the vertex position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
