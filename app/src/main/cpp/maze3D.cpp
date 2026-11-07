#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GLES3/gl3.h>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#define _USE_MATH_DEFINES
#endif


#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

const bool COLLISION_ENABLED = true;
const bool POSITION_SDL_LOG_ENABLED = false;
const bool COLLISION_SDL_LOG_ENABLED = true;
const float CUBE_EDGE_WIDTH_PIXELS = 3.0f;

// Adds quad-edge coordinates so the fragment shader can draw pixel-width outlines.
std::vector<float> *convertQuadsToTriangles(const std::vector<float> &quadVertexData)
{
    std::vector<float> *triangleVertexData = new std::vector<float>();

    for (size_t i = 0; i < quadVertexData.size(); i += 24) // 24 floats per quad (4 verts * 6 floats)
    {
        if (i + 23 >= quadVertexData.size())
            break; // Ensure we have a full quad

        // Extract quad vertices (x, y, z, r, g, b)
        float v0[6] = {quadVertexData[i], quadVertexData[i + 1], quadVertexData[i + 2],
                       quadVertexData[i + 3], quadVertexData[i + 4], quadVertexData[i + 5]};
        float v1[6] = {quadVertexData[i + 6], quadVertexData[i + 7], quadVertexData[i + 8],
                       quadVertexData[i + 9], quadVertexData[i + 10], quadVertexData[i + 11]};
        float v2[6] = {quadVertexData[i + 12], quadVertexData[i + 13], quadVertexData[i + 14],
                       quadVertexData[i + 15], quadVertexData[i + 16], quadVertexData[i + 17]};
        float v3[6] = {quadVertexData[i + 18], quadVertexData[i + 19], quadVertexData[i + 20],
                       quadVertexData[i + 21], quadVertexData[i + 22], quadVertexData[i + 23]};

        auto appendVertex = [&](const float vertex[6], float edgeX, float edgeY)
        {
            for (int j = 0; j < 6; ++j)
                triangleVertexData->push_back(vertex[j]);
            triangleVertexData->push_back(edgeX);
            triangleVertexData->push_back(edgeY);
            triangleVertexData->push_back(1.0f);
        };

        appendVertex(v0, 0.0f, 1.0f);
        appendVertex(v1, 0.0f, 0.0f);
        appendVertex(v2, 1.0f, 0.0f);

        appendVertex(v0, 1.0f, 0.0f);
        appendVertex(v2, 0.0f, 1.0f);
        appendVertex(v3, 0.0f, 0.0f);
    }

    return triangleVertexData;
}

