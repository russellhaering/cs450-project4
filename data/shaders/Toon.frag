// This shader is based on information from the following resources:
// http://www.opengl.org/sdk/docs/tutorials/ClockworkCoders/lighting.php
// http://www.lighthouse3d.com/opengl/glsl/index.php
varying vec3 N, v;

void main()
{
	vec4 color;

	// The eye normal
	vec3 E = normalize(-v);

	// How brightly lit the fragment is
	float intensity = dot((gl_LightSource[0].position.xyz - v), normalize(N));

	if (intensity > .999)
		color = vec4(0.0, 1.0, 1.0, 1.0);
	else if (intensity > .7)
		color = vec4(0.0, 0.6, 0.6, 1.0);
	else if (intensity > .05)
		color = vec4(0.0, 0.4, 0.4, 1.0);
	else
		color = vec4(0.0, 0.2, 0.2, 1.0);

	if (dot(N, E) < .02)
		color = vec4(0.0, 0.0, 0.0, 1.0);
	
	gl_FragColor = color;
}
