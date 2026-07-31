#include "Renderer2D.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/Components.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstring>
#include <stdexcept>

// ---------------------------------------------------------------------------
// GLSL sources
// ---------------------------------------------------------------------------
static const char* kQuadVS = R"(
#version 460 core
layout(location=0) in vec3  a_pos;
layout(location=1) in vec4  a_color;
layout(location=2) in vec2  a_uv;
layout(location=3) in float a_tex;

uniform mat4 u_viewProj;

out vec4  v_color;
out vec2  v_uv;
out float v_tex;

void main(){
    gl_Position = u_viewProj * vec4(a_pos, 1.0);
    v_color = a_color;
    v_uv    = a_uv;
    v_tex   = a_tex;
}
)";

static const char* kQuadFS = R"(
#version 460 core
in vec4  v_color;
in vec2  v_uv;
in float v_tex;

uniform sampler2D u_textures[16];

out vec4 frag_color;

void main(){
    int idx = int(v_tex);
    vec4 texColor = texture(u_textures[idx], v_uv);
    frag_color = texColor * v_color;
}
)";

static const char* kLineVS = R"(
#version 460 core
layout(location=0) in vec3 a_pos;
layout(location=1) in vec4 a_color;

uniform mat4 u_viewProj;
out vec4 v_color;

void main(){
    gl_Position = u_viewProj * vec4(a_pos, 1.0);
    v_color = a_color;
}
)";

static const char* kLineFS = R"(
#version 460 core
in vec4 v_color;
out vec4 frag_color;
void main(){ frag_color = v_color; }
)";

// ---------------------------------------------------------------------------
GLuint Renderer2D::CompileShader(const char* vs, const char* fs) {
    auto compile = [](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint v = compile(GL_VERTEX_SHADER,   vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// ---------------------------------------------------------------------------
void Renderer2D::Init() {
    // --- White 1x1 texture for solid quads ---
    glGenTextures(1, &m_whiteTex);
    glBindTexture(GL_TEXTURE_2D, m_whiteTex);
    uint32_t white = 0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_texSlots[0] = m_whiteTex;

    // --- Quad batch ---
    m_quadBase = new QuadVertex[kMaxVertices];
    m_quadShader = CompileShader(kQuadVS, kQuadFS);

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, kMaxVertices * sizeof(QuadVertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, color));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, texIndex));

    // Pre-build index buffer
    auto* indices = new uint32_t[kMaxIndices];
    for (uint32_t i = 0, offset = 0; i < kMaxIndices; i += 6, offset += 4) {
        indices[i+0] = offset+0; indices[i+1] = offset+1; indices[i+2] = offset+2;
        indices[i+3] = offset+2; indices[i+4] = offset+3; indices[i+5] = offset+0;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, kMaxIndices * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    delete[] indices;

    glBindVertexArray(0);

    // --- Line batch ---
    m_lineBase   = new LineVertex[kMaxLines * 2];
    m_lineShader = CompileShader(kLineVS, kLineFS);

    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, kMaxLines * 2 * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));
    glBindVertexArray(0);
}

void Renderer2D::Shutdown() {
    delete[] m_quadBase; m_quadBase = nullptr;
    delete[] m_lineBase; m_lineBase = nullptr;
    if (m_quadVAO) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO) { glDeleteBuffers(1, &m_quadVBO); m_quadVBO = 0; }
    if (m_quadEBO) { glDeleteBuffers(1, &m_quadEBO); m_quadEBO = 0; }
    if (m_lineVAO) { glDeleteVertexArrays(1, &m_lineVAO); m_lineVAO = 0; }
    if (m_lineVBO) { glDeleteBuffers(1, &m_lineVBO); m_lineVBO = 0; }
    if (m_whiteTex){ glDeleteTextures(1, &m_whiteTex); m_whiteTex = 0; }
    if (m_quadShader){ glDeleteProgram(m_quadShader); m_quadShader = 0; }
    if (m_lineShader){ glDeleteProgram(m_lineShader); m_lineShader = 0; }
}

// ---------------------------------------------------------------------------
void Renderer2D::BeginScene(const glm::mat4& viewProjection) {
    m_viewProj   = viewProjection;
    m_quadPtr    = m_quadBase;
    m_quadCount  = 0;
    m_linePtr    = m_lineBase;
    m_lineCount  = 0;
    m_texSlotIdx = 1;
    m_texSlots[0] = m_whiteTex;
}

void Renderer2D::EndScene() {
    Flush();
    FlushLines();
}

