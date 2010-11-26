varying vec3 N, L, E, R;

void main()
{
	vec4 ambient, diffuse, specular;
	float shininess;

	// We are given a pre-computed light product ambient (material * (light + model))
	ambient = gl_FrontLightProduct[0].ambient;

	// Diffuse depends on the dot of the Normal and Light directions,
	// and may not be below zero
	diffuse = gl_FrontLightProduct[0].diffuse * max(dot(N, normalize(L)), 0.0);

	// Tone down the shininess some
	shininess = gl_FrontMaterial.shininess;

	// Specular depends on how much of the reflection goes in the direction of the eye
	// and gets raised to the 'shininess' value to make it go from very dark to very
	// bright quickly
	specular = gl_FrontLightProduct[0].specular * pow(dot(R, E), shininess);

	// Force specular to be between 0 and 1
	specular = clamp(specular, 0.0, 1.0);

    gl_FragColor = gl_FrontLightModelProduct.sceneColor + ambient + diffuse + specular;
}
