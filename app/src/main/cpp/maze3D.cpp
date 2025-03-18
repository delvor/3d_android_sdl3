#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GLES3/gl3.h>

#define _USE_MATH_DEFINES

#include <cmath>
#include <vector>
#include <android/log.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "maze3D", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "maze3D", __VA_ARGS__)

int SCREEN_WIDTH;
int SCREEN_HEIGHT;
const bool isCollisionsEnabled = true;
const bool isPositionLogEnabled = false;
const bool isCollisionLogEnabled = false;
const bool isWireRendered = true;

extern "C" void* iconv_open(const char* tocode, const char* fromcode) {
    // Dummy implementation
    return nullptr;
}

std::vector<glm::vec3> convertQuadsToTriangles(const std::vector<glm::vec3> &quadVertices) {
    std::vector<glm::vec3> triangleVertices;

    for (size_t i = 0; i < quadVertices.size(); i += 4) {
        if (i + 3 >= quadVertices.size()) break; // Ensure we have a full quad
        glm::vec3 v0 = quadVertices[i];     // First vertex
        glm::vec3 v1 = quadVertices[i + 1]; // Second vertex
        glm::vec3 v2 = quadVertices[i + 2]; // Third vertex
        glm::vec3 v3 = quadVertices[i + 3]; // Fourth vertex

        // Split quad into two triangles (v0-v1-v2 and v0-v2-v3)
        triangleVertices.push_back(v0);
        triangleVertices.push_back(v1);
        triangleVertices.push_back(v2);

        triangleVertices.push_back(v0);
        triangleVertices.push_back(v2);
        triangleVertices.push_back(v3);
    }

    return triangleVertices;
}

class Shader {
public:
    GLuint program;

    Shader(const char *vertexSource, const char *fragmentSource) {
        LOGI("Creating vertex shader...");
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        checkShaderError(vertexShader, "Vertex");

        LOGI("Creating fragment shader...");
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        checkShaderError(fragmentShader, "Fragment");

        LOGI("Linking shader program...");
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            LOGE("Shader linking failed: %s", infoLog);
        } else {
            LOGI("Shader program linked and validated: %u", program);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        LOGI("Shader program created: %u", program);
    }

    void use() { glUseProgram(program); }

    ~Shader() {
        LOGI("Deleting shader program: %u", program);
        if (program)
            glDeleteProgram(program);
    }

private:
    void checkShaderError(GLuint shader, const std::string &type) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            LOGE("%s compilation failed: %s", type.c_str(), infoLog);
        } else {
            LOGI("%s shader compiled", type.c_str());
        }
    }
};

class CubeShader {
public:
    GLuint program;

    CubeShader(const char *vertexSource, const char *fragmentSource) {
        LOGI("Creating cube vertex shader...");
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        checkShaderError(vertexShader, "Cube Vertex");

        LOGI("Creating cube fragment shader...");
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        checkShaderError(fragmentShader, "Cube Fragment");

        LOGI("Linking cube shader program...");
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            LOGE("Cube Shader linking failed: %s", infoLog);
        } else {
            LOGI("Cube shader program linked and validated: %u", program);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        LOGI("Cube shader program created: %u", program);
    }

    void use() { glUseProgram(program); }

    ~CubeShader() {
        LOGI("Deleting cube shader program: %u", program);
        if (program)
            glDeleteProgram(program);
    }

private:
    void checkShaderError(GLuint shader, const std::string &type) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            LOGE("%s compilation failed: %s", type.c_str(), infoLog);
        } else {
            LOGI("%s shader compiled", type.c_str());
        }
    }
};

class Player {
public:
    float x, y, z;
    float angle; // Degrees, 0 = +Z, 90 = +X, 180 = -Z, 270 = -X
    float moveSpeed;
    float turnSpeed;
    float radius;

    Player(float startX, float startY, float startZ)
            : x(startX), y(startY), z(startZ), angle(90.0f), moveSpeed(0.05f), turnSpeed(2.5f),
              radius(0.10f) {}

