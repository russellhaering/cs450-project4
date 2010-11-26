varying vec3 N, E;

void main()
{
	float intensity = dot(vec3(gl_LightSource[0].position), normalize(N));
	vec4 color;

	if (intensity > .999)
		color = vec4(0.0, 1.0, 1.0, 1.0);
	else if (intensity > .5)
		color = vec4(0.0, 0.6, 0.6, 1.0);
	else if (intensity > .1)
		color = vec4(0.0, 0.4, 0.4, 1.0);
	else
		color = vec4(0.0, 0.2, 0.2, 1.0);

	if (dot(N, E) < .08)
		color = vec4(0.0, 0.0, 0.0, 1.0);
	
	gl_FragColor = color;
}
