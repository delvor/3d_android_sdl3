#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GLES3/gl3.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const bool isCollisionsEnabled = true;
const bool isPositionLogEnabled = false;
const bool isCollisionLogEnabled = false;
const bool isWireRendered = true;

class Shader
{
public:
    GLuint program;

    Shader(const char *vertexSource, const char *fragmentSource)
    {
        std::cout << "Creating vertex shader...\n";
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        checkShaderError(vertexShader, "Vertex");

        std::cout << "Creating fragment shader...\n";
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        checkShaderError(fragmentShader, "Fragment");

        std::cout << "Linking shader program...\n";
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
            std::cerr << "Shader linking failed: " << infoLog << "\n";
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        std::cout << "Shader program created: " << program << "\n";
    }

    void use() { glUseProgram(program); }

    ~Shader()
    {
        std::cout << "Deleting shader program: " << program << "\n";
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
            std::cerr << type << " compilation failed: " << infoLog << "\n";
        }
    }
};

class CubeShader
{
public:
    GLuint program;

    CubeShader(const char *vertexSource, const char *fragmentSource)
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        checkShaderError(vertexShader, "Cube Vertex");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        checkShaderError(fragmentShader, "Cube Fragment");

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
            std::cerr << "Cube Shader linking failed: " << infoLog << "\n";
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        std::cout << "Cube shader program created: " << program << "\n";
    }

    void use() { glUseProgram(program); }

    ~CubeShader()
    {
        std::cout << "Deleting cube shader program: " << program << "\n";
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
            std::cerr << type << " compilation failed: " << infoLog << "\n";
        }
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
            : x(startX), y(startY), z(startZ), angle(90.0f), moveSpeed(0.05f), turnSpeed(2.5f), radius(0.10f) {}

    bool collides(float newX, float newZ, const std::vector<std::vector<int>> &map)
    {
        int mapWidth = static_cast<int>(map[0].size());
        int mapHeight = static_cast<int>(map.size());

        if (newX - radius < 0 || newX + radius > mapWidth || newZ - radius < 0 || newZ + radius > mapHeight)
        {
            if (isCollisionLogEnabled)
                std::cout << "Boundary collision at (" << newX << ", " << newZ << ")\n";
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
                    float closestX = std::max(static_cast<float>(x), std::min(newX, static_cast<float>(x + 1)));
                    float closestZ = std::max(static_cast<float>(z), std::min(newZ, static_cast<float>(z + 1)));
                    float dx = newX - closestX;
                    float dz = newZ - closestZ;
                    float distance = std::sqrt(dx * dx + dz * dz);
                    if (distance < radius)
                    {
                        if (isCollisionLogEnabled)
                            std::cout << "Wall collision at (" << x << ", " << z << "), distance: " << distance << "\n";
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void move(bool forward, bool backward, bool turnLeft, bool turnRight, const std::vector<std::vector<int>> &map)
    {
        float rad = angle * M_PI / 180.0f;
        float dirX = sin(rad);
        float dirZ = -cos(rad);

        float newX = x;
        float newZ = z;

        if (turnLeft)
            angle += turnSpeed;
        if (turnRight)
            angle -= turnSpeed;
        if (forward)
        {
            newX += dirX * moveSpeed;
            newZ += dirZ * moveSpeed;
        }
        if (backward)
        {
            newX -= dirX * moveSpeed;
            newZ -= dirZ * moveSpeed;
        }

        if (!collides(newX, newZ, map) || !isCollisionsEnabled)
        {
            x = newX;
            z = newZ;
        }

        if ((forward || backward || turnLeft || turnRight) && isPositionLogEnabled)
        {
            std::cout << "Position: (" << x << ", " << y << ", " << z << "), Angle: " << angle << "°\n";
        }
    }
};

class Level
{
private:
    std::vector<std::vector<int>> map;
    int width, height;
    GLuint vao, vbo;           // For cubes
    GLuint floorVao, floorVbo; // For floor/ceiling
    CubeShader *cubeShader;
    Shader *flatShader;
    std::vector<float> cubeVertexData;
    std::vector<float> flatVertexData;

    const char *cubeVertexShaderSource = R"(
        #version 330 core
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

    const char *cubeFragmentShaderSource = R"(
        #version 330 core
        in vec3 fragColor;
        uniform bool useOverrideColor;
        out vec4 FragColor;
        void main() {
            vec3 color = useOverrideColor ? vec3(0.0) : fragColor;
            FragColor = vec4(color, 1.0);
        }
    )";

    const char *flatVertexShaderSource = R"(
        #version 330 core
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

    const char *flatFragmentShaderSource = R"(
        #version 330 core
        in vec3 fragColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(fragColor, 1.0);
        }
    )";