    bool collides(float newX, float newZ, const std::vector<std::vector<int>> &map) {
        int mapWidth = static_cast<int>(map[0].size());
        int mapHeight = static_cast<int>(map.size());

        if (newX - radius < 0 || newX + radius > mapWidth || newZ - radius < 0 ||
            newZ + radius > mapHeight) {
            if (isCollisionLogEnabled)
                //LOGI("Boundary collision at (%f, %f)", newX, newZ);
                return true;
        }

        int minX = std::max(0, static_cast<int>(newX - radius));
        int maxX = std::min(mapWidth - 1, static_cast<int>(newX + radius));
        int minZ = std::max(0, static_cast<int>(newZ - radius));
        int maxZ = std::min(mapHeight - 1, static_cast<int>(newZ + radius));

        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (map[z][x] == 1) {
                    float closestX = std::max(static_cast<float>(x),
                                              std::min(newX, static_cast<float>(x + 1)));
                    float closestZ = std::max(static_cast<float>(z),
                                              std::min(newZ, static_cast<float>(z + 1)));
                    float dx = newX - closestX;
                    float dz = newZ - closestZ;
                    float distance = std::sqrt(dx * dx + dz * dz);
                    if (distance < radius) {
                        if (isCollisionLogEnabled)
                            //LOGI("Wall collision at (%d, %d), distance: %f", x, z, distance);
                            return true;
                    }
                }
            }
        }
        return false;
    }

    void move(bool forward, bool backward, bool turnLeft, bool turnRight,
              const std::vector<std::vector<int>> &map) {
        float rad = angle * M_PI / 180.0f;
        float dirX = -sin(rad);
        float dirZ = cos(rad);

        float newX = x;
        float newZ = z;

        if (turnLeft)
            angle += turnSpeed;
        if (turnRight)
            angle -= turnSpeed;
        if (forward) {
            newX += dirX * moveSpeed;
            newZ += dirZ * moveSpeed;
        }
        if (backward) {
            newX -= dirX * moveSpeed;
            newZ -= dirZ * moveSpeed;
        }

        if (!collides(newX, newZ, map) || !isCollisionsEnabled) {
            x = newX;
            z = newZ;
        }

        if ((forward || backward || turnLeft || turnRight) && isPositionLogEnabled) {
            //LOGI("Position: (%f, %f, %f), Angle: %f°", x, y, z, angle);
        }
    }
};

class Level {
private:
    std::vector<std::vector<int>> map;
    int width, height;
    GLuint vao, vbo;           // For cubes
    GLuint floorVao, floorVbo; // For floor/ceiling
    CubeShader *cubeShader;
    Shader *flatShader;
    std::vector<float> cubeVertexData;  // Initial quad data
    std::vector<float> flatVertexData;  // Initial quad data
    std::vector<float> cubeTriangleData; // Converted triangle data
    std::vector<float> flatTriangleData; // Converted triangle data

    const char *cubeVertexShaderSource = R"(#version 300 es
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

    const char *cubeFragmentShaderSource = R"(#version 300 es
        precision mediump float;
        in vec3 fragColor;
        uniform bool useOverrideColor;
        out vec4 FragColor;
        void main() {
            vec3 color = useOverrideColor ? vec3(1.0, 0.0, 0.0) : fragColor; // Red for override
            FragColor = vec4(color, 1.0);
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

public:
    Level(const std::vector<std::vector<int>> &levelData)
            : map(levelData), height(static_cast<int>(map.size())),
              width(height > 0 ? static_cast<int>(map[0].size()) : 0) {
        LOGI("Initializing Level...");
        cubeShader = new CubeShader(cubeVertexShaderSource, cubeFragmentShaderSource);
        flatShader = new Shader(flatVertexShaderSource, flatFragmentShaderSource);
        setupGeometry();
    }

    ~Level() {
        LOGI("Destroying Level...");
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &floorVao);
        glDeleteBuffers(1, &floorVbo);
        delete cubeShader;
        delete flatShader;
    }

