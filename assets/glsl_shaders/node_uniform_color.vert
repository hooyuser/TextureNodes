#version 460

// gl_VertexIndex      UV
// 0                  (0,0)
// 1                  (2,0)
// 2                  (0,2)

void main() {
    vec2 outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}