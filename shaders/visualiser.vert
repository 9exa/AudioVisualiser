#version 450

//Just read in the desired positions and uvs
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 inuv;
layout(location = 0) out vec2 uv;


void main() {
	gl_Position = vec4(position, 1.0);
	uv = inuv;
}