// ---------------------------------------------------------------------------
void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color) {
    DrawQuad(transform, 0 /*white*/, color);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, GLuint textureID, const glm::vec4& tint) {
    if (m_quadCount >= kMaxQuads) { Flush(); m_quadPtr = m_quadBase; m_quadCount = 0; m_texSlotIdx = 1; }

    float texIdx = 0.f;
    if (textureID != 0) {
        for (uint32_t i = 1; i < m_texSlotIdx; ++i) {
            if (m_texSlots[i] == textureID) { texIdx = (float)i; goto found; }
        }
        if (m_texSlotIdx >= kMaxTexSlots) { Flush(); m_quadPtr = m_quadBase; m_quadCount = 0; m_texSlotIdx = 1; }
        texIdx = (float)m_texSlotIdx;
        m_texSlots[m_texSlotIdx++] = textureID;
        found:;
    }

    static const glm::vec4 kCorners[4] = {
        {-0.5f, -0.5f, 0.f, 1.f},
        { 0.5f, -0.5f, 0.f, 1.f},
        { 0.5f,  0.5f, 0.f, 1.f},
        {-0.5f,  0.5f, 0.f, 1.f},
    };
    static const glm::vec2 kUVs[4] = {{0,0},{1,0},{1,1},{0,1}};

    for (int i = 0; i < 4; ++i) {
        m_quadPtr->position = transform * kCorners[i];
        m_quadPtr->color    = tint;
        m_quadPtr->uv       = kUVs[i];
        m_quadPtr->texIndex = texIdx;
        ++m_quadPtr;
    }
    ++m_quadCount;
}

// ---------------------------------------------------------------------------
void Renderer2D::DrawRect(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color) {
    glm::vec2 hs = size * 0.5f;
    glm::vec2 corners[4] = {
        center + glm::vec2{-hs.x, -hs.y},
        center + glm::vec2{ hs.x, -hs.y},
        center + glm::vec2{ hs.x,  hs.y},
        center + glm::vec2{-hs.x,  hs.y},
    };
    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        m_linePtr->position = {corners[i], 0.f}; m_linePtr->color = color; ++m_linePtr;
        m_linePtr->position = {corners[j], 0.f}; m_linePtr->color = color; ++m_linePtr;
        ++m_lineCount;
    }
}

void Renderer2D::DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments) {
    for (int i = 0; i < segments; ++i) {
        float a0 = (float)i       / segments * 6.2831853f;
        float a1 = (float)(i + 1) / segments * 6.2831853f;
        m_linePtr->position = {center + glm::vec2{std::cos(a0), std::sin(a0)} * radius, 0.f};
        m_linePtr->color    = color; ++m_linePtr;
        m_linePtr->position = {center + glm::vec2{std::cos(a1), std::sin(a1)} * radius, 0.f};
        m_linePtr->color    = color; ++m_linePtr;
        ++m_lineCount;
    }
}

// ---------------------------------------------------------------------------
void Renderer2D::DrawScene(SceneGraph& scene) {
    auto view = scene.View<Transform2D, SpriteRenderer>();
    for (auto [entity, tf, sr] : view.each()) {
        if (sr.textureID != 0)
            DrawQuad(tf.GetMatrix(), sr.textureID, sr.color);
        else
            DrawQuad(tf.GetMatrix(), sr.color);
    }

    // Collider overlays
    static const glm::vec4 kColliderColor = {0.f, 1.f, 0.f, 0.8f};
    for (auto [entity, tf, bc] : scene.View<Transform2D, BoxCollider2D>().each())
        DrawRect(tf.position + bc.offset, bc.size * tf.scale, kColliderColor);

    for (auto [entity, tf, cc] : scene.View<Transform2D, CircleCollider2D>().each())
        DrawCircle(tf.position + cc.offset, cc.radius * glm::max(tf.scale.x, tf.scale.y), kColliderColor);
}

// ---------------------------------------------------------------------------
void Renderer2D::Flush() {
    if (m_quadCount == 0) return;

    // Bind textures
    for (uint32_t i = 0; i < m_texSlotIdx; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_texSlots[i]);
    }

    glUseProgram(m_quadShader);
    glUniformMatrix4fv(glGetUniformLocation(m_quadShader, "u_viewProj"), 1, GL_FALSE, glm::value_ptr(m_viewProj));

    // Set sampler indices
    int samplers[kMaxTexSlots];
    for (int i = 0; i < (int)kMaxTexSlots; ++i) samplers[i] = i;
    glUniform1iv(glGetUniformLocation(m_quadShader, "u_textures"), kMaxTexSlots, samplers);

    uint32_t dataSize = (uint32_t)((uint8_t*)m_quadPtr - (uint8_t*)m_quadBase);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, m_quadBase);
    glDrawElements(GL_TRIANGLES, m_quadCount * 6, GL_UNSIGNED_INT, nullptr);

    m_quadPtr   = m_quadBase;
    m_quadCount = 0;
    m_texSlotIdx = 1;
}

void Renderer2D::FlushLines() {
    if (m_lineCount == 0) return;

    uint32_t dataSize = (uint32_t)((uint8_t*)m_linePtr - (uint8_t*)m_lineBase);
    glUseProgram(m_lineShader);
    glUniformMatrix4fv(glGetUniformLocation(m_lineShader, "u_viewProj"), 1, GL_FALSE, glm::value_ptr(m_viewProj));

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, m_lineBase);
    glDrawArrays(GL_LINES, 0, m_lineCount * 2);

    m_linePtr   = m_lineBase;
    m_lineCount = 0;
}
