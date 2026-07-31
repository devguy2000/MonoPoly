#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class SceneGraph;

// Minimal immediate-mode 2D batch renderer.
// All draw calls accumulate into a vertex buffer; Flush() submits one draw call.
class Renderer2D {
public:
    Renderer2D() = default;
    ~Renderer2D() { Shutdown(); }

    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    void Init();
    void Shutdown();

    // Call once per frame before drawing
    void BeginScene(const glm::mat4& viewProjection);
    // Submit and reset
    void EndScene();

    // Solid colored quad (world space)
    void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
    // Textured quad (flipX/flipY mirror the UV coordinates)
    void DrawQuad(const glm::mat4& transform, GLuint textureID,
                  const glm::vec4& tint = {1,1,1,1},
                  bool flipX = false, bool flipY = false);

    // Single line segment
    void DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color);

    // Collider debug outlines (wireframe)
    void DrawRect(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color);
    void DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 32);

    // Walk the scene and draw everything
    void DrawScene(SceneGraph& scene);

private:
    void Flush();
    void FlushLines();

    struct QuadVertex {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 uv;
        float     texIndex;
    };

    struct LineVertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    static constexpr uint32_t kMaxQuads    = 10000;
    static constexpr uint32_t kMaxVertices = kMaxQuads * 4;
    static constexpr uint32_t kMaxIndices  = kMaxQuads * 6;
    static constexpr uint32_t kMaxTexSlots = 16;

    // Quad batch
    GLuint m_quadVAO = 0, m_quadVBO = 0, m_quadEBO = 0;
    GLuint m_quadShader = 0;
    QuadVertex* m_quadBase    = nullptr;
    QuadVertex* m_quadPtr     = nullptr;
    uint32_t    m_quadCount   = 0;
    GLuint      m_texSlots[kMaxTexSlots] = {};
    uint32_t    m_texSlotIdx  = 1; // slot 0 = white 1x1
    GLuint      m_whiteTex    = 0;

    // Line batch
    GLuint m_lineVAO = 0, m_lineVBO = 0;
    GLuint m_lineShader = 0;
    LineVertex* m_lineBase  = nullptr;
    LineVertex* m_linePtr   = nullptr;
    uint32_t    m_lineCount = 0;
    static constexpr uint32_t kMaxLines = 5000;

    glm::mat4 m_viewProj = glm::mat4(1.f);

    GLuint CompileShader(const char* vs, const char* fs);
};
