#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#ifndef AVEN_IMPLEMENTATION
    #define AVEN_IMPLEMENTATION
#endif
#include <aven.h>
#include <aven/gl.h>
#include <aven/glm.h>
#include <aven/gl/text.h>
#include <aven/time.h>

#include "../game.h"
#include "font.h"

#define ROTATION_VELOCITY (2.0f * AVEN_GLM_PI_F / 100.0f)
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

static void game_load(GameCtx *ctx, AvenGL *gl, AvenArena *arena) {
    ctx->text_geometry = aven_gl_text_geometry_init(gl, ctx->font, 128, arena);
}

static void game_unload(GameCtx *ctx, AvenGL *gl) {
    aven_gl_text_geometry_deinit(&ctx->text_geometry, gl);
}

GameCtx game_init(AvenGL *gl, AvenArena *arena) {
    GameCtx ctx = { 0 };

    AvenArena temp_arena = *arena;
    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);

    ctx.font = aven_gl_text_font_init(gl, font_bytes, 64.0f, temp_arena);
    game_load(&ctx, gl, arena);

    ctx.last_update = aven_time_now();
    ctx.angle = 0.0f;

    return ctx;
}

void game_deinit(GameCtx *ctx, AvenGL *gl) {
    aven_gl_text_font_deinit(&ctx->font, gl);
    game_unload(ctx, gl);
    *ctx = (GameCtx){ 0 };
}

int game_reload(GameCtx *ctx, AvenGL *gl, AvenArena *arena) {
    game_unload(ctx, gl);
    game_load(ctx, gl, arena);

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

    aven_gl_text_geometry_clear(&ctx->text_geometry);
    aven_gl_text_geometry_push(
        &ctx->text_geometry,
        0.0f,
        0.0f,
        pixel_size,
        (Vec4){ 1.0f, 1.0f, 1.0f, 1.0f },
        aven_str("Say what?!")
    );

    aven_gl_text_geometry_draw(&ctx->text_geometry, gl, mvp);

    return 0;
}