public:
    Level(const std::vector<std::vector<int>> &levelData)
            : map(levelData), height(static_cast<int>(map.size())), width(height > 0 ? static_cast<int>(map[0].size()) : 0)
    {
        std::cout << "Initializing Level...\n";
        cubeShader = new CubeShader(cubeVertexShaderSource, cubeFragmentShaderSource);
        flatShader = new Shader(flatVertexShaderSource, flatFragmentShaderSource);
        setupGeometry();
    }

    ~Level()
    {
        std::cout << "Destroying Level...\n";
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &floorVao);
        glDeleteBuffers(1, &floorVbo);
        delete cubeShader;
        delete flatShader;
    }

    void setupGeometry()
    {
        std::cout << "Setting up geometry...\n";
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
                -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f};

        cubeVertexData.clear();
        for (int z = 0; z < height; ++z)
        {
            for (int x = 0; x < width; ++x)
            {
                if (map[z][x] == 1)
                {
                    float r = static_cast<float>(x) / (width - 1);
                    float g = static_cast<float>(z) / (height - 1);
                    float b = 0.5f + 0.5f * (1.0f - r - g);
                    r = std::max(0.0f, std::min(1.0f, r));
                    g = std::max(0.0f, std::min(1.0f, g));
                    b = std::max(0.0f, std::min(1.0f, b));

                    for (int i = 0; i < 144; i += 6)
                    {
                        cubeVertexData.push_back(cubeVertices[i] + x + 0.5f);
                        cubeVertexData.push_back(cubeVertices[i + 1]);
                        cubeVertexData.push_back(cubeVertices[i + 2] + z + 0.5f);
                        cubeVertexData.push_back(r);
                        cubeVertexData.push_back(g);
                        cubeVertexData.push_back(b);
                    }
                    if (x == 0 && z == 0)
                    {
                        std::cout << "First cube vertex: ["
                                  << cubeVertexData[0] << ", "
                                  << cubeVertexData[1] << ", "
                                  << cubeVertexData[2] << "]\n";
                    }
                }
            }
        }

        // Floor and ceiling
        float floorColor[3] = {0.5f, 0.5f, 0.5f};
        flatVertexData.insert(flatVertexData.end(), {0.0f, -0.5f, 0.0f, floorColor[0], floorColor[1], floorColor[2]});
        flatVertexData.insert(flatVertexData.end(), {static_cast<float>(width), -0.5f, 0.0f, floorColor[0], floorColor[1], floorColor[2]});
        flatVertexData.insert(flatVertexData.end(), {static_cast<float>(width), -0.5f, static_cast<float>(height), floorColor[0], floorColor[1], floorColor[2]});
        flatVertexData.insert(flatVertexData.end(), {0.0f, -0.5f, static_cast<float>(height), floorColor[0], floorColor[1], floorColor[2]});

        float ceilingColor[3] = {0.3f, 0.3f, 0.7f};
        flatVertexData.insert(flatVertexData.end(), {0.0f, 0.5f, 0.0f, ceilingColor[0], ceilingColor[1], ceilingColor[2]});
        flatVertexData.insert(flatVertexData.end(), {static_cast<float>(width), 0.5f, 0.0f, ceilingColor[0], ceilingColor[1], ceilingColor[2]});
        flatVertexData.insert(flatVertexData.end(), {static_cast<float>(width), 0.5f, static_cast<float>(height), ceilingColor[0], ceilingColor[1], ceilingColor[2]});
        flatVertexData.insert(flatVertexData.end(), {0.0f, 0.5f, static_cast<float>(height), ceilingColor[0], ceilingColor[1], ceilingColor[2]});

        // Cube VAO/VBO
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, cubeVertexData.size() * sizeof(float), cubeVertexData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        std::cout << "Cube vertices: " << cubeVertexData.size() / 6 << "\n";

        // Floor/Ceiling VAO/VBO
        glGenVertexArrays(1, &floorVao);
        glGenBuffers(1, &floorVbo);
        glBindVertexArray(floorVao);

        glBindBuffer(GL_ARRAY_BUFFER, floorVbo);
        glBufferData(GL_ARRAY_BUFFER, flatVertexData.size() * sizeof(float), flatVertexData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        std::cout << "Flat vertices: " << flatVertexData.size() / 6 << "\n";
    }

    void render(const Player &player)
    {
        float aspect = static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT;
        float fovRad = 45.0f * M_PI / 180.0f;
        float nearPlane = 0.1f, farPlane = 100.0f;
        float projection[16] = {
                1.0f / (aspect * (float)tan(fovRad / 2)), 0, 0, 0,
                0, 1.0f / (float)tan(fovRad / 2), 0, 0,
                0, 0, -(farPlane + nearPlane) / (farPlane - nearPlane), -1,
                0, 0, -(2 * farPlane * nearPlane) / (farPlane - nearPlane), 0};
        float rad = player.angle * M_PI / 180.0f;
        float eyeX = player.x, eyeY = player.y + 0.05f, eyeZ = player.z;
        float centerX = eyeX + sin(rad), centerY = eyeY, centerZ = eyeZ - cos(rad);
        float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
        float fX = centerX - eyeX, fY = centerY - eyeY, fZ = centerZ - eyeZ;
        float fLen = sqrt(fX * fX + fY * fY + fZ * fZ);
        fX /= fLen;
        fY /= fLen;
        fZ /= fLen;
        float sX = upY * fZ - upZ * fY, sY = upZ * fX - upX * fZ, sZ = upX * fY - upY * fX;
        float sLen = sqrt(sX * sX + sY * sY + sZ * sZ);
        sX /= sLen;
        sY /= sLen;
        sZ /= sLen;
        float uX = fY * sZ - fZ * sY, uY = fZ * sX - fX * sZ, uZ = fX * sY - fY * sX;
        float view[16] = {
                sX, uX, -fX, 0,
                sY, uY, -fY, 0,
                sZ, uZ, -fZ, 0,
                -(sX * eyeX + sY * eyeY + sZ * eyeZ), -(uX * eyeX + uY * eyeY + uZ * eyeZ), -(-fX * eyeX - fY * eyeY - fZ * eyeZ), 1};
        float model[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

        // Render cubes
        cubeShader->use();
        // Pass matrices (assuming glm::mat4)
        glUniformMatrix4fv(glGetUniformLocation(cubeShader->program, "projection"), 1, GL_FALSE, projection);
        glUniformMatrix4fv(glGetUniformLocation(cubeShader->program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(cubeShader->program, "model"), 1, GL_FALSE, model);
        glBindVertexArray(vao);

        // First pass: Filled cubes, all faces
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE); // No culling
        glUniform1i(glGetUniformLocation(cubeShader->program, "useOverrideColor"), GL_FALSE);
        glDrawArrays(GL_QUADS, 0, cubeVertexData.size() / 6); // 1036 quads (if correct)
        // glBindVertexArray(0);

        if (isWireRendered)
        {
            // Second pass: Red wireframe, player-facing faces
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(3.0f); // Thicker lines (adjust as needed)
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glDepthFunc(GL_LEQUAL);
            glUniform1i(glGetUniformLocation(cubeShader->program, "useOverrideColor"), GL_TRUE); // Red
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.0f);
            glDrawArrays(GL_QUADS, 0, cubeVertexData.size() / 6);
            glDisable(GL_POLYGON_OFFSET_LINE);

            // Reset state
            glLineWidth(1.0f); // Reset to default
            glDisable(GL_CULL_FACE);
            glDepthFunc(GL_LESS);
        }

        glBindVertexArray(0);

        // Render floor/ceiling
        flatShader->use();
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "projection"), 1, GL_FALSE, projection);
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(flatShader->program, "model"), 1, GL_FALSE, model);
        glBindVertexArray(floorVao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Back to normal
        glDrawArrays(GL_QUADS, 0, flatVertexData.size() / 6);
        glBindVertexArray(0);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL Error during render: " << error << "\n";
        }
    }

    const std::vector<std::vector<int>> &getMap() const { return map; }
};

std::vector<std::vector<int>> generateCustomMap(int width, int height)
{
    if (width <= 10 || height <= 10)
    {
        std::cerr << "Invalid map dimensions: width=" << width << ", height=" << height << ". Using 10x10 default.\n";
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
    std::cout << "Starting program...\n";

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return -1;
    }
    std::cout << "SDL initialized.\n";

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window *window = SDL_CreateWindow("3D Engine OpenGL 3.1", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "GL context creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK)
    {
        std::cerr << "GLEW init failed: " << glewGetErrorString(glewStatus) << "\n";
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Player player(1.5f, 0.0f, 1.5f);
    Level level(generateCustomMap(30, 30));
    std::cout << "Player start: (" << player.x << ", " << player.y << ", " << player.z << "), Angle: " << player.angle << "°\n";

    bool running = true;
    SDL_Event event;
    bool moveForward = false, moveBackward = false, turnLeft = false, turnRight = false;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
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
                }
            }
            else if (event.type == SDL_EVENT_KEY_UP)
            {
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
            }
        }

        player.move(moveForward, moveBackward, turnLeft, turnRight, level.getMap());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        level.render(player);
        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    std::cout << "Cleaning up...\n";
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Program ended.\n";
    return 0;
}