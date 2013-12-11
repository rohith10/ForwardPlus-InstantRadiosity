#version 420 core

out vec4 outColor;

void main ()
{
	vec3 outColor3 = vec3 (gl_FragCoord.z);
	outColor = vec4 (outColor3, 1.0);
}