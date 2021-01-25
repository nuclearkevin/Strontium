// Include guard.
#pragma once

class Shader
{
public:
    Shader(const char* vertPath, const char* fragPath);

    void addUniform();
protected:
    GLuint progID;
    GLuint vertID;
    GLuint fragID;
};

// Forward declaration of the shader parser/compiler function.
int buildShader(int type, const char* filename);

// Forward declaration of the program linker function.
int buildProgram(int first, ...);

// Forward declaration of the shader debug function.
void dumpProgram(int program, char* description);
