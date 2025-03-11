
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <chrono>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <vector>

constexpr float M_PI = 3.14159265358979323846f;

class Wave {
public:
    float amplitude;
    float frequency;
    Wave(float amplitude, float frequency)
        :amplitude(amplitude), frequency(frequency) {
    };

    ~Wave() {
        amplitude = 0;
        frequency = 0;
    }

    Wave addBeats(float time, const Wave& wave1, const Wave& wave2) {
        float omega = (wave1.frequency + wave2.frequency);
        Wave wave_res((wave1.amplitude + wave2.amplitude) / 2.0f * cos(time), omega * cos(time) * 10.0f);
        return wave_res;
    }
};

class Renderer {
    unsigned int shaderProgram;
public:
    Renderer() {}
    Renderer(unsigned int shaderProgram) : shaderProgram(shaderProgram) {}

    void DrawRect(float x1, float y1, float x2, float y2, float r, float g, float b) const {
        float vertices[] = {
            x1, y1, 0.0f,
            x1, y2, 0.0f,
            x2, y2, 0.0f,
            x2, y1, 0.0f
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        GLuint VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glUseProgram(shaderProgram);
        glUniform3f(glGetUniformLocation(shaderProgram, "ourColor"), r, g, b);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    void DrawCircle(float x_c, float y_c, float radius, float r, float g, float b) const {
        const int segments = 25;
        float vertices[segments * 3 + 3];

        vertices[0] = x_c;
        vertices[1] = y_c;
        vertices[2] = 0.0f;

        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            vertices[3 + i * 3] = x_c + radius * cos(angle);
            vertices[3 + i * 3 + 1] = y_c + radius * sin(angle);
            vertices[3 + i * 3 + 2] = 0.0f;
        }

        unsigned int indices[segments * 3];
        for (int i = 0; i < segments; i++) {
            indices[i * 3] = 0;
            indices[i * 3 + 1] = i + 1;
            indices[i * 3 + 2] = (i + 1) % segments + 1;
        }

        GLuint VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glUseProgram(shaderProgram);
        glUniform3f(glGetUniformLocation(shaderProgram, "ourColor"), r, g, b);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, segments * 3, GL_UNSIGNED_INT, 0);

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    void DrawWave(const Wave& wave, float time) {
        const int numPoints = 50;
        glLineWidth(5.0f);

        float wavePoints[numPoints * 2];
        for (int i = 0; i < numPoints; i++) {
            float x = -0.8f + 0.85f * i / (numPoints - 1);
            float y = pow(cos(x * 7.0f + time * 2.0f), 2) * wave.amplitude * cos(2.0f * wave.frequency * x + wave.frequency * time) + 0.12f;
            wavePoints[i * 2] = x;
            wavePoints[i * 2 + 1] = y;
        }

        GLuint VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(wavePoints), wavePoints, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(wavePoints), wavePoints);

        glUseProgram(shaderProgram);

        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_STRIP, 0, numPoints);
        glBindVertexArray(0);
    }
    void DrawOscilloscope() const {
        DrawRect(-0.9f, -0.8f, 0.9f, 0.8f, 0.95f, 0.95f, 0.65f);
        DrawRect(0.15f, -0.75f, 0.87f, 0.75f, 0.9f, 0.9f, 0.5f);
        DrawRect(0.17f, -0.72f, 0.85f, 0.72f, 0.9f, 0.9f, 0.6f);
        DrawRect(-0.87f, -0.5f, 0.12f, 0.75f, 0.9f, 0.9f, 0.46f);
        DrawRect(-0.8f, -0.4f, 0.05f, 0.65f, 0.1f, 0.45f, 0.25f);
        DrawRect(0.72f, 0.57f, 0.82f, 0.67f, 1.0f, 0.2f, 0.2f);

        for (int i = 0; i < 3; i++) {
            float xOffset = 0.22f * i;
            DrawRect(0.2f + xOffset, -0.31f, 0.39f + xOffset, 0.05f, 1.0f, 1.0f, 0.56f);
            DrawCircle(0.295f + xOffset, -0.13f, 0.086f, 0.8f, 0.8f, 0.4f);
            DrawCircle(0.295f + xOffset, -0.53f, 0.05f, 0.7f, 0.7f, 0.5f);
            DrawCircle(0.295f + xOffset, -0.53f, 0.04f, 0.9f, 0.9f, 0.7f);
            DrawCircle(0.295f + xOffset, -0.53f, 0.02f, 0.3f, 0.3f, 0.2f);
            DrawRect(0.26f + xOffset, -0.45f, 0.33f + xOffset, -0.38f, 1.0f, 1.0f, 0.76f);
            DrawCircle(0.295f + xOffset, 0.2f, 0.06f, 0.5f, 0.5f, 0.4f);
            DrawRect(-0.67f + xOffset, -0.75f, -0.52f + xOffset, -0.55f, 1.0f, 1.0f, 0.76f);
        }

        for (int i = 0; i < 2; i++) {
            float xOffset = 1.4f * i;
            DrawRect(-0.8f + xOffset, -0.91f, -0.6f + xOffset, -0.81f, 0.7f, 0.7f, 0.5f);
        }
    }
};