// For floor/ceiling: converts quads to triangles (positions only)
std::vector<float> *convertFlatQuadsToTriangles(const std::vector<glm::vec3> &quadVertices,
                                                const float floorColor[3],
                                                const float ceilingColor[3])
{
    std::vector<float> *triangleData = new std::vector<float>();

    for (size_t i = 0; i < quadVertices.size(); i += 4)
    {
        if (i + 3 >= quadVertices.size())
            break;
        glm::vec3 v0 = quadVertices[i];
        glm::vec3 v1 = quadVertices[i + 1];
        glm::vec3 v2 = quadVertices[i + 2];
        glm::vec3 v3 = quadVertices[i + 3];

        // Select color and winding based on quad (floor or ceiling)
        const float *color = (i == 0) ? floorColor : ceilingColor;
        if (i == 0)
        { // Floor: CCW from above
            // Triangle 1: v0-v2-v1
            triangleData->push_back(v0.x);
            triangleData->push_back(v0.y);
            triangleData->push_back(v0.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v2.x);
            triangleData->push_back(v2.y);
            triangleData->push_back(v2.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v1.x);
            triangleData->push_back(v1.y);
            triangleData->push_back(v1.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);

            // Triangle 2: v0-v3-v2
            triangleData->push_back(v0.x);
            triangleData->push_back(v0.y);
            triangleData->push_back(v0.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v3.x);
            triangleData->push_back(v3.y);
            triangleData->push_back(v3.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v2.x);
            triangleData->push_back(v2.y);
            triangleData->push_back(v2.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
        }
        else
        { // Ceiling: CCW from below (reverse winding)
            // Triangle 1: v0-v1-v2 (reverse of v0-v2-v1)
            triangleData->push_back(v0.x);
            triangleData->push_back(v0.y);
            triangleData->push_back(v0.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v1.x);
            triangleData->push_back(v1.y);
            triangleData->push_back(v1.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v2.x);
            triangleData->push_back(v2.y);
            triangleData->push_back(v2.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);

            // Triangle 2: v0-v2-v3 (reverse of v0-v3-v2)
            triangleData->push_back(v0.x);
            triangleData->push_back(v0.y);
            triangleData->push_back(v0.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v2.x);
            triangleData->push_back(v2.y);
            triangleData->push_back(v2.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
            triangleData->push_back(v3.x);
            triangleData->push_back(v3.y);
            triangleData->push_back(v3.z);
            triangleData->push_back(color[0]);
            triangleData->push_back(color[1]);
            triangleData->push_back(color[2]);
        }
    }

    return triangleData;
}

class Shader
{
public:
    GLuint program;

    Shader(const char *vertexSource, const char *fragmentSource)
    {
        SDL_Log("Creating vertex shader...");
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        checkShaderError(vertexShader, "Vertex");

        SDL_Log("Creating fragment shader...");
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        checkShaderError(fragmentShader, "Fragment");

        SDL_Log("Linking shader program...");
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            SDL_Log("Shader linking failed: %s", infoLog);
        }
        else
        {
            SDL_Log("Shader program linked and validated: %u", program);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        SDL_Log("Shader program created: %u", program);
    }

    void use() { glUseProgram(program); }

    ~Shader()
    {
        SDL_Log("Deleting shader program: %u", program);
        if (program)
            glDeleteProgram(program);
    }

private:
    void checkShaderError(GLuint shader, const std::string &type)
    {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            SDL_Log("%s compilation failed: %s", type.c_str(), infoLog);
        }
        else
        {
            SDL_Log("%s shader compiled", type.c_str());
        }
    }
};

class CubeShader
{
public:
    GLuint program;

    CubeShader(const char *vertexSource, const char *fragmentSource)
    {
        program = createProgram(vertexSource, fragmentSource);
        if (!program)
        {
            SDL_Log("Cube edge shader unavailable; falling back to solid-color cubes.");
#ifdef __ANDROID__
            const char *fallbackVertexSource = R"(#version 300 es
                precision mediump float;
                layout(location=0) in vec3 aPos;
                layout(location=1) in vec3 aColor;
                out vec3 fragColor;
                uniform mat4 model, view, projection;
                void main() {
                    gl_Position = projection * view * model * vec4(aPos, 1.0);
                    fragColor = aColor;
                }
            )";
            const char *fallbackFragmentSource = R"(#version 300 es
                precision mediump float;
                in vec3 fragColor;
                out vec4 FragColor;
                void main() {
                    FragColor = vec4(fragColor, 1.0);
                }
            )";
#else
            const char *fallbackVertexSource = R"(#version 330 core
                layout(location=0) in vec3 aPos;
                layout(location=1) in vec3 aColor;
                out vec3 fragColor;
                uniform mat4 model, view, projection;
                void main() {
                    gl_Position = projection * view * model * vec4(aPos, 1.0);
                    fragColor = aColor;
                }
            )";
            const char *fallbackFragmentSource = R"(#version 330 core
                in vec3 fragColor;
                out vec4 FragColor;
                void main() {
                    FragColor = vec4(fragColor, 1.0);
                }
            )";
#endif
            program = createProgram(fallbackVertexSource, fallbackFragmentSource);
        }
        SDL_Log("Cube shader program created: %u", program);
    }

    void use() { glUseProgram(program); }

    ~CubeShader()
    {
        SDL_Log("Deleting cube shader program: %u", program);
        if (program)
            glDeleteProgram(program);
    }

private:
    GLuint compileShader(GLenum shaderType, const char *source, const char *name)
    {
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            SDL_Log("%s compilation failed: %s", name, infoLog);
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint createProgram(const char *vertexSource, const char *fragmentSource)
    {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, "Cube vertex shader");
        if (!vertexShader)
            return 0;

        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "Cube fragment shader");
        if (!fragmentShader)
        {
            glDeleteShader(vertexShader);
            return 0;
        }

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            SDL_Log("Cube shader linking failed: %s", infoLog);
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return shaderProgram;
    }
};

class Player
{
public:
    float x, y, z;
    float angle; // Degrees, 0 = +Z, 90 = +X, 180 = -Z, 270 = -X
    float moveSpeed;
    float turnSpeed;
    float radius;

