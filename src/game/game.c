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

#define ROTATION_VELOCITY (2.0f * AVEN_GLM_PI_F / 4.0f)

#ifdef _MSC_VER
__declspec(dllexport)
#endif
GameTable game_table = {
    .init = game_init,
    .reload = game_reload,
    .update = game_update,
    .deinit = game_deinit,
};

typedef struct {
    Vec2 pos;
    Vec3 col;
} Vertex;

static const Vertex vertices[3] = {
    { { -0.6f, -0.4f }, { 1.0f, 0.0f, 0.0f } },
    { {  0.6f, -0.4f }, { 0.0f, 1.0f, 0.0f } },
    { {  0.0f,  0.6f }, { 0.0f, 0.0f, 1.0f } }
};

static const char* vertex_shader_text =
    "#version 100\n"
    "precision mediump float;\n"
    "uniform mat4 MVP;\n"
    "attribute vec3 vCol;\n"
    "attribute vec2 vPos;\n"
    "varying vec3 color;\n"
    "void main() {\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    color = vCol;\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec3 color;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(color, 1.0);\n"
    "}\n";

static void game_load(GameCtx *ctx, AvenGL *gl) {
    gl->GenBuffers(1, &ctx->gl.vertex_buffer);
    gl->BindBuffer(GL_ARRAY_BUFFER, ctx->gl.vertex_buffer);
    gl->BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    ctx->gl.vertex_shader = gl->CreateShader(GL_VERTEX_SHADER);
    gl->ShaderSource(ctx->gl.vertex_shader, 1, &vertex_shader_text, NULL);
    gl->CompileShader(ctx->gl.vertex_shader);

    ctx->gl.fragment_shader = gl->CreateShader(GL_FRAGMENT_SHADER);
    gl->ShaderSource(ctx->gl.fragment_shader, 1, &fragment_shader_text, NULL);
    gl->CompileShader(ctx->gl.fragment_shader);

    ctx->gl.program = gl->CreateProgram();
    gl->AttachShader(ctx->gl.program, ctx->gl.vertex_shader);
    gl->AttachShader(ctx->gl.program, ctx->gl.fragment_shader);
    gl->LinkProgram(ctx->gl.program);

    ctx->gl.mvp_location = (GLuint)gl->GetUniformLocation(ctx->gl.program, "MVP");
    ctx->gl.vpos_location = (GLuint)gl->GetAttribLocation(ctx->gl.program, "vPos");
    ctx->gl.vcol_location = (GLuint)gl->GetAttribLocation(ctx->gl.program, "vCol");

    gl->EnableVertexAttribArray(ctx->gl.vpos_location);
    gl->EnableVertexAttribArray(ctx->gl.vcol_location);
    gl->VertexAttribPointer(
        ctx->gl.vpos_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, pos)
    );
    gl->VertexAttribPointer(
        ctx->gl.vcol_location,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, col)
    );
}

static void game_unload(GameCtx *ctx, AvenGL *gl) {
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

    gl->Viewport(0, 0, width, height);
    gl->Clear(GL_COLOR_BUFFER_BIT);

    Mat4 id_m, rot_m, per_m, mvp;
    mat4_identity(id_m);
    mat4_rotate_z(rot_m, id_m, ctx->angle);
    mat4_ortho(per_m, -ratio, ratio, -1.0f, 1.0f, 1.0f, -1.0f);
    mat4_mul_mat4(mvp, per_m, rot_m);

    gl->UseProgram(ctx->gl.program);
    gl->UniformMatrix4fv(
        (GLint)ctx->gl.mvp_location,
        1,
        GL_FALSE,
        (GLfloat*)&mvp
    );
    gl->DrawArrays(GL_TRIANGLES, 0, 3);

    return 0;
}

