#ifndef GAME_H
#define GAME_H

#include <aven/gl.h>
#include <aven/time.h>

typedef struct {
    GLuint vertex_buffer;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint mvp_location;
    GLuint vpos_location;
    GLuint vcol_location;
} GameCtxGL;

typedef struct {
    GameCtxGL gl;
    AvenTimeInst last_update;
    float angle;
} GameCtx;

GameCtx game_init(AvenGL *gl);
void game_deinit(GameCtx *ctx, AvenGL *gl);
int game_reload(GameCtx *ctx, AvenGL *gl);
int game_update(GameCtx *ctx, AvenGL *gl, int width, int height);

typedef GameCtx (*GameInitFn)(AvenGL *gl);
typedef int (*GameReloadFn)(GameCtx *ctx, AvenGL *gl);
typedef int (*GameUpdateFn)(GameCtx *ctx, AvenGL *gl, int width, int height);
typedef void (*GameDeinitFn)(GameCtx *ctx, AvenGL *gl);

typedef struct {
    GameInitFn init;
    GameReloadFn reload;
    GameUpdateFn update;
    GameDeinitFn deinit;
} GameTable;

#endif // GAME_H

