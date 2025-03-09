#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <chrono> 
#include <ft2build.h>
#include <map>
#include <glm/glm.hpp>
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include FT_FREETYPE_H

constexpr float M_PI = 3.14;

unsigned int height = 800;
unsigned int width = 1000;

// Шейдерная программа
unsigned int shaderProgram;

struct Character {
    GLuint textureID;   // ID текстуры символа
    glm::ivec2 size;    // Размер символа
    glm::ivec2 bearing; // Смещение символа
    GLuint advance;     // Расстояние до следующего символа
};

std::map<GLchar, Character> Characters; // Хранилище символов
GLuint VAO, VBO;

void loadFont(const std::string& fontPath, GLuint fontSize) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Загрузка символов
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

#define checkOpenALError() checkOpenALError_(__FILE__, __LINE__)
bool checkOpenALError_(const char* file, int line) {
    ALCenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error (" << file << ":" << line << "): " << alGetString(error) << std::endl;
        return true;
    }
    return false;
}

struct Wave {
    float amplitude; 
    float frequency; 
    float phase;    
};

enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

//class Game
//{
//public:
//    GameState    State;
//    bool         Keys[3];
//    unsigned int Width, Height;
//    Game(unsigned int width, unsigned int height);
//    ~Game();
//    void Init();
//    void ProcessInput(float dt);
//    void Update(float dt);
//    void Render();
//};