    void setupGeometry() {
        LOGI("Setting up geometry...");

        // Initial quad-based cube vertices
        float cubeVertices[] = {
                -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, // Front
                0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
                0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
                -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
                0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, // Back
                -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
                -0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
                0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f, 0.9f, 0.0f, 0.0f, // Left
                -0.5f, -0.5f, 0.5f, 0.9f, 0.0f, 0.0f,
                -0.5f, 0.5f, 0.5f, 0.9f, 0.0f, 0.0f,
                -0.5f, 0.5f, -0.5f, 0.9f, 0.0f, 0.0f,
                0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, // Right
                0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
                0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
                0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
                -0.5f, 0.5f, 0.5f, 0.7f, 0.0f, 0.0f, // Top
                0.5f, 0.5f, 0.5f, 0.7f, 0.0f, 0.0f,
                0.5f, 0.5f, -0.5f, 0.7f, 0.0f, 0.0f,
                -0.5f, 0.5f, -0.5f, 0.7f, 0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, // Bottom
                0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
                0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
                -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f
        };

        cubeVertexData.clear();
        std::vector<glm::vec3> cubeQuadVertices; // Store quad vertices as vec3 (position only for conversion)
        for (int z = 0; z < height; ++z) {
            for (int x = 0; x < width; ++x) {
                if (map[z][x] == 1) {
                    float r = static_cast<float>(x) / (width - 1);
                    float g = static_cast<float>(z) / (height - 1);
                    float b = 0.5f + 0.5f * (1.0f - r - g);
                    r = std::max(0.0f, std::min(1.0f, r));
                    g = std::max(0.0f, std::min(1.0f, g));
                    b = std::max(0.0f, std::min(1.0f, b));

                    for (int i = 0; i < 24 * 6; i += 6) // 24 vertices per cube (6 faces * 4 verts)
                    {
                        cubeVertexData.push_back(cubeVertices[i] + x + 0.5f);
                        cubeVertexData.push_back(cubeVertices[i + 1]);
                        cubeVertexData.push_back(cubeVertices[i + 2] + z + 0.5f);
                        cubeVertexData.push_back(r);
                        cubeVertexData.push_back(g);
                        cubeVertexData.push_back(b);
                        // Store position for quad-to-triangle conversion
                        cubeQuadVertices.push_back(glm::vec3(
                                cubeVertices[i] + x + 0.5f,
                                cubeVertices[i + 1],
                                cubeVertices[i + 2] + z + 0.5f
                        ));
                    }
                    if (x == 0 && z == 0) {
                        LOGI("First cube vertex: [%f, %f, %f]", cubeVertexData[0],
                             cubeVertexData[1], cubeVertexData[2]);
                    }
                }
            }
        }

        // Convert quads to triangles (position only for now)
        std::vector<glm::vec3> cubeTrianglePositions = convertQuadsToTriangles(cubeQuadVertices);

        // Create cubeTriangleData with positions and colors
        cubeTriangleData.clear();
        size_t quadIdx = 0;
        for (size_t i = 0; i < cubeTrianglePositions.size(); ++i) {
            cubeTriangleData.push_back(cubeTrianglePositions[i].x);
            cubeTriangleData.push_back(cubeTrianglePositions[i].y);
            cubeTriangleData.push_back(cubeTrianglePositions[i].z);
            // Reuse the last color from cubeVertexData for each triangle vertex
            cubeTriangleData.push_back(cubeVertexData[quadIdx + 3]); // r
            cubeTriangleData.push_back(cubeVertexData[quadIdx + 4]); // g
            cubeTriangleData.push_back(cubeVertexData[quadIdx + 5]); // b
            if ((i + 1) % 3 == 0) quadIdx += 6; // Move to next quad's color after each triangle
            if (quadIdx >= cubeVertexData.size()) quadIdx = cubeVertexData.size() - 6;
        }

        // Floor and ceiling (already quads, convert to triangles)
        float floorColor[3] = {0.5f, 0.5f, 0.5f};
        std::vector<glm::vec3> flatQuadVertices = {
                glm::vec3(0.0f, -0.5f, 0.0f),
                glm::vec3(width, -0.5f, 0.0f),
                glm::vec3(width, -0.5f, height),
                glm::vec3(0.0f, -0.5f, height),
                glm::vec3(0.0f, 0.5f, 0.0f),
                glm::vec3(width, 0.5f, 0.0f),
                glm::vec3(width, 0.5f, height),
                glm::vec3(0.0f, 0.5f, height)
        };
        std::vector<glm::vec3> flatTrianglePositions = convertQuadsToTriangles(flatQuadVertices);

        // Create flatTriangleData with positions and colors
        flatTriangleData.clear();
        for (size_t i = 0; i < flatTrianglePositions.size(); ++i) {
            flatTriangleData.push_back(flatTrianglePositions[i].x);
            flatTriangleData.push_back(flatTrianglePositions[i].y);
            flatTriangleData.push_back(flatTrianglePositions[i].z);
            if (i < 6) // Floor (first 4 vertices)
            {
                flatTriangleData.push_back(floorColor[0]);
                flatTriangleData.push_back(floorColor[1]);
                flatTriangleData.push_back(floorColor[2]);
            } else // Ceiling (last 4 vertices)
            {
                float ceilingColor[3] = {0.3f, 0.3f, 0.7f};
                flatTriangleData.push_back(ceilingColor[0]);
                flatTriangleData.push_back(ceilingColor[1]);
                flatTriangleData.push_back(ceilingColor[2]);
            }
        }

        // Cube VAO/VBO (using triangle data)
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, cubeTriangleData.size() * sizeof(float),
                     cubeTriangleData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        //LOGI("Cube triangles: %zu", cubeTriangleData.size() / 6);

        // Floor/Ceiling VAO/VBO (using triangle data)
        glGenVertexArrays(1, &floorVao);
        glGenBuffers(1, &floorVbo);
        glBindVertexArray(floorVao);
        glBindBuffer(GL_ARRAY_BUFFER, floorVbo);
        glBufferData(GL_ARRAY_BUFFER, flatTriangleData.size() * sizeof(float),
                     flatTriangleData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        //LOGI("Flat triangles: %zu", flatTriangleData.size() / 6);
    }

    void render(const Player &player) {
        // Use GLM for projection matrix
        glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),  // Field of view (45 degrees)
                static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT,  // Aspect ratio
                0.1f,  // Near plane
                100.0f // Far plane
        );

