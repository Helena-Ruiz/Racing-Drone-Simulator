#pragma once
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader 
{
public:
    Shader();
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void use() const { glUseProgram(programID); }

    // Setters d'uniforms
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
    }

    GLuint getID() const { return programID; }

private:
    GLuint programID = 0;

    std::string readFile(const std::string& filePath) const;
    void checkCompileErrors(GLuint shader, const std::string& type) const;
};