// Функция для рисования прямоугольника
void DrawRect(float x1, float y1, float x2, float y2, float r, float g, float b) {
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

    unsigned int VBO, VAO, EBO;
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

// Функция для рисования круга
void DrawCircle(float x_c, float y_c, float radius, float r, float g, float b) {
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

    unsigned int VBO, VAO, EBO;
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

// Функция для рисования волны
void DrawWave(Wave wave, float time) {
    // Количество точек для волны
    const int numPoints = 50;
    glLineWidth(5.0f); // Толщина линии в 3 пикселя

    // Массив координат точек
    float wavePoints[numPoints * 2]; // Каждая точка имеет 2 координаты (x, y)

    // Заполняем массив координатами волны
    for (int i = 0; i < numPoints; i++) {
        float x = -0.8f + 1.45f * i / (numPoints - 1); // X от -0.8 до 0.65
        float y = wave.amplitude * sin(10.0f * x); // Y вычисляется по формуле синуса
        wavePoints[i * 2] = x; // X координата
        wavePoints[i * 2 + 1] = y; // Y координата
    }

    // Создаём VAO и VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Привязываем VAO
    glBindVertexArray(VAO);

    // Привязываем и заполняем VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wavePoints), wavePoints, GL_STATIC_DRAW);

    // Указываем, как интерпретировать данные
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Отвязываем VAO и VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Обновляем массив точек для волны
    for (int i = 0; i < numPoints; i++) {
        float x = -0.8f + 0.85f * i / (numPoints - 1); // X от -0.8 до 0.6
        float y = wave.amplitude * cos(2.0f * wave.frequency * x + time * 3.0f) + 0.12f; // Y вычисляется по формуле синуса с фазой
        wavePoints[i * 2] = x; // X координата
        wavePoints[i * 2 + 1] = y; // Y координата
    }

    // Обновляем VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(wavePoints), wavePoints);

    // Используем шейдерную программу
    glUseProgram(shaderProgram);

    // Привязываем VAO
    glBindVertexArray(VAO);

    // Рисуем волну
    glDrawArrays(GL_LINE_STRIP, 0, numPoints); // Рисуем линию из точек

    // Отвязываем VAO
    glBindVertexArray(0);
}

Wave addWaves(float time, const Wave& wave1, const Wave& wave2) {
    Wave wave_res;
    wave_res.amplitude = abs(wave1.amplitude - wave2.amplitude);
    wave_res.frequency = wave1.frequency;
    wave_res.phase = wave1.phase - wave2.phase;
    return wave_res; 
}

Wave addBeats(float time, const Wave& wave1, const Wave& wave2) {
    Wave wave_res;
    float deltaOmega = wave1.frequency - wave2.frequency; 
    float omega = (wave1.frequency + wave2.frequency); 
    wave_res.amplitude = (wave1.amplitude + wave2.amplitude)/2.0f*cos(time);
    wave_res.frequency = omega*cos(time)*10.0f;
    wave_res.phase = (wave1.phase - wave2.phase);
    return wave_res;
}

void DrawOscilloscope() {
    // Рисуем общую область
    DrawRect(-0.9f, -0.8f, 0.9f, 0.8f, 0.95f, 0.95f, 0.65f); // Общая область
    DrawRect(0.15f, -0.75f, 0.87f, 0.75f, 0.9f, 0.9f, 0.5f); // Область работы с кнопками
    DrawRect(0.17f, -0.72f, 0.85f, 0.72f, 0.9f, 0.9f, 0.6f);
    DrawRect(-0.87f, -0.5f, 0.12f, 0.75f, 0.9f, 0.9f, 0.46f); // Область вокруг экрана
    DrawRect(-0.8f, -0.4f, 0.05f, 0.65f, 0.1f, 0.45f, 0.25f); // Экран
    DrawRect(0.72f, 0.57f, 0.82f, 0.67f, 1.0f, 0.2f, 0.2f);


    // Кнопки и круги
    for (int i = 0; i < 3; i++) {
        float xOffset = 0.22f * i;

        // Квадраты вольт
        DrawRect(0.2f + xOffset, -0.31f, 0.39f + xOffset, 0.05f, 1.0f, 1.0f, 0.56f);

        // Круги вольт
        DrawCircle(0.295f + xOffset, -0.13f, 0.086f, 0.8f, 0.8f, 0.4f);

        // Круги для подачи волн
        DrawCircle(0.295f + xOffset, -0.53f, 0.05f, 0.7f, 0.7f, 0.5f);
        DrawCircle(0.295f + xOffset, -0.53f, 0.04f, 0.9f, 0.9f, 0.7f);
        DrawCircle(0.295f + xOffset, -0.53f, 0.02f, 0.3f, 0.3f, 0.2f);

        // Кнопки каналов
        DrawRect(0.26f + xOffset, -0.45f, 0.33f + xOffset, -0.38f, 1.0f, 1.0f, 0.76f);

        // Круги Y-POS
        DrawCircle(0.295f + xOffset, 0.2f, 0.06f, 0.5f, 0.5f, 0.4f);

        // Кнопки под экраном
        DrawRect(-0.67f + xOffset, -0.75f, -0.52f + xOffset, -0.55f, 1.0f, 1.0f, 0.76f);
    }

    // Ножки
    for (int i = 0; i < 2; i++) {
        float xOffset = 1.4f * i;
        DrawRect(-0.8f + xOffset, -0.91f, -0.6f + xOffset, -0.81f, 0.7f, 0.7f, 0.5f);
    }
}

ALuint loadAudio(const std::string& filePath) {
    SF_INFO fileInfo;
    SNDFILE* file = sf_open(filePath.c_str(), SFM_READ, &fileInfo);
    if (!file) {
        std::cerr << "Failed to open audio file: " << filePath << std::endl;
        return 0;
    }

    // Чтение данных из файла
    std::vector<short> audioData(fileInfo.frames * fileInfo.channels);
    sf_readf_short(file, audioData.data(), fileInfo.frames);
    sf_close(file);

    // Создание буфера OpenAL
    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, fileInfo.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
        audioData.data(), audioData.size() * sizeof(short), fileInfo.samplerate);

    if (checkOpenALError()) {
        alDeleteBuffers(1, &buffer);
        return 0;
    }

    return buffer;
}

ALuint playAudio(ALuint buffer) {
    ALuint source;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    if (checkOpenALError()) {
        alDeleteSources(1, &source);
        return 0;
    }

    return source;
}

void RenderText(GLuint shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Проходим по всем символам строки
    for (std::string::const_iterator c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.bearing.x * scale;
        GLfloat ypos = y - (ch.size.y - ch.bearing.y) * scale;

        GLfloat w = ch.size.x * scale;
        GLfloat h = ch.size.y * scale;

        // Обновляем VBO для каждого символа
        GLfloat vertices[6][4] = {
            {xpos, ypos + h, 0.0, 0.0},
            {xpos, ypos, 0.0, 1.0},
            {xpos + w, ypos, 1.0, 1.0},

            {xpos, ypos + h, 0.0, 0.0},
            {xpos + w, ypos, 1.0, 1.0},
            {xpos + w, ypos + h, 1.0, 0.0}
        };

        // Отрисовываем текстуру символа
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Сдвигаем позицию для следующего символа
        x += (ch.advance >> 6) * scale; // Сдвиг в 1/64 пикселя
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        MousePressed = false;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        MousePressed = true;
}

void checkInput(float wavePosition) {
    int score = 0;
    if (keyZPressed && wavePosition >= -0.1f && wavePosition <= 0.1f) {
        score += 100;
    }
    if (keyXPressed && wavePosition >= -0.1f && wavePosition <= 0.1f) {
        score += 100;
    }
    if (keyCPressed && wavePosition >= -0.1f && wavePosition <= 0.1f) {
        score += 100;
    }
    //std::cout << "Score: " << score << std::endl;
}

int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Не удалось инициализировать GLFW!" << std::endl;
        return -1;
    }

    // Настройка GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна
    GLFWwindow* window = glfwCreateWindow(1000, 800, "FSR-Sonification", NULL, NULL);
    if (!window) {
        std::cerr << "Не удалось создать окно GLFW!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Сделаем окно текущим контекстом
    glfwMakeContextCurrent(window);

    // Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Не удалось инициализировать GLAD!" << std::endl;
        return -1;
    }

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR: Could not init FreeType Library" << std::endl;
        return -1;
    }
    std::cout << "FreeType initialized successfully!" << std::endl;
    FT_Done_FreeType(ft);

    // Загрузка шейдеров
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

    ALCdevice* device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open OpenAL device" << std::endl;
        return -1;
    }

    ALCcontext* context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Failed to create OpenAL context" << std::endl;
        alcCloseDevice(device);
        return -1;
    }

    alcMakeContextCurrent(context);

    // Проверка инициализации
    if (checkOpenALError()) {
        alcDestroyContext(context);
        alcCloseDevice(device);
        return -1;
    }

    // Инициализация OpenAL
    ALCdevice* device = alcOpenDevice(nullptr);
    ALCcontext* context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    // Загрузка аудиофайла
    ALuint buffer = loadAudio("path/to/your/music.wav");
    if (!buffer) {
        alcDestroyContext(context);
        alcCloseDevice(device);
        return -1;
    }

    // Воспроизведение музыки
    ALuint source = playAudio(buffer);
    if (!source) {
        alDeleteBuffers(1, &buffer);
        alcDestroyContext(context);
        alcCloseDevice(device);
        return -1;
    }

    std::cout << "OpenAL initialized successfully!" << std::endl;

    // Компиляция шейдеров
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Создание шейдерной программы
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    loadFont("C:/Windows/Fonts/Arial.ttf", 48);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    auto start_time = std::chrono::high_resolution_clock::now();

    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        // Вычисляем время с начала программы
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - start_time).count();
        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        // Очистка экрана
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        DrawOscilloscope();

        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            break; // Музыка закончилась
        }

        Wave base_wave = { 0.03f, 100.0f, 0.0f };
        if (!keyZPressed && !keyXPressed && !keyCPressed) {
            DrawWave(base_wave, time);
        }

        Wave wave1 = { 0.05f, 5.0f, 0.0f }; 
        Wave wave2 = { 0.25f, 6.0f, 0.0f }; 
        Wave wave3 = { 0.45f, 7.0f, 0.0f };

        float wavePosition = cos(2.0f * M_PI * 1.0f * time);
        
        if (keyZPressed && !keyXPressed && !keyCPressed) {
            DrawWave(wave1, time);
        }
        else if (keyXPressed && !keyZPressed && !keyCPressed) {
            DrawWave(wave2, time);
        }
        else if (keyCPressed && !keyZPressed && !keyXPressed) {
            DrawWave(wave3, time);
        }
        else if (keyZPressed && keyXPressed) {
            DrawWave(addWaves(time, wave1, wave2), time);
        }
        else if (keyXPressed && keyCPressed) {
            DrawWave(addWaves(time, wave2, wave3), time);
        }
        else if (keyZPressed && keyCPressed) {
            DrawWave(addBeats(time, wave1, wave3), time);
        }

        if (MousePressed) {
            DrawRect(-0.8f, -0.4f, 0.05f, 0.65f, 0.15f, 0.15f, 0.15f);
        }
        RenderText(shaderProgram, "Hello!", -0.5f, -0.5f, 10.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        // Проверка ввода и начисление очков
        checkInput(wavePosition);

        // Отвязываем VAO
        glBindVertexArray(0);

        // Удаляем шейдеры, так как они больше не нужны
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Обмен буферов
        glfwSwapBuffers(window);

        // Обработка событий
        glfwPollEvents();
    }

    // Освобождение ресурсов
    alcDestroyContext(context);
    alcCloseDevice(device);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}