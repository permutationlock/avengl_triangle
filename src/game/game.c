#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#ifndef AVEN_IMPLEMENTATION
    #define AVEN_IMPLEMENTATION
#endif
#include <aven.h>
#include <aven/gl.h>
#include <aven/glm.h>
#include <aven/time.h>

#include "../game.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <stdio.h>

#define ROTATION_VELOCITY (2.0f * AVEN_GLM_PI_F / 10.0f)
#define MAX_VERTICES 256
#define MAX_INDICES 512

#if !defined(HOT_RELOAD)
static
#elif defined(_MSC_VER)
__declspec(dllexport)
#endif
const GameTable game_table = {
    .init = game_init,
    .reload = game_reload,
    .update = game_update,
    .deinit = game_deinit,
};

static const uint32_t texture[2 * 2] = {
    0xffdddddd, 0xff222222,
    0xff222222, 0xffdddddd,
};

typedef struct {
    Vec2 pos;
    Vec2 tex;
} Vertex;

typedef List(Vertex) VertexList;
typedef List(GLushort) IndexList;

static const char* vertex_shader_text =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform mat4 uMVP;\n"
    "attribute vec2 vTex;\n"
    "attribute vec2 vPos;\n"
    "varying vec2 texCoord;\n"
    "void main() {\n"
    "    gl_Position = uMVP * vec4(vPos, 0.0, 1.0);\n"
    "    texCoord = vTex;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform sampler2D texSampler;\n"
    "varying vec2 texCoord;\n"
    "void main() {\n"
    "    vec4 temp = texture2D(texSampler, texCoord);\n"
    "    gl_FragColor = vec4(1.0, 1.0, 1.0, temp.w);\n"
    "}\n";

#define ROWS ((20 + 2) * 2 * 4)
#define COLS ((20 + 2) * 2 * 9)
static unsigned char ttf_buffer[1<<20];
static unsigned char temp_bitmap[ROWS * COLS];

static stbtt_packedchar cdata[96]; // ASCII 32..126 is 95 glyphs

static void game_load(GameCtx *ctx, AvenGL *gl) {
    gl->GenBuffers(1, &ctx->gl.vertex_buffer);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, ctx->gl.vertex_buffer);
    assert(gl->GetError() == 0);
    {
        Vertex blank_vertices[MAX_VERTICES] = { 0 };
        gl->BufferData(
            GL_ARRAY_BUFFER,
            MAX_VERTICES * sizeof(Vertex),
            blank_vertices,
            GL_DYNAMIC_DRAW
        );
        assert(gl->GetError() == 0);
    }

    ctx->gl.vertex_shader = gl->CreateShader(GL_VERTEX_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(ctx->gl.vertex_shader, 1, &vertex_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(ctx->gl.vertex_shader);
    assert(gl->GetError() == 0);

    ctx->gl.fragment_shader = gl->CreateShader(GL_FRAGMENT_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(ctx->gl.fragment_shader, 1, &fragment_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(ctx->gl.fragment_shader);
    assert(gl->GetError() == 0);

    ctx->gl.program = gl->CreateProgram();
    assert(gl->GetError() == 0);
    gl->AttachShader(ctx->gl.program, ctx->gl.vertex_shader);
    assert(gl->GetError() == 0);
    gl->AttachShader(ctx->gl.program, ctx->gl.fragment_shader);
    assert(gl->GetError() == 0);
    gl->LinkProgram(ctx->gl.program);
    assert(gl->GetError() == 0);

    ctx->gl.mvp_location = (GLuint)gl->GetUniformLocation(
        ctx->gl.program,
        "uMVP"
    );
    assert(gl->GetError() == 0);
    ctx->gl.vpos_location = (GLuint)gl->GetAttribLocation(
        ctx->gl.program,
        "vPos"
    );
    assert(gl->GetError() == 0);
    ctx->gl.vcol_location = (GLuint)gl->GetAttribLocation(
        ctx->gl.program,
        "vTex"
    );
    assert(gl->GetError() == 0);

    gl->EnableVertexAttribArray(ctx->gl.vpos_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(ctx->gl.vcol_location);
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->gl.vpos_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, pos)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->gl.vcol_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, tex)
    );
    assert(gl->GetError() == 0);

    gl->GenTextures(1, &ctx->gl.tex_id);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, ctx->gl.tex_id);
    assert(gl->GetError() == 0);

    fread(
        ttf_buffer,
        1,
        1<<20,
        fopen("/usr/share/fonts/TTF/FiraCode-Regular.ttf", "rb")
    );
    stbtt_pack_context pc;
    int success = stbtt_PackBegin(
        &pc,
        temp_bitmap,
        ROWS,
        COLS,
        0,
        1,
        NULL
    );
    assert(success != 0);
    stbtt_PackSetOversampling(&pc, 2, 2);
    success = stbtt_PackFontRange(
        &pc,
        ttf_buffer,
        0,
        20.0f,
        32,
        96,
        cdata
    );
    assert(success != 0);
    stbtt_PackEnd(&pc);
    gl->TexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA, 
        ROWS,
        COLS,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        temp_bitmap
    );
    //gl->TexImage2D(
    //    GL_TEXTURE_2D,
    //    0,
    //    GL_RGBA,
    //    2,
    //    2,
    //    0,
    //    GL_RGBA,
    //    GL_UNSIGNED_BYTE,
    //    texture
    //);
    //gl->GenerateMipmap(GL_TEXTURE_2D);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);
}