class AudioManager {
    ALCdevice* device;
    ALCcontext* context;
    ALuint buffer;
    ALuint source;
public:
    AudioManager() {
        device = alcOpenDevice(nullptr);
        context = alcCreateContext(device, nullptr);
        buffer = 0;
        source = 0;
        alcMakeContextCurrent(context);
    };
    ~AudioManager() {
        alcDestroyContext(context);
        alcCloseDevice(device);
    }
    void loadAudio(const std::string& filePath) {
        SF_INFO fileInfo;
        SNDFILE* file = sf_open(filePath.c_str(), SFM_READ, &fileInfo);
        if (!file) {
            std::cerr << "Failed to open audio file: " << filePath << std::endl;
            return;
        }

        std::vector<short> audioData(fileInfo.frames * fileInfo.channels);
        sf_readf_short(file, audioData.data(), fileInfo.frames);
        sf_close(file);
        alGenBuffers(1, &buffer);
        alBufferData(buffer, fileInfo.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
            audioData.data(), static_cast<ALsizei>(audioData.size() * sizeof(short)), static_cast<ALsizei>(fileInfo.samplerate));
    };

    void playAudio() {
        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, buffer);
        alSourcePlay(source);
    };

    ALuint getsource() {
        return source;
    }
    void stopAudio() {
        alSourceStop(source);
    }
    void setVolume(float volume) {
        alSourcef(source, AL_GAIN, volume);
    };
};

bool keyZPressed = false;
bool keyXPressed = false;
bool keyCPressed = false;
bool MousePressed = true;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        keyZPressed = true;
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        keyXPressed = true;
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        keyCPressed = true;

    if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        keyZPressed = false;
    if (key == GLFW_KEY_X && action == GLFW_RELEASE)
        keyXPressed = false;
    if (key == GLFW_KEY_C && action == GLFW_RELEASE)
        keyCPressed = false;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        MousePressed = false;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        MousePressed = true;
}

class Game {
public:
    Game(unsigned int width, unsigned int height)
        : width(width), height(height), isRunning(false), isOscilloscopeOn(false) {
        init();
    }

    ~Game() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void run() {
        auto start_time = std::chrono::high_resolution_clock::now();
        audioManager.loadAudio("C:\\audio\\Cosmos(Outer_Space).wav");
        while (isRunning && !glfwWindowShouldClose(window)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float>(currentTime - start_time).count();
            render(time);
            update(time);

            glfwSwapBuffers(window);
            glfwPollEvents(); // Обработка событий
        }
    }
private:
    GLFWwindow* window;
    unsigned int width, height;
    unsigned int shaderProgram;
    Renderer renderer;
    AudioManager audioManager;
    bool isRunning;
    bool isOscilloscopeOn;
    float startTime;
    int i = 0;
    float presstime[4] = { 3.0f,5.0f,7.0f,500.0f };
    int pressbutton[3] = { 0,1,20 };