        // Use GLM for view matrix
        glm::vec3 eye(player.x, player.y + 0.05f, player.z);  // Camera position
        glm::vec3 center = eye + glm::vec3(sin(player.angle * M_PI / 180.0f), 0.0f,
                                           -cos(player.angle * M_PI / 180.0f));  // Look direction
        glm::vec3 up(0.0f, 1.0f, 0.0f);  // Up vector
        glm::mat4 view = glm::lookAt(eye, center, up);

        // Model matrix (identity for now)
        glm::mat4 model = glm::mat4(1.0f);

        // Log uniform locations
        cubeShader->use();
        GLint projLoc = glGetUniformLocation(cubeShader->program, "projection");
        GLint viewLoc = glGetUniformLocation(cubeShader->program, "view");
        GLint modelLoc = glGetUniformLocation(cubeShader->program, "model");
        //LOGI("Uniforms: proj=%d, view=%d, model=%d", projLoc, viewLoc, modelLoc);

        // Render cubes (filled triangles)
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(vao);
        glUniform1i(glGetUniformLocation(cubeShader->program, "useOverrideColor"), GL_FALSE);
        //LOGI("Drawing cube triangles: %zu vertices", cubeTriangleData.size() / 6);
        glDrawArrays(GL_TRIANGLES, 0, cubeTriangleData.size() / 6);

        if (isWireRendered) {
            // Render wireframe using GL_LINES (simplified approach)
            glUniform1i(glGetUniformLocation(cubeShader->program, "useOverrideColor"),
                        GL_TRUE); // Red
            // Note: This is a basic approximation; for accurate wireframe, you'd need edge data
            glDrawArrays(GL_LINES, 0, cubeTriangleData.size() / 6 * 2); // Approximate with lines
        }

        glBindVertexArray(0);

        // Render floor/ceiling
        flatShader->use();
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "view"), 1, GL_FALSE,
                           glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "model"), 1, GL_FALSE,
                           glm::value_ptr(model));
        glBindVertexArray(floorVao);
        glDrawArrays(GL_TRIANGLES, 0, flatTriangleData.size() / 6);
        glBindVertexArray(0);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            LOGE("OpenGL Error during render: %d", error);
        }
    }

    const std::vector<std::vector<int>> &getMap() const { return map; }
};

std::vector<std::vector<int>> generateCustomMap(int width, int height) {
    if (width <= 10 || height <= 10) {
        LOGE("Invalid map dimensions: width=%d, height=%d. Using 10x10 default.", width, height);
        width = 10;
        height = 10;
    }

    std::vector<std::vector<int>> map(height, std::vector<int>(width, 0));
    srand(static_cast<unsigned>(time(nullptr)));

    for (int x = 0; x < width; ++x) {
        map[0][x] = 1;
        map[height - 1][x] = 1;
    }
    for (int z = 0; z < height; ++z) {
        map[z][0] = 1;
        map[z][width - 1] = 1;
    }

    int spawnSize = 3;
    for (int z = 1; z < height - 1; z++) {
        for (int x = 1; x < width - 1; x++) {
            if (z > spawnSize || x > spawnSize)
                map[z][x] = (rand() % 100 < 20) ? 1 : 0;
            else
                map[z][x] = 0;
        }
    }
    return map;
}

