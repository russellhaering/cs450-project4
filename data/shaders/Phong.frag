varying vec3 N, v;

void main()
{
	vec4 ambient, diffuse, specular;
	vec3 L, E, R;
	float shininess, rFactor;

	// Now calculate the light, eye and reflection vectors
	L = normalize(gl_LightSource[0].position.xyz - v);
	// This must be in the fragment shader
	E = normalize(-v);
	// Note that N is not actually normal at this point, but it doesn't matter
	R = normalize(-reflect(L, N));

	// We are given a pre-computed light product ambient (material * (light + model))
	ambient = gl_FrontLightProduct[0].ambient;

	// Diffuse depends on the dot of the Normal and Light directions,
	// and may not be below zero
	diffuse = gl_FrontLightProduct[0].diffuse * max(dot(N, L), 0.0);


	// Specular depends on how much of the reflection goes in the direction of the eye
	// and gets raised to the 'shininess' value to make it go from very dark to very
	// bright quickly
	shininess = gl_FrontMaterial.shininess * 0.5;
	rFactor = max(dot(R, E), 0.0);
	specular = gl_FrontLightProduct[0].specular * pow(rFactor, shininess);

	// Force specular to be between 0 and 1
	specular = clamp(specular, 0.0, 1.0);

    gl_FragColor = gl_FrontLightModelProduct.sceneColor + ambient + diffuse + specular;
}
