#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#ifndef AVEN_IMPLEMENTATION
    #define AVEN_IMPLEMENTATION
#endif
#include <aven.h>
#include <aven/arena.h>
#include <aven/gl.h>
#include <aven/glm.h>
#include <aven/gl/shape.h>
#include <aven/gl/text.h>
#include <aven/str.h>
#include <aven/time.h>

#include "../game.h"
#include "font.h"

#define ROTATION_VELOCITY (2.0f * AVEN_GLM_PI_F / 10.0f)

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

static void game_load(GameCtx *ctx, AvenGl *gl) {
    ctx->arena = ctx->init_arena;

    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);
    ctx->text.font = aven_gl_text_font_init(
        gl,
        font_bytes,
        32.0f,
        ctx->init_arena
    );

    ctx->text.ctx = aven_gl_text_ctx_init(gl);
    ctx->text.geometry = aven_gl_text_geometry_init(
        128,
        &ctx->arena
    );
    ctx->text.buffer = aven_gl_text_buffer_init(
        gl,
        &ctx->text.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx->shapes.ctx = aven_gl_shape_ctx_init(gl);
    ctx->shapes.geometry = aven_gl_shape_geometry_init(
        128,
        192,
        &ctx->arena
    );
    ctx->shapes.buffer = aven_gl_shape_buffer_init(
        gl,
        &ctx->shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );
}

static void game_unload(GameCtx *ctx, AvenGl *gl) {
    aven_gl_shape_buffer_deinit(gl, &ctx->shapes.buffer);
    aven_gl_shape_ctx_deinit(gl, &ctx->shapes.ctx);
    ctx->shapes = (GameShapes){ 0 };

    aven_gl_text_buffer_deinit(gl, &ctx->text.buffer);
    aven_gl_text_ctx_deinit(gl, &ctx->text.ctx);
    ctx->text = (GameText){ 0 };
}

GameCtx game_init(AvenGl *gl, AvenArena *arena) {
    GameCtx ctx = { 0 };

    ctx.init_arena = aven_arena_init(
        aven_arena_alloc(
            arena,
            GAME_ARENA_SIZE,
            AVEN_ARENA_BIGGEST_ALIGNMENT,
            1
        ),
        GAME_ARENA_SIZE
    );

    game_load(&ctx, gl);

    ctx.last_update = aven_time_now();
    ctx.angle = 0.0f;

    return ctx;
}

void game_deinit(GameCtx *ctx, AvenGl *gl) {
    aven_gl_text_font_deinit(gl, &ctx->text.font);
    game_unload(ctx, gl);
    *ctx = (GameCtx){ 0 };
}

int game_reload(GameCtx *ctx, AvenGl *gl) {
    game_unload(ctx, gl);
    game_load(ctx, gl);

    return 0;
}

int game_update(
    GameCtx *ctx,
    AvenGl *gl,
    int width,
    int height,
    AvenArena arena
) {
    AvenTimeInst now = aven_time_now();
    int64_t elapsed = aven_time_since(now, ctx->last_update);
    ctx->last_update = now;

    float elapsed_sec = (float)elapsed / (float)AVEN_TIME_NSEC_PER_SEC;

    ctx->angle += ROTATION_VELOCITY * elapsed_sec;

    float screen_ratio = (float)width / (float)height;
    float pixel_size = 2.0f / (float)height;

    gl->Viewport(0, 0, width, height);
    assert(gl->GetError() == 0);
    gl->ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    assert(gl->GetError() == 0);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    assert(gl->GetError() == 0);

    Aff2 cam_trans;
    aff2_camera_position(
        cam_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ screen_ratio, 1.0f },
        ctx->angle
    );

    aven_gl_text_geometry_clear(&ctx->text.geometry);
    aven_gl_shape_geometry_clear(&ctx->shapes.geometry);

    Aff2 square_trans;
    aff2_position(
        square_trans,
        (Vec2){ -0.5f, -0.5f },
        (Vec2){ 0.4f, 0.4f },
        0.0f
    );
    aven_gl_shape_geometry_push_square(
        &ctx->shapes.geometry,
        square_trans,
        0.8f,
        (Vec4){ 0.75f, 0.75f, 0.0f, 1.0f }
    );

    Aff2 triangle_trans;
    aff2_position(
        triangle_trans,
        (Vec2){ 0.5f, 0.5f },
        (Vec2){ 0.25f, 0.25f },
        0.0f
    );
    aven_gl_shape_geometry_push_triangle_isoceles(
        &ctx->shapes.geometry,
        triangle_trans,
        0.5f,
        (Vec4){ 0.15f, 0.45f, 0.75f, 1.0f }
    );

    AvenGlTextLine hello_line = aven_gl_text_line(
        &ctx->text.font,
        aven_str("Hello, World!"),
        &arena
    );
    Aff2 hello_trans;
    aff2_identity(hello_trans);
    aven_gl_text_geometry_push_line(
        &ctx->text.geometry,
        &hello_line,
        hello_trans,
        pixel_size,
        (Vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
    );

    aven_gl_shape_buffer_update(gl, &ctx->shapes.buffer, &ctx->shapes.geometry);
    aven_gl_text_buffer_update(gl, &ctx->text.buffer, &ctx->text.geometry);

    aven_gl_shape_draw(
        gl,
        &ctx->shapes.ctx,
        &ctx->shapes.buffer,
        cam_trans
    );
    aven_gl_text_geometry_draw(
        gl,
        &ctx->text.ctx,
        &ctx->text.buffer,
        &ctx->text.font,
        cam_trans
    );

    return 0;
}