    Player(float startX, float startY, float startZ)
            : x(startX), y(startY), z(startZ), angle(90.0f), moveSpeed(0.12f), turnSpeed(2.5f),
              radius(0.10f) {}

    bool collides(float newX, float newZ, const std::vector<std::vector<int>> &map)
    {
        int mapWidth = static_cast<int>(map[0].size());
        int mapHeight = static_cast<int>(map.size());

        if (newX - radius < 0 || newX + radius > mapWidth || newZ - radius < 0 ||
            newZ + radius > mapHeight)
        {
            if (COLLISION_SDL_LOG_ENABLED)
                SDL_Log("Collides with wall");

            return true;
        }

        int minX = std::max(0, static_cast<int>(newX - radius));
        int maxX = std::min(mapWidth - 1, static_cast<int>(newX + radius));
        int minZ = std::max(0, static_cast<int>(newZ - radius));
        int maxZ = std::min(mapHeight - 1, static_cast<int>(newZ + radius));

        for (int z = minZ; z <= maxZ; ++z)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                if (map[z][x] == 1)
                {
                    float closestX = std::max(static_cast<float>(x),
                                              std::min(newX, static_cast<float>(x + 1)));
                    float closestZ = std::max(static_cast<float>(z),
                                              std::min(newZ, static_cast<float>(z + 1)));
                    float dx = newX - closestX;
                    float dz = newZ - closestZ;
                    float distance = std::sqrt(dx * dx + dz * dz);
                    if (distance < radius)
                    {
                        if (COLLISION_SDL_LOG_ENABLED)
                            SDL_Log("Collides with cube. distance - %f, radius - %f", distance, radius);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void move(float movement, float turning, float turnSpeedMultiplier,
              const std::vector<std::vector<int>> &map)
    {
        angle += turning * turnSpeed * turnSpeedMultiplier;

        float rad = angle * M_PI / 180.0f;
        float dirX = sin(rad);
        float dirZ = -cos(rad);

        float newX = x + dirX * moveSpeed * movement;
        float newZ = z + dirZ * moveSpeed * movement;

        if (!COLLISION_ENABLED || !collides(newX, newZ, map))
        {
            x = newX;
            z = newZ;
        }

        if ((movement != 0.0f || turning != 0.0f) && POSITION_SDL_LOG_ENABLED)
        {
            SDL_Log("position.x - %f, position.y - %f", x, y);
        }
    }
};

class Level
{
private:
    std::vector<std::vector<int>> map;
    int height, width;
    GLuint cubeVao, cubeVbo;   // For filled cubes (triangles)
    GLuint floorVao, floorVbo; // For floor/ceiling
    CubeShader *cubeShader;
    Shader *flatShader;
    std::vector<float> *cubeVertexData;   // Initial quad data for filled cubes
    std::vector<float> *flatVertexData;   // Initial quad data for floor/ceiling
    std::vector<float> *cubeTriangleData; // Converted triangle data for filled cubes
    std::vector<float> *flatTriangleData; // Converted triangle data for floor/ceiling

#ifdef __ANDROID__
    const char *cubeVertexShaderSource = R"(#version 300 es
        precision highp float;
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aColor;
        layout(location=2) in vec3 aEdgeDistance;
        out vec3 fragColor;
        out vec3 edgeDistance;
        uniform mat4 model, view, projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            fragColor = aColor;
            edgeDistance = aEdgeDistance;
        }
    )";

    const char *cubeFragmentShaderSource = R"(#version 300 es
    precision highp float;
    in vec3 fragColor;
    in vec3 edgeDistance;
    uniform float edgeWidthPixels;
    out vec4 FragColor;
    void main() {
        vec3 pixelDistance = edgeDistance / max(fwidth(edgeDistance), vec3(0.0001));
        float distanceToEdge = min(pixelDistance.x, min(pixelDistance.y, pixelDistance.z));
        float edge = 1.0 - smoothstep(edgeWidthPixels - 0.5, edgeWidthPixels + 0.5,
                                     distanceToEdge);
        FragColor = vec4(mix(fragColor, vec3(0.0), edge), 1.0);
    }
)";
    const char *flatVertexShaderSource = R"(#version 300 es
    precision mediump float;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    out vec3 fragColor;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        fragColor = aColor;
    }
)";
    const char *flatFragmentShaderSource = R"(#version 300 es
    precision mediump float;
    in vec3 fragColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(fragColor, 1.0);
    }
)";
#else
    const char *cubeVertexShaderSource = R"(#version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aColor;
        layout(location=2) in vec3 aEdgeDistance;
        out vec3 fragColor;
        out vec3 edgeDistance;
        uniform mat4 model, view, projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            fragColor = aColor;
            edgeDistance = aEdgeDistance;
        }
    )";

    const char *cubeFragmentShaderSource = R"(#version 330 core
    in vec3 fragColor;
    in vec3 edgeDistance;
    uniform float edgeWidthPixels;
    out vec4 FragColor;
    void main() {
        vec3 pixelDistance = edgeDistance / max(fwidth(edgeDistance), vec3(0.0001));
        float distanceToEdge = min(pixelDistance.x, min(pixelDistance.y, pixelDistance.z));
        float edge = 1.0 - smoothstep(edgeWidthPixels - 0.5, edgeWidthPixels + 0.5,
                                     distanceToEdge);
        FragColor = vec4(mix(fragColor, vec3(0.0), edge), 1.0);
    }
)";
    const char *flatVertexShaderSource = R"(#version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    out vec3 fragColor;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        fragColor = aColor;
    }
)";
    const char *flatFragmentShaderSource = R"(#version 330 core
    in vec3 fragColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(fragColor, 1.0);
    }
)";
#endif

