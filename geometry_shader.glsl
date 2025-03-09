#version 330 core
layout (points) in; // ������� ������ � �����
layout (line_strip, max_vertices = 100) out; // �������� ������ � �����

void main() {
    // ������ �����
    for (float x = -0.8; x <= 0.8; x += 0.01) {
        float y = 0.1 * sin(10.0 * x + gl_in[0].gl_Position.x); // ������� �����
        gl_Position = vec4(x, y, 0.0, 1.0); // ������� �������
        EmitVertex();
    }
    EndPrimitive(); // ��������� ��������
}