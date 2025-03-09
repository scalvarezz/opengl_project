#version 330 core
layout (points) in; // Входные данные — точки
layout (line_strip, max_vertices = 100) out; // Выходные данные — линии

void main() {
    // Создаём волну
    for (float x = -0.8; x <= 0.8; x += 0.01) {
        float y = 0.1 * sin(10.0 * x + gl_in[0].gl_Position.x); // Формула волны
        gl_Position = vec4(x, y, 0.0, 1.0); // Позиция вершины
        EmitVertex();
    }
    EndPrimitive(); // Завершаем примитив
}