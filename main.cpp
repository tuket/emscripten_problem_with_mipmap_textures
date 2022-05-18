#include <iostream>
#include <assert.h>
#include <GLFW/glfw3.h>
#include <span>

#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <emscripten.h>
#else
    #include <glad/glad.h>
#endif

typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;
typedef const char* const ConstStr;

GLFWwindow* window = nullptr;
u32 prog;
u32 vao;
u32 tex;

namespace shader_srcs
{
ConstStr header =
R"GLSL(#version 300 es
#define PI 3.1415926535897932
precision highp float;
)GLSL";
}

ConstStr vertSrc =
R"GLSL(
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_tc;

out vec2 v_tc;

void main()
{
    gl_Position = vec4(a_pos, 1);
    v_tc = a_tc;
}
)GLSL";

ConstStr fragSrc =
R"GLSL(
layout(location = 0) out vec4 o_color;
in vec2 v_tc;

uniform sampler2D u_tex;

void main()
{
    o_color = texture(u_tex, v_tc);
    //o_color = vec4(v_tc, 0, 1);
}
)GLSL";

char* checkCompileErrors(u32 shad, std::span<char> buffer)
{
    i32 ok;
    glGetShaderiv(shad, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLsizei outSize;
        glGetShaderInfoLog(shad, buffer.size(), &outSize, buffer.data());
        return buffer.data();
    }
    return nullptr;
}

char* checkLinkErrors(u32 prog, std::span<char> buffer)
{
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, buffer.size(), nullptr, buffer.data());
        return buffer.data();
    }
    return nullptr;
}

static void printCodeWithLines(std::span<const char*> srcs)
{
    int line = 1;
    for (const char* s : srcs)
    {
        //int line = 1;
        int start = 0;
        int end = 0;
        auto printLine = [&]() { printf("%4d| %.*s\n", line, end - start, s + start); };
        while (s[end]) {
            if (s[end] == '\n') {
                printLine();
                start = end = end + 1;
                line++;
            }
            else
                end++;
        }
        printLine();
    }
}

void printShaderCodeWithHeader(const char* src)
{
    const char* srcs[2] = { shader_srcs::header, src };
    printCodeWithLines(srcs);
}

static const char* shaderTypeToStr(GLenum type)
{
    const char* str = "";
    switch (type) {
    case GL_VERTEX_SHADER:
        str = "VERT"; break;
    case GL_FRAGMENT_SHADER:
        str = "FRAG"; break;
    default:
        assert(false);
    }
    return str;
}

char buffer[16 << 10];
u32 easyCreateShader(const char* name, const char* src, GLenum type)
{
    const char* typeName = shaderTypeToStr(type);

    const u32 shad = glCreateShader(type);
    ConstStr srcs[] = { shader_srcs::header, src };
    glShaderSource(shad, 2, srcs, nullptr);
    glCompileShader(shad);
    if (const char* errMsg = checkCompileErrors(shad, buffer)) {
        printf("Error in '%s'(%s):\n%s", name, typeName, errMsg);
        printShaderCodeWithHeader(src);
        assert(false);
    }
    return shad;
}

struct ShaderCreateInfo { const char* src; GLenum type; };

u32 easyCreateShaderProg(const char* name, std::span<const ShaderCreateInfo> infos, std::span<const u32> shaders)
{
    assert(infos.size() == shaders.size());
    const u32 numShaders = infos.size();

    u32 prog = glCreateProgram();
    for (u32 i = 0; i < numShaders; i++)
        glAttachShader(prog, shaders[i]);

    glLinkProgram(prog);
    const char* errMsg = checkLinkErrors(prog, buffer);
    if (errMsg) {
        printf("%s\n", errMsg);
        for (u32 i = 0; i < numShaders; i++) {
            printf("shader:%s:\n", shaderTypeToStr(infos[i].type));
            printShaderCodeWithHeader(infos[i].src);
        }
        assert(false);
    }

    for (u32 i = 0; i < numShaders; i++)
        glDetachShader(prog, shaders[i]);

    return prog;
}

u32 easyCreateShaderProg(const char* name, std::span<const ShaderCreateInfo> infos)
{
    constexpr u32 MAX_SHADERS = 8;
    const u32 numShaders = infos.size();
    assert(infos.size() < MAX_SHADERS);
    u32 shaders[MAX_SHADERS];
    for (u32 i = 0; i < numShaders; i++)
        shaders[i] = easyCreateShader(name, infos[i].src, infos[i].type);

    const u32 prog = easyCreateShaderProg(name, infos, { shaders, shaders + numShaders });

    for (u32 i = 0; i < numShaders; i++)
        glDeleteShader(shaders[i]);

    return prog;
}

void mainLoop() {
    glfwPollEvents();

    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glfwSwapBuffers(window);
}

int main()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(800, 600, "webgl", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

#ifndef __EMSCRIPTEN__
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
        printf("Error initilizing glad\n");
        exit(EXIT_FAILURE);
    }
#endif

    const ShaderCreateInfo infos[] = { {vertSrc, GL_VERTEX_SHADER}, {fragSrc, GL_FRAGMENT_SHADER} };
    prog = easyCreateShaderProg("prog", infos);
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "u_tex"), 0);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    const float verts[] = {
        -1, -1, 0,  0, 0,
        +1, -1, 0,  1, 0,
        +1, +1, 0,  1, 1,
        -1, +1, 0,  0, 1
    };
    u32 vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    struct Color { u8 r, g, b; };
    const Color pixels[] = {
        {255, 0, 0}, {0, 255, 0},
        {0, 0, 255}, {255, 255, 0}
    };
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    //glTexImage2D(GL_TEXTURE_2D, 1, GL_SRGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(&mainLoop, 0, 1);
#else
    while (!glfwWindowShouldClose(window)) {
        mainLoop();
    }
#endif

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
