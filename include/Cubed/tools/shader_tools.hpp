#pragma once

#include <glad/glad.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <SOIL2.h>

void printShaderLog(GLuint shader);
void printProgramInfo(int prog);
bool checkOpenGLError();
std::string readShaderSource(const char* filePath);
GLuint loadTexture(const char* texImagePath);