// OpenGL includes.
#include <GL/glew.h>

// STL includes.
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

// Header include.
#include "Shaders.h"

char *readShaderFile(const char* filename) {
	FILE *fid;
	char *buffer;
	int len;
	int n;

	fid = fopen(filename,"r");
	if (fid == NULL) {
		printf("can't open shader file: %s\n", filename);
		return(0);
	}

	fseek(fid, 0, SEEK_END);
	len = ftell(fid);
	rewind(fid);

	buffer = new char[len+1];
	n = fread(buffer, sizeof(char), len, fid);
	buffer[n] = 0;

	return buffer;
}

int buildShader(int type, const char* filename) {
	int shader;
	char *source;
	int result;
	char *buffer;

	shader = glCreateShader(type);
	source = readShaderFile(filename);
	if (source == 0)
		return 0;

	glShaderSource(shader, 1, (const  GLchar **) &source, 0);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		printf("shader compile error: %s\n",filename);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &result);
		buffer = new char[result];
		glGetShaderInfoLog(shader, result, 0, buffer);
		printf("%s\n", buffer);
		delete buffer;
		return(0);
	}

	return(shader);
}

int buildProgram(int first, ...) {
	int result;
	char *buffer;
	int program;
	va_list argptr;
	int shader;
	int vs = 0;
	int fs = 0;
	int type;

	program = glCreateProgram();
	if (first != 0) {
		glAttachShader(program,first);
		glGetShaderiv(first, GL_SHADER_TYPE, &type);
		if (type == GL_VERTEX_SHADER)
			vs++;
		if (type == GL_FRAGMENT_SHADER)
			fs++;
	}

	va_start(argptr,first);
	while ((shader = va_arg(argptr,int)) != 0) {
		glAttachShader(program,shader);
		glGetShaderiv(shader, GL_SHADER_TYPE, &type);
		if (type == GL_VERTEX_SHADER)
			vs++;
		if (type == GL_FRAGMENT_SHADER)
			fs++;
	}

	if (vs == 0) {
		printf("no vertex shader\n");
	}
	if (fs == 0) {
		printf("no fragment shader\n");
	}
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		printf("program link error\n");
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &result);
		buffer = new char[result];
		glGetProgramInfoLog(program, result, 0, buffer);
		printf("%s\n",buffer);
		delete buffer;
		return(0);
	}

	return(program);
}

void dumpProgram(int program, char *description) {
	char name[256];
	GLsizei length;
	GLint size;
	GLenum type;
	int uniforms;
	int attributes;
	int shaders;
	int i;

	printf("Information for shader: %s\n",description);

	if (!glIsProgram(program)) {
		printf("not a valid shader program\n");
		return;
	}

	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaders);
	printf("Number of shaders: %d\n",shaders);

	glGetProgramiv(program,GL_ACTIVE_UNIFORMS,&uniforms);
	printf("uniforms: %d\n",uniforms);
	for (i=0; i<uniforms; i++) {
		glGetActiveUniform(program, i, 256, &length ,&size ,&type, name);
		printf("  name: %s\n",name);
	}
	glGetProgramiv(program,GL_ACTIVE_ATTRIBUTES,&attributes);
	printf("attributes: %d\n",attributes);
	for (i=0; i<attributes; i++) {
		glGetActiveAttrib(program, i, 256, &length, &size, &type, name);
		printf("  name: %s\n",name);
	}
}
