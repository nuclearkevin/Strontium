#version 330 core
/*
 *  Simple fragment sharder for example four
 */

in vec3 normal;

void main() {
	vec3 N;
	vec3 L = vec3(0.0, 0.0, 1.0);
	vec4 colour = vec4(1.0, 0.0, 0.0, 1.0);
	float diffuse;

	N = normalize(normal);
	diffuse = dot(N,L);
	if(diffuse < 0.0) {
		diffuse = 0.0;
	}

	gl_FragColor = min(0.3*colour + 0.7*diffuse*colour, vec4(1.0));
	gl_FragColor.a = colour.a;
}