    void init() {
        if (!glfwInit()) {
            std::cerr << "Не удалось инициализировать GLFW!" << std::endl;
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, "FSR-Sonification", nullptr, nullptr);
        if (!window) {
            std::cerr << "Не удалось создать окно GLFW!" << std::endl;
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Не удалось инициализировать GLAD!" << std::endl;
            return;
        }

        const char* vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

        const char* fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 ourColor;
        void main() {
            FragColor = vec4(ourColor, 1.0);
        }
    )";

        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        renderer = Renderer(shaderProgram);

        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);

        isRunning = true;
    }

    void update(float time) {
        if (startTime > 0.0f) {
            if (i < 3 && time - startTime >= presstime[i] && time - startTime <= presstime[i] + 0.5f) {
                if (pressbutton[i] > 9) {
                    renderer.DrawRect(-0.67f + 0.22f * (pressbutton[i] / 10), -0.75f, -0.52f + 0.22f * (pressbutton[i] / 10), -0.55f, 1.0f, 0.0f, 0.0f);
                    renderer.DrawRect(-0.67f + 0.22f * (pressbutton[i] % 10), -0.75f, -0.52f + 0.22f * (pressbutton[i] % 10), -0.55f, 1.0f, 0.0f, 0.0f);
                }
                else {
                    renderer.DrawRect(-0.67f + 0.22f * pressbutton[i], -0.75f, -0.52f + 0.22f * pressbutton[i], -0.55f, 1.0f, 0.0f, 0.0f);
                }
            }
            else if (time - startTime > presstime[i] + 0.5f) {
                i++;
            }
        }
    }

    void render(float time) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.DrawOscilloscope();

        // Проверка состояния музыки
        ALint sourceState;
        alGetSourcei(audioManager.getsource(), AL_SOURCE_STATE, &sourceState);

        if (!MousePressed && sourceState != AL_PLAYING) {
            audioManager.playAudio(); // Запуск музыки, если осциллограф включен, а музыка не играет
            startTime = time;
        }
        if (MousePressed && sourceState == AL_PLAYING) {
            audioManager.stopAudio(); // Остановка музыки, если осциллограф выключен, а музыка играет
            startTime = -1.0f;
            i = 0;
        }

        if (!MousePressed) {
            Wave base_wave = { 0.03f, 100.0f };
            if (!keyZPressed && !keyXPressed && !keyCPressed) {
                renderer.DrawWave(base_wave, time);
            }
            Wave wave1 = { 0.05f, 21.0f };
            Wave wave2 = { 0.25f, 42.0f };
            Wave wave3 = { 0.45f, 63.0f };
            if (keyZPressed && !keyXPressed && !keyCPressed) {
                renderer.DrawWave(wave1, time);
            }
            if (keyXPressed && !keyZPressed && !keyCPressed) {
                renderer.DrawWave(wave2, time);
            }
            if (keyCPressed && !keyZPressed && !keyXPressed) {
                renderer.DrawWave(wave3, time);
            }

            else if (keyZPressed && keyXPressed) {
                renderer.DrawWave(wave1.addBeats(time, wave1, wave2), time);
            }
            else if (keyXPressed && keyCPressed) {
                renderer.DrawWave(wave2.addBeats(time, wave2, wave3), time);
            }
            else if (keyZPressed && keyCPressed) {
                renderer.DrawWave(wave3.addBeats(time, wave1, wave3), time);
            }

        }
        else if (MousePressed) {
            renderer.DrawRect(-0.8f, -0.4f, 0.05f, 0.65f, 0.15f, 0.15f, 0.15f);
        }
    }
};


// Точка входа
int main() {
    Game game(1000, 800);
    game.run();
    return 0;
}