public:
    Level(const std::vector<std::vector<int>> &levelData)
            : map(levelData), height(static_cast<int>(map.size())),
              width(height > 0 ? static_cast<int>(map[0].size()) : 0)
    {
        SDL_Log("Initializing Level...");
        cubeShader = new CubeShader(cubeVertexShaderSource, cubeFragmentShaderSource);
        flatShader = new Shader(flatVertexShaderSource, flatFragmentShaderSource);

        cubeVertexData = new std::vector<float>();
        cubeTriangleData = new std::vector<float>();
        flatTriangleData = new std::vector<float>();
        flatVertexData = new std::vector<float>();

        setupGeometry();
    }

    ~Level()
    {
        SDL_Log("Destroying Level...");
        glDeleteVertexArrays(1, &cubeVao);
        glDeleteBuffers(1, &cubeVbo);
        glDeleteVertexArrays(1, &floorVao);
        glDeleteBuffers(1, &floorVbo);
        delete cubeShader;
        delete flatShader;
        delete cubeVertexData;
        delete cubeTriangleData;
        delete flatVertexData;
        delete flatTriangleData;
    }
    void setupGeometry()
    {
        SDL_Log("Setting up geometry...");

        // Initial quad-based cube vertices
        float cubeVertices[] = {
                -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  // Front: 0
                0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   //       1
                0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,    //       2
                -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   //       3
                0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,  // Back:  4
                -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, //       5
                -0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f,  //       6
                0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f,   //       7
                -0.5f, -0.5f, -0.5f, 0.9f, 0.0f, 0.0f, // Left:  8 (5)
                -0.5f, -0.5f, 0.5f, 0.9f, 0.0f, 0.0f,  //       9 (0)
                -0.5f, 0.5f, 0.5f, 0.9f, 0.0f, 0.0f,   //       10 (3)
                -0.5f, 0.5f, -0.5f, 0.9f, 0.0f, 0.0f,  //       11 (6)
                0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   // Right: 12 (1)
                0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  //       13 (4)
                0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,   //       14 (7)
                0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,    //       15 (2)
                -0.5f, 0.5f, 0.5f, 0.7f, 0.0f, 0.0f,   // Top:   16 (3)
                0.5f, 0.5f, 0.5f, 0.7f, 0.0f, 0.0f,    //       17 (2)
                0.5f, 0.5f, -0.5f, 0.7f, 0.0f, 0.0f,   //       18 (7)
                -0.5f, 0.5f, -0.5f, 0.7f, 0.0f, 0.0f,  //       19 (6)
                -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, // Bottom:20 (5)
                0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,  //       21 (4)
                0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f,   //       22 (1)
                -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f   //       23 (0)
        };
        SDL_Log("Clearing cube vertex data...");
        cubeVertexData->clear();
        int cubeCount = 0;
        SDL_Log("Building cube geometry...");
        for (int z = 0; z < height; ++z)
        {
            for (int x = 0; x < width; ++x)
            {
                if (map[z][x] == 1)
                {
                    cubeCount++;
                    float r = static_cast<float>(x) / (width - 1);
                    float g = static_cast<float>(z) / (height - 1);
                    float b = 0.5f + 0.5f * (1.0f - r - g);
                    r = std::max(0.0f, std::min(1.0f, r));
                    g = std::max(0.0f, std::min(1.0f, g));
                    b = std::max(0.0f, std::min(1.0f, b));

                    for (int i = 0; i < 24 * 6; i += 6) // 24 vertices per cube
                    {
                        float shade = cubeVertices[i + 3];
                        float cubeX = x + 0.5f;
                        float cubeY = 0.0f;
                        float cubeZ = z + 0.5f;
                        float vertexX = cubeVertices[i] + cubeX;
                        float vertexY = cubeVertices[i + 1] + cubeY;
                        float vertexZ = cubeVertices[i + 2] + cubeZ;
                        cubeVertexData->push_back(vertexX);
                        cubeVertexData->push_back(vertexY);
                        cubeVertexData->push_back(vertexZ);
                        cubeVertexData->push_back(r * shade);
                        cubeVertexData->push_back(g * shade);
                        cubeVertexData->push_back(b * shade);

                    }
                    if (x == 0 && z == 0)
                    {
                        SDL_Log("First cube vertex: [%f, %f, %f]", (*cubeVertexData)[0],
                                (*cubeVertexData)[1], (*cubeVertexData)[2]);
                    }
                }
            }
        }
        SDL_Log("Cube count: %d, cube vertex data size: %zu",
                cubeCount, cubeVertexData->size());

        SDL_Log("Converting quads to triangles...");
        delete cubeTriangleData; // Free old data
        cubeTriangleData = convertQuadsToTriangles(*cubeVertexData);
        SDL_Log("Cube triangles: %zu vertices, Cube quads: %zu vertices",
                cubeTriangleData->size() / 9, cubeVertexData->size() / 6);

        SDL_Log("Setting up floor and ceiling...");
        float floorColor[3] = {0.5f, 0.5f, 0.5f};
        float ceilingColor[3] = {0.3f, 0.3f, 0.7f};
        std::vector<glm::vec3> flatQuadVertices = {
                glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(width, -0.5f, 0.0f),
                glm::vec3(width, -0.5f, height), glm::vec3(0.0f, -0.5f, height),
                glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(width, 0.5f, 0.0f),
                glm::vec3(width, 0.5f, height), glm::vec3(0.0f, 0.5f, height)};
        flatTriangleData = convertFlatQuadsToTriangles(flatQuadVertices, floorColor, ceilingColor);

        SDL_Log("Setting up VAOs...");
        // Cube VAO/VBO for filled triangles
        glGenVertexArrays(1, &cubeVao);
        glGenBuffers(1, &cubeVbo);
        glBindVertexArray(cubeVao);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
        glBufferData(GL_ARRAY_BUFFER, cubeTriangleData->size() * sizeof(float),
                     cubeTriangleData->data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        SDL_Log("Filled Cube VAO: %u, VBO: %u", cubeVao, cubeVbo);

        // Floor/Ceiling VAO/VBO
        glGenVertexArrays(1, &floorVao);
        glGenBuffers(1, &floorVbo);
        glBindVertexArray(floorVao);
        glBindBuffer(GL_ARRAY_BUFFER, floorVbo);
        glBufferData(GL_ARRAY_BUFFER, flatTriangleData->size() * sizeof(float),
                     flatTriangleData->data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        SDL_Log("Flat triangles: %zu", flatTriangleData->size() / 6);

        SDL_Log("Geometry setup complete.");
    }

    void render(const Player &player)
    {
        glm::mat4 projection = glm::perspective(
                glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT, 0.1f, 100.0f);
        glm::vec3 eye(player.x, player.y + 0.05f, player.z);
        glm::vec3 center = eye + glm::vec3(sin(player.angle * M_PI / 180.0f), 0.0f,
                                           -cos(player.angle * M_PI / 180.0f));
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(eye, center, up);
        glm::mat4 model = glm::mat4(1.0f);

        flatShader->use();
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "view"), 1, GL_FALSE,
                           glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "model"), 1, GL_FALSE,
                           glm::value_ptr(model));
        glBindVertexArray(floorVao);

        // SDL_Log("Drawing flat triangles: %zu vertices", flatTriangleData->size() / 6);
        glDrawArrays(GL_TRIANGLES, 0, flatTriangleData->size() / 6);
        glBindVertexArray(0);

        // Render filled cubes
        cubeShader->use();
        GLint projLoc = glGetUniformLocation(cubeShader->program, "projection");
        GLint viewLoc = glGetUniformLocation(cubeShader->program, "view");
        GLint modelLoc = glGetUniformLocation(cubeShader->program, "model");
        // SDL_Log("Uniforms: proj=%d, view=%d, model=%d", projLoc, viewLoc, modelLoc);

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(cubeVao);
        glUniform1f(glGetUniformLocation(cubeShader->program, "edgeWidthPixels"),
                    CUBE_EDGE_WIDTH_PIXELS);
        glDrawArrays(GL_TRIANGLES, 0, cubeTriangleData->size() / 9);
        glBindVertexArray(0);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            SDL_Log("OpenGL Error during render: %d", error);
        }
    }

    const std::vector<std::vector<int>> &getMap() const { return map; }
};

std::vector<std::vector<int>> generateCustomMap(int width, int height)
{
    if (width <= 10 || height <= 10)
    {
        SDL_Log("Invalid map dimensions: width=%d, height=%d. Using 10x10 default.", width, height);
        width = 10;
        height = 10;
    }

    std::vector<std::vector<int>> map(height, std::vector<int>(width, 0));
    srand(static_cast<unsigned>(time(nullptr)));

    for (int x = 0; x < width; ++x)
    {
        map[0][x] = 1;
        map[height - 1][x] = 1;
    }
    for (int z = 0; z < height; ++z)
    {
        map[z][0] = 1;
        map[z][width - 1] = 1;
    }

    int spawnSize = 3;
    for (int z = 1; z < height - 1; z++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            if (z > spawnSize || x > spawnSize)
                map[z][x] = (rand() % 100 < 20) ? 1 : 0;
            else
                map[z][x] = 0;
        }
    }
    return map;
}

int main(int argc, char *argv[])
{
    SDL_Log("Starting 3D Engine...");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }
    SDL_Log("SDL initialized.");

#ifdef __ANDROID__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

    SDL_Window *window = SDL_CreateWindow("3D Engine OpenGL 3.0", 0, 0,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                          SDL_WINDOW_FULLSCREEN);
    if (!window)
    {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    if (SCREEN_WIDTH <= 0 || SCREEN_HEIGHT <= 0)
    {
        SDL_Log("Invalid window size: %dx%d, falling back to 800x600", SCREEN_WIDTH, SCREEN_HEIGHT);
        SCREEN_WIDTH = 600;
        SCREEN_HEIGHT = 800;
    }
    SDL_Log("Initial window size: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        SDL_Log("GL context creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

#ifndef __ANDROID__
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        SDL_Log("GLEW init failed");
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_Log("GLEW initialized.");
#endif

    SDL_Log("OpenGL ES Version: %s", glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE); // Enable backface culling
    glCullFace(GL_BACK);    // Cull back faces
    glFrontFace(GL_CCW);    // Define counterclockwise as front

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // Set initial viewport
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Player player(1.5f, 0.0f, 1.5f);
    Level level(generateCustomMap(30, 30));
    SDL_Log("Player start: (%f, %f, %f), Angle: %f°", player.x, player.y, player.z, player.angle);

    bool running = true;
    bool paused = false;

    SDL_Event event;
    bool moveForward = false, moveBackward = false, turnLeft = false, turnRight = false;

    bool touchActive = false;
    SDL_FingerID activeFingerId = 0;
    float touchMovement = 0.0f;
    float touchTurning = 0.0f;
    float touchTurnSpeedMultiplier = 1.0f;

    auto updateTouchControls = [&](float touchX, float touchY)
    {
        const float centerX = 0.5f;
        const float centerY = 0.75f;
        const float halfHeight = 0.25f;
        const float deadZone = 0.08f;

        float horizontal = std::max(-1.0f, std::min((touchX - centerX) / centerX, 1.0f));
        float vertical = std::max(-1.0f, std::min((centerY - touchY) / halfHeight, 1.0f));

        auto applyDeadZone = [deadZone](float axis)
        {
            float magnitude = std::abs(axis);
            if (magnitude <= deadZone)
                return 0.0f;
            return std::copysign((magnitude - deadZone) / (1.0f - deadZone), axis);
        };

        touchTurning = applyDeadZone(horizontal);
        touchMovement = applyDeadZone(vertical);
        float distanceFromCenter = std::sqrt(horizontal * horizontal + vertical * vertical) /
                                   std::sqrt(2.0f);
        touchTurnSpeedMultiplier = 1.0f + 0.25f * distanceFromCenter;
    };

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            // SDL_Log("SDL Event: %d", event.type);

            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    SDL_Log("SDL_EVENT_QUIT received, but ignoring on Android");
                    break;

                case SDL_EVENT_WILL_ENTER_BACKGROUND:
                    SDL_Log("SDL_EVENT_WILL_ENTER_BACKGROUND received, pausing app");
                    paused = true;
                    break;

                case SDL_EVENT_DID_ENTER_FOREGROUND:
                    SDL_Log("SDL_EVENT_DID_ENTER_FOREGROUND received, resuming app");
                    paused = false;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                    SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
                    SDL_Log("Window size changed to: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
                    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                    break;

                case SDL_EVENT_KEY_DOWN:
                    switch (event.key.scancode)
                    {
                        case SDL_SCANCODE_W:
                            moveForward = true;
                            break;
                        case SDL_SCANCODE_S:
                            moveBackward = true;
                            break;
                        case SDL_SCANCODE_A:
                            turnLeft = true;
                            break;
                        case SDL_SCANCODE_D:
                            turnRight = true;
                            break;
                        case SDL_SCANCODE_ESCAPE:
                            SDL_Log("Escape key pressed, quitting app");
                            running = false;
                            break;
                    }
                    break;

                case SDL_EVENT_KEY_UP:
                    switch (event.key.scancode)
                    {
                        case SDL_SCANCODE_W:
                            moveForward = false;
                            break;

                        case SDL_SCANCODE_S:

                            moveBackward = false;
                            break;

                        case SDL_SCANCODE_A:
                            turnLeft = false;

                            break;

                        case SDL_SCANCODE_D:

                            turnRight = false;

                            break;
                    }
                    break;

                case SDL_EVENT_FINGER_DOWN:
                    if (!touchActive && event.tfinger.y >= 0.5f)
                    {
                        touchActive = true;
                        activeFingerId = event.tfinger.fingerID;
                        updateTouchControls(event.tfinger.x, event.tfinger.y);
                    }
                    break;

                case SDL_EVENT_FINGER_UP:
                case SDL_EVENT_FINGER_CANCELED:
                    if (touchActive && event.tfinger.fingerID == activeFingerId)
                    {
                        touchActive = false;
                        touchMovement = 0.0f;
                        touchTurning = 0.0f;
                        touchTurnSpeedMultiplier = 1.0f;
                    }
                    break;

                case SDL_EVENT_FINGER_MOTION:
                    if (touchActive && event.tfinger.fingerID == activeFingerId)
                    {
                        float touchX = std::max(0.0f, std::min(event.tfinger.x, 1.0f));
                        float touchY = std::max(0.5f, std::min(event.tfinger.y, 1.0f));
                        updateTouchControls(touchX, touchY);
                    }
                    break;

                default:
                    // SDL_Log("Unhandled SDL Event: %d", event.type);
                    break;
            }
        }

        if (!paused)
        {
            float movement = (moveForward ? 1.0f : 0.0f) -
                             (moveBackward ? 1.0f : 0.0f) + touchMovement;
            float turning = (turnRight ? 1.0f : 0.0f) -
                            (turnLeft ? 1.0f : 0.0f) + touchTurning;
            player.move(std::max(-1.0f, std::min(movement, 1.0f)),
                        std::max(-1.0f, std::min(turning, 1.0f)),
                        touchActive ? touchTurnSpeedMultiplier : 1.0f,
                        level.getMap());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            level.render(player);
            SDL_GL_SwapWindow(window);
        }
        else
        {
            SDL_Log("App is paused, skipping render");
            SDL_Delay(100);
        }

        SDL_Delay(16);
    }

    SDL_Log("Cleaning up...");
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    SDL_Log("Program ended.");
    return 0;
}