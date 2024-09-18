#ifndef AVEN_GL_TEXT_H
#define AVEN_GL_TEXT_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/str.h>

#include "../gl.h"
#include "../glm.h"

typedef struct {
   unsigned short x0;
   unsigned short y0;
   unsigned short x1;
   unsigned short y1;
   float xoff;
   float yoff;
   float xadvance;
   float xoff2;
   float yoff2;
} AvenGLTextChar;

typedef struct {
    AvenGLTextChar packed_chars[96];
    float height;
    GLuint texture_id;
    int texture_width;
    int texture_height;
} AvenGLTextFont;

static inline AvenGLTextFont aven_gl_text_font_init(
    AvenGL *gl,
    ByteSlice font_bytes,
    float font_height,
    AvenArena arena
) {
    struct stbtt_pack_context {
        void *user_allocator_context;
        void *pack_info;
        int width;
        int height;
        int stride_in_bytes;
        int padding;
        int skip_missing;
        unsigned int h_oversample, v_oversample;
        unsigned char *pixels;
        void *nodes;
    };
    int stbtt_PackBegin(
        struct stbtt_pack_context *spc,
        unsigned char *pixels,
        int width,
        int height,
        int stride_in_bytes,
        int padding,
        void *alloc_context
    );
    void stbtt_PackEnd(struct stbtt_pack_context *spc);
    void stbtt_PackSetOversampling(
    struct stbtt_pack_context *spc,
        unsigned int h_oversample,
        unsigned int v_oversample
    );
    int stbtt_PackFontRange(
        struct stbtt_pack_context *spc,
        const unsigned char *fontdata,
        int font_index,
        float font_size,
        int first_unicode_char_in_range,
        int num_chars_in_range,
        AvenGLTextChar *chardata_for_range
    );

    AvenArena temp_arena;
    int ncols = 4;
    int nrows = 8;
    int char_dim = (int)((font_height + 2.0f) * 2.0f);

    int success = 0;
    AvenGLTextFont font = { .height = font_height };
    ByteSlice texture_bytes = { 0 };
    do {
        temp_arena = arena;

        font.texture_width = char_dim * ncols;
        font.texture_height = char_dim * nrows;
        texture_bytes.len = (size_t)font.texture_width *
            (size_t)font.texture_height;
        texture_bytes.ptr = aven_arena_alloc(&temp_arena, texture_bytes.len, 1);

        struct stbtt_pack_context ctx = { 0 };
        success = stbtt_PackBegin(
            &ctx,
            texture_bytes.ptr,
            font.texture_width,
            font.texture_height,
            0,
            1,
            &temp_arena
        );
        assert(success != 0);

        stbtt_PackSetOversampling(&ctx, 2, 2);

        success = stbtt_PackFontRange(
            &ctx,
            font_bytes.ptr,
            0,
            font_height,
            32,
            countof(font.packed_chars),
            font.packed_chars
        );

        stbtt_PackEnd(&ctx);

        nrows += 1;
    } while (success == 0);

    arena = temp_arena;

    assert(gl->GetError() == 0);
    gl->GenTextures(1, &font.texture_id);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, font.texture_id);
    assert(gl->GetError() == 0);
    gl->TexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA, 
        (GLsizei)font.texture_width,
        (GLsizei)font.texture_height,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        texture_bytes.ptr
    );
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    assert(gl->GetError() == 0);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);
    
    return font;
}

static inline void aven_gl_text_font_deinit(AvenGLTextFont *font, AvenGL *gl) {
    gl->DeleteTextures(1, &font->texture_id);
}

typedef struct {
    Vec2 pos;
    Vec2 tex;
    Vec4 color;
} AvenGLTextVertex;

typedef struct {
    AvenGLTextFont font;
    List(AvenGLTextVertex) vertices;
    List(GLushort) indices;
    GLuint vertex_buffer;
    GLuint index_buffer;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint utrans_location;
    GLuint upos_location;
    GLuint vpos_location;
    GLuint vtex_location;
    GLuint vcolor_location;
} AvenGLTextGeometry;