int main(int argc, char *argv[]) {
    LOGI("Starting 3D Engine...");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOGE("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }
    LOGI("SDL initialized.");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0); // OpenGL ES 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    // Create window with initial size (will be overridden by Android)
    SDL_Window *window = SDL_CreateWindow("3D Engine OpenGL ES 3.0", 0, 0,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                          SDL_WINDOW_FULLSCREEN);
    if (!window) {
        LOGE("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Get the actual window size (set by Android)

    SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    if (SCREEN_WIDTH <= 0 || SCREEN_HEIGHT <= 0) {
        LOGE("Invalid window size: %dx%d, falling back to 800x600", SCREEN_WIDTH, SCREEN_HEIGHT);
        SCREEN_WIDTH = 600;
        SCREEN_HEIGHT = 800;
    }
    LOGI("Initial window size: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        LOGE("GL context creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    LOGI("OpenGL ES Version: %s", glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // Set initial viewport
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Player player(1.5f, 0.0f, 1.5f);
    Level level(generateCustomMap(30, 30));
    LOGI("Player start: (%f, %f, %f), Angle: %f°", player.x, player.y, player.z, player.angle);

    bool running = true;
    bool paused = false;

    SDL_Event event;
    bool moveForward = false, moveBackward = false, turnLeft = false, turnRight = false;


    // Touch control variables
    bool isTurning = false; // Track if we're in a turning gesture
    float lastTouchX = 0.0f; // Last X position of the finger for turning


    while (running) {
        while (SDL_PollEvent(&event))
        {
            LOGI("SDL Event: %d", event.type); // Debug all events

            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    LOGI("SDL_EVENT_QUIT received, but ignoring on Android");
                    break;

                case SDL_EVENT_WILL_ENTER_BACKGROUND:
                    LOGI("SDL_EVENT_WILL_ENTER_BACKGROUND received, pausing app");
                    paused = true;
                    break;

                case SDL_EVENT_DID_ENTER_FOREGROUND:
                    LOGI("SDL_EVENT_DID_ENTER_FOREGROUND received, resuming app");
                    paused = false;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                    SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
                    LOGI("Window size changed to: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
                    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); // Update viewport on resize
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
                            LOGI("Escape key pressed, quitting app");
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
                {
                    float touchX = event.tfinger.x * SCREEN_WIDTH; // Normalized [0,1] to pixel coordinates
                    lastTouchX = touchX; // Store initial touch position for turning

                    if (touchX < SCREEN_WIDTH / 2) // Left half of screen for turning
                    {
                        isTurning = true;
                        LOGI("Finger down on left half, starting turn gesture at x=%f", touchX);
                    }
                    else // Right half for movement
                    {
                        moveBackward = true;
                        LOGI("Finger down on right half, moveBackward = true at x=%f", touchX);
                    }
                    break;
                }

                case SDL_EVENT_FINGER_UP:
                    if (isTurning)
                    {
                        isTurning = false;
                        turnLeft = false;
                        turnRight = false;
                        LOGI("Finger up, stopping turn gesture");
                    }
                    else
                    {
                        moveBackward = false;
                        LOGI("Finger up, moveBackward = false");
                    }
                    break;

                case SDL_EVENT_FINGER_MOTION:
                    if (isTurning)
                    {
                        float touchX = event.tfinger.x * SCREEN_WIDTH; // Current X position in pixels
                        float deltaX = touchX - lastTouchX; // Change in X position

                        // Turn based on drag direction
                        if (deltaX > 0) // Dragging right
                        {
                            turnRight = true;
                            turnLeft = false;
                            LOGI("Turning right, deltaX=%f", deltaX);
                        }
                        else if (deltaX < 0) // Dragging left
                        {
                            turnLeft = true;
                            turnRight = false;
                            LOGI("Turning left, deltaX=%f", deltaX);
                        }

                        lastTouchX = touchX; // Update last position for next motion event
                    }
                    break;

                default:
                    LOGI("Unhandled SDL Event: %d", event.type);
                    break;
            }
        }

        if (!paused) {
            player.move(moveForward, moveBackward, turnLeft, turnRight, level.getMap());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            level.render(player);
            SDL_GL_SwapWindow(window);
        } else {
            LOGI("App is paused, skipping render");
            SDL_Delay(100); // Reduce CPU usage while paused
        }

        SDL_Delay(16); // Cap frame rate
    }

    LOGI("Cleaning up...");
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    LOGI("Program ended.");
    return 0;
}