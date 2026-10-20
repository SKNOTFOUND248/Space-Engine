#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

class Shader {
public:
    unsigned int ID = 0;

    Shader() = default;

    Shader(const char* vertexSource, const char* fragmentSource) {
        initFromSource(vertexSource, fragmentSource);
    }

    explicit Shader(const char* computeSource) {
        initCompute(computeSource);
    }

    ~Shader() {
        if (ID != 0) {
            glDeleteProgram(ID);
            ID = 0;
        }
    }

    // Disable copy to avoid accidental double-deletion of OpenGL program
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Enable move semantics
    Shader(Shader&& other) noexcept : ID(other.ID) {
        other.ID = 0;
    }
    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            if (ID != 0) {
                glDeleteProgram(ID);
            }
            ID = other.ID;
            other.ID = 0;
        }
        return *this;
    }

    void initFromSource(const char* vertexSource, const char* fragmentSource) {
        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, "VERTEX");
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void initCompute(const char* computeSource) {
        unsigned int computeShader = compileShader(GL_COMPUTE_SHADER, computeSource, "COMPUTE");
        ID = glCreateProgram();
        glAttachShader(ID, computeShader);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(computeShader);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), static_cast<int>(value));
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setUint(const std::string& name, unsigned int value) const {
        glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }

    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    static unsigned int compileShader(GLenum type, const char* source, const std::string& typeName) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        checkCompileErrors(shader, typeName);
        return shader;
    }

    static void checkCompileErrors(unsigned int shaderOrProgram, const std::string& type) {
        int success = 0;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shaderOrProgram, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shaderOrProgram, 1024, nullptr, infoLog);
                std::cerr << "[ERROR::SHADER_COMPILATION_ERROR] Type: " << type << "\n"
                          << infoLog << "\n--------------------------------------------------" << std::endl;
            }
        } else {
            glGetProgramiv(shaderOrProgram, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderOrProgram, 1024, nullptr, infoLog);
                std::cerr << "[ERROR::PROGRAM_LINKING_ERROR] Type: " << type << "\n"
                          << infoLog << "\n--------------------------------------------------" << std::endl;
            }
        }
    }
};