static inline AvenGLTextGeometry aven_gl_text_geometry_init(
    AvenGL *gl,
    AvenGLTextFont font,
    size_t max_chars,
    AvenArena *arena
) {
    AvenGLTextGeometry geometry = {
        .vertices = { .cap = max_chars * 4 },
        .indices = { .cap = max_chars * 6 },
        .font = font,
    };
    geometry.vertices.ptr = aven_arena_create_array(
        AvenGLTextVertex,
        arena,
        geometry.vertices.cap
    );
    geometry.indices.ptr = aven_arena_create_array(
        GLushort,
        arena,
        geometry.indices.cap
    );

    assert(gl->GetError() == 0);
    gl->GenBuffers(1, &geometry.vertex_buffer);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, geometry.vertex_buffer);
    assert(gl->GetError() == 0);
    gl->BufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)(geometry.vertices.cap * sizeof(*geometry.vertices.ptr)),
        geometry.vertices.ptr,
        GL_DYNAMIC_DRAW
    );
    assert(gl->GetError() == 0);

    gl->GenBuffers(1, &geometry.index_buffer);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.index_buffer);
    assert(gl->GetError() == 0);
    gl->BufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)(geometry.indices.cap * sizeof(*geometry.indices.ptr)),
        geometry.indices.ptr,
        GL_DYNAMIC_DRAW
    );
    assert(gl->GetError() == 0);

    static const char* vertex_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform mat2 uTrans;\n"
        "uniform vec2 uPos;\n"
        "attribute vec2 vTex;\n"
        "attribute vec2 vPos;\n"
        "attribute vec4 vColor;\n"
        "varying vec4 fColor;\n"
        "varying vec2 tCoord;\n"
        "void main() {\n"
        "    gl_Position = vec4(uTrans * (vPos.xy - uPos), 0.0, 1.0);\n"
        "    tCoord = vTex;\n"
        "    fColor = vColor;\n"
        "}\n";

    static const char* fragment_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform sampler2D texSampler;\n"
        "varying vec4 fColor;\n"
        "varying vec2 tCoord;\n"
        "void main() {\n"
        "    vec4 tColor = texture2D(texSampler, tCoord);\n"
        "    float alpha = tColor.w * fColor.w;\n"
        "    gl_FragColor = vec4(fColor.xyz, alpha);\n"
        "}\n";

    geometry.vertex_shader = gl->CreateShader(GL_VERTEX_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(geometry.vertex_shader, 1, &vertex_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(geometry.vertex_shader);
    assert(gl->GetError() == 0);

    geometry.fragment_shader = gl->CreateShader(GL_FRAGMENT_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(geometry.fragment_shader, 1, &fragment_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(geometry.fragment_shader);
    assert(gl->GetError() == 0);

    geometry.program = gl->CreateProgram();
    assert(gl->GetError() == 0);
    gl->AttachShader(geometry.program, geometry.vertex_shader);
    assert(gl->GetError() == 0);
    gl->AttachShader(geometry.program, geometry.fragment_shader);
    assert(gl->GetError() == 0);
    gl->LinkProgram(geometry.program);
    assert(gl->GetError() == 0);

    geometry.utrans_location = (GLuint)gl->GetUniformLocation(
        geometry.program,
        "uTrans"
    );
    assert(gl->GetError() == 0);
    geometry.upos_location = (GLuint)gl->GetUniformLocation(
        geometry.program,
        "uPos"
    );
    assert(gl->GetError() == 0);

    geometry.vpos_location = (GLuint)gl->GetAttribLocation(
        geometry.program,
        "vPos"
    );
    assert(gl->GetError() == 0);
    geometry.vtex_location = (GLuint)gl->GetAttribLocation(
        geometry.program,
        "vTex"
    );
    assert(gl->GetError() == 0);
    geometry.vcolor_location = (GLuint)gl->GetAttribLocation(
        geometry.program,
        "vColor"
    );
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);

    return geometry;
}

static inline void aven_gl_text_geometry_deinit(
    AvenGLTextGeometry *geometry,
    AvenGL *gl
) {
    gl->DeleteProgram(geometry->program);
    gl->DeleteShader(geometry->fragment_shader);
    gl->DeleteShader(geometry->vertex_shader);
    gl->DeleteBuffers(1, &geometry->index_buffer);
    gl->DeleteBuffers(1, &geometry->vertex_buffer);
}

typedef struct {
    float x0;
    float y0;
    float s0;
    float t0;
    float x1;
    float y1;
    float s1;
    float t1;
} AvenGLTextQuad;

static inline float aven_gl_text_geometry_push(
    AvenGLTextGeometry *geometry,
    Vec2 pos,
    float pixel_size,
    Vec4 color,
    AvenStr text
) {
    void stbtt_GetPackedQuad(
        const AvenGLTextChar *chardata,
        int pw,
        int ph,
        int char_index,
        float *xpos,
        float *ypos,
        AvenGLTextQuad *q,
        int align_to_integer
    );

    float cur_x = 0.0f;
    float cur_y = 0.0f;

    for (size_t i = 0; i < text.len; i += 1) {
        int pc_index = slice_get(text, i) - 32;

        AvenGLTextQuad q;
        stbtt_GetPackedQuad(
            geometry->font.packed_chars,
            geometry->font.texture_width,
            geometry->font.texture_height,
            pc_index,
            &cur_x,
            &cur_y,
            &q,
            0
        );

        size_t start_index = geometry->vertices.len;

        list_push(geometry->vertices) = (AvenGLTextVertex){
            .pos = { q.x0 * pixel_size + pos[0], q.y0 * pixel_size + pos[1] },
            .tex = { q.s0, q.t0, },
            .color = { color[0], color[1], color[2], color[3] },
        };
        list_push(geometry->vertices) = (AvenGLTextVertex){
            .pos = { q.x1 * pixel_size + pos[0], q.y0 * pixel_size + pos[1] },
            .tex = { q.s1, q.t0, },
            .color = { color[0], color[1], color[2], color[3] },
        };
        list_push(geometry->vertices) = (AvenGLTextVertex){
            .pos = { q.x1 * pixel_size + pos[0], q.y1 * pixel_size + pos[1] },
            .tex = { q.s1, q.t1, },
            .color = { color[0], color[1], color[2], color[3] },
        };
        list_push(geometry->vertices) = (AvenGLTextVertex){
            .pos = { q.x0 * pixel_size + pos[0], q.y1 * pixel_size + pos[1] },
            .tex = { q.s0, q.t1, },
            .color = { color[0], color[1], color[2], color[3] },
        };

        list_push(geometry->indices) = (GLushort)start_index + 0;
        list_push(geometry->indices) = (GLushort)start_index + 1;
        list_push(geometry->indices) = (GLushort)start_index + 2;
        list_push(geometry->indices) = (GLushort)start_index + 0;
        list_push(geometry->indices) = (GLushort)start_index + 2;
        list_push(geometry->indices) = (GLushort)start_index + 3;
    }

    return cur_x;
}

static inline void aven_gl_text_geometry_clear(AvenGLTextGeometry *geometry) {
    geometry->vertices.len = 0;
    geometry->indices.len = 0;
}

static inline float aven_gl_text_geometry_width(
    AvenGLTextFont *font,
    float pixel_size,
    AvenStr text
) {
    void stbtt_GetPackedQuad(
        const AvenGLTextChar *chardata,
        int pw,
        int ph,
        int char_index,
        float *xpos,
        float *ypos,
        AvenGLTextQuad *q,
        int align_to_integer
    );

    float cur_x = 0.0f;
    float cur_y = 0.0f;

    for (size_t i = 0; i < text.len; i += 1) {
        int pc_index = slice_get(text, i) - 32;

        AvenGLTextQuad q;
        stbtt_GetPackedQuad(
            font->packed_chars,
            font->texture_width,
            font->texture_height,
            pc_index,
            &cur_x,
            &cur_y,
            &q,
            0
        );
    }

    return cur_x * pixel_size;
}

static inline void aven_gl_text_geometry_draw(
    AvenGLTextGeometry *geometry,
    AvenGL *gl,
    Mat2 cam_trans,
    Vec2 cam_pos
) {
    gl->BindBuffer(GL_ARRAY_BUFFER, geometry->vertex_buffer);
    assert(gl->GetError() == 0);
    gl->BufferSubData(
        GL_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(sizeof(*geometry->vertices.ptr) * geometry->vertices.len),
        geometry->vertices.ptr
    );
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->index_buffer);
    assert(gl->GetError() == 0);
    gl->BufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(sizeof(*geometry->indices.ptr) * geometry->indices.len),
        geometry->indices.ptr
    );
    assert(gl->GetError() == 0);

    gl->UseProgram(geometry->program);
    assert(gl->GetError() == 0);

    gl->EnableVertexAttribArray(geometry->vpos_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(geometry->vtex_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(geometry->vcolor_location);
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        geometry->vpos_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGLTextVertex),
        (void*)offsetof(AvenGLTextVertex, pos)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        geometry->vtex_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGLTextVertex),
        (void*)offsetof(AvenGLTextVertex, tex)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        geometry->vcolor_location,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGLTextVertex),
        (void*)offsetof(AvenGLTextVertex, color)
    );
    assert(gl->GetError() == 0);
    gl->Enable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, geometry->font.texture_id);
    assert(gl->GetError() == 0);

    gl->UniformMatrix2fv(
        (GLint)geometry->utrans_location,
        1,
        GL_FALSE,
        (GLfloat*)cam_trans
    );
    assert(gl->GetError() == 0);
    gl->Uniform2fv(
        (GLint)geometry->upos_location,
        1,
        (GLfloat*)cam_pos
    );
    assert(gl->GetError() == 0);

    gl->DrawElements(
        GL_TRIANGLES,
        (GLsizei)geometry->indices.len,
        GL_UNSIGNED_SHORT,
        0
    );
    assert(gl->GetError() == 0);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);

    gl->Disable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(geometry->vcolor_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(geometry->vtex_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(geometry->vpos_location);
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

#endif // AVEN_GL_TEXT_H
