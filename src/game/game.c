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

static void game_load(GameCtx *ctx, AvenGL *gl) {
    ctx->arena = ctx->init_arena;
    ctx->text_geometry = aven_gl_text_geometry_init(
        gl,
        ctx->font,
        128,
        &ctx->arena
    );
    ctx->shape_color_geometry = aven_gl_shape_color_geometry_init(
        gl,
        128,
        192,
        &ctx->arena
    );
}

static void game_unload(GameCtx *ctx, AvenGL *gl) {
    aven_gl_shape_color_geometry_deinit(&ctx->shape_color_geometry, gl);
    aven_gl_text_geometry_deinit(&ctx->text_geometry, gl);

    ctx->shape_color_geometry = (AvenGLShapeColorGeometry){ 0 };
    ctx->text_geometry = (AvenGLTextGeometry){ 0 };
}

GameCtx game_init(AvenGL *gl, AvenArena *arena) {
    GameCtx ctx = { 0 };

    ctx.init_arena = aven_arena_init(
        aven_arena_alloc(arena, GAME_ARENA_SIZE, __BIGGEST_ALIGNMENT__),
        GAME_ARENA_SIZE
    );

    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);
    ctx.font = aven_gl_text_font_init(gl, font_bytes, 64.0f, ctx.init_arena);

    game_load(&ctx, gl);

    ctx.last_update = aven_time_now();
    ctx.angle = 0.0f;

    return ctx;
}

void game_deinit(GameCtx *ctx, AvenGL *gl) {
    aven_gl_text_font_deinit(&ctx->font, gl);
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

    Mat2 mi;
    mat2_identity(mi);
    Mat2 mr;
    mat2_rotate(mr, mi, ctx->angle);
    Mat2 mp;
    mat2_ortho(mp, -ratio, ratio, 1.0f, -1.0f);
    Mat2 mvp;
    mat2_mul_mat2(mvp, mp, mr);

    Vec2 pos = { 0.0f, 0.0f };

    aven_gl_text_geometry_clear(&ctx->text_geometry);
    aven_gl_shape_color_geometry_clear(&ctx->shape_color_geometry);

    aven_gl_shape_color_geometry_push_rectangle(
        &ctx->shape_color_geometry,
        (Vec2){ -0.5f, -0.5f },
        (Vec2){ 0.4f, 0.4f },
        ctx->angle,
        0.25f,
        (Vec4){ 0.75f, 0.75f, 0.0f, 1.0f }
    );

    aven_gl_shape_color_geometry_push_triangle_equilateral(
        &ctx->shape_color_geometry,
        (Vec2){ 0.5f, 0.5f },
        0.25f,
        ctx->angle,
        0.25f,
        (Vec4){ 0.15f, 0.45f, 0.75f, 1.0f }
    );

    aven_gl_text_geometry_push(
        &ctx->text_geometry,
        (Vec2){ 0.0f, 0.0f },
        pixel_size,
        (Vec4){ 1.0f, 1.0f, 1.0f, 1.0f },
        aven_str("Hello, World!")
    );

    aven_gl_shape_color_geometry_draw(&ctx->shape_color_geometry, gl, mvp, pos);
    aven_gl_text_geometry_draw(&ctx->text_geometry, gl, mvp, pos);

    return 0;
}

