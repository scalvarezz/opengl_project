#version 330 core
layout (location = 0) in vec2 aPos; // ¬ходные данные Ч координаты точки

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0); // ѕередаЄм точку в geometry shader
}