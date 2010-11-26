varying vec3 N, L, E, R;

void main()
{
	// Calculate the normal
	N = normalize(gl_NormalMatrix * gl_Normal);

	// And the vertex position
	vec3 v = vec3(gl_ModelViewMatrix * gl_Vertex);

	// Now calculate the light, eye and reflection vectors
	L = normalize(vec3(gl_LightSource[0].position));
	E = normalize(-v);
	R = normalize(-reflect(L, N));

	// Set the vertex position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