static void game_unload(GameCtx *ctx, AvenGL *gl) {
    gl->DeleteTextures(1, &ctx->gl.tex_id);
    gl->DeleteProgram(ctx->gl.program);
    gl->DeleteShader(ctx->gl.fragment_shader);
    gl->DeleteShader(ctx->gl.vertex_shader);
    gl->DeleteBuffers(1, &ctx->gl.vertex_buffer);
}

GameCtx game_init(AvenGL *gl) {
    GameCtx ctx = { 0 };

    game_load(&ctx, gl);

    ctx.last_update = aven_time_now();
    ctx.angle = 0.0f;

    return ctx;
}

void game_deinit(GameCtx *ctx, AvenGL *gl) {
    game_unload(ctx, gl);
    *ctx = (GameCtx){ 0 };
}

int game_reload(GameCtx *ctx, AvenGL *gl) {
    game_unload(ctx, gl);
    game_load(ctx, gl);

    return 0;
}

int game_update(
    GameCtx *ctx,
    AvenGL *gl,
    int width,
    int height
) {
    AvenTimeInst now = aven_time_now();
    int64_t elapsed = aven_time_since(now, ctx->last_update);
    ctx->last_update = now;

    float elapsed_sec = (float)elapsed / (float)AVEN_TIME_NSEC_PER_SEC;

    ctx->angle += ROTATION_VELOCITY * elapsed_sec;

    float ratio = (float)width / (float)height;
    float pixel_size = 2.0f / (float)height;

    gl->Viewport(0, 0, width, height);
    assert(gl->GetError() == 0);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    assert(gl->GetError() == 0);

    Mat4 id_m, rot_m, per_m, mvp;
    mat4_identity(id_m);
    mat4_rotate_z(rot_m, id_m, ctx->angle);
    mat4_ortho(per_m, ratio, -ratio, -1.0f, 1.0f, 1.0f, -1.0f);
    mat4_mul_mat4(mvp, per_m, rot_m);

    float x = 0.0f;
    float y = 0.0f;
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(cdata, ROWS, COLS, '6' - 32, &x, &y, &q, 0);
    q.x0 *= pixel_size;
    q.x1 *= pixel_size;
    q.y0 *= pixel_size;
    q.y1 *= pixel_size;

    Vertex vertex_data[MAX_VERTICES];
    VertexList vertices = list_array(vertex_data);
    list_push(vertices) = (Vertex){ { q.x0, q.y0, }, { q.s0, q.t0, } };
    list_push(vertices) = (Vertex){ { q.x1, q.y0, }, { q.s1, q.t0, } };
    list_push(vertices) = (Vertex){ { q.x1, q.y1, }, { q.s1, q.t1, } };
    list_push(vertices) = (Vertex){ { q.x0, q.y1, }, { q.s0, q.t1, } };

    gl->BindBuffer(GL_ARRAY_BUFFER, ctx->gl.vertex_buffer);
    assert(gl->GetError() == 0);
    ByteSlice vertex_bytes = list_as_bytes(vertices);
    gl->BufferSubData(
        GL_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(vertex_bytes.len),
        vertex_bytes.ptr
    );
    assert(gl->GetError() == 0);

    GLushort index_data[MAX_INDICES];
    IndexList indices = list_array(index_data);
    list_push(indices) = 0;
    list_push(indices) = 1;
    list_push(indices) = 2;
    list_push(indices) = 0;
    list_push(indices) = 2;
    list_push(indices) = 3;

    gl->UseProgram(ctx->gl.program);
    assert(gl->GetError() == 0);
    gl->UniformMatrix4fv(
        (GLint)ctx->gl.mvp_location,
        1,
        GL_FALSE,
        (GLfloat*)mvp
    );
    assert(gl->GetError() == 0);

    gl->Enable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    assert(gl->GetError() == 0);
    //gl->Enable(GL_BLEND);
    gl->BindTexture(GL_TEXTURE_2D, ctx->gl.tex_id);
    assert(gl->GetError() == 0);
    gl->DrawElements(
        GL_TRIANGLES,
        (GLsizei)indices.len,
        GL_UNSIGNED_SHORT,
        indices.ptr
    );
    assert(gl->GetError() == 0);

    return 0;
}

