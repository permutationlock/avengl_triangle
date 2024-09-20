#ifndef GAME_H
#define GAME_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/gl/text.h>
#include <aven/time.h>

#define GAME_ARENA_SIZE (4096 * 2000)

typedef struct {
    AvenGLShapeCtx ctx;
    AvenGLShapeGeometry geometry;
    AvenGLShapeBuffer buffer;
} GameShapes;

typedef struct {
    AvenGLTextFont font;
    AvenGLTextCtx ctx;
    AvenGLTextGeometry geometry;
    AvenGLTextBuffer buffer;
} GameText;

typedef struct {
    GameText text;
    GameShapes shapes;
    AvenArena init_arena;
    AvenArena arena;
    AvenTimeInst last_update;
    float angle;
} GameCtx;

GameCtx game_init(AvenGL *gl, AvenArena *arena);
void game_deinit(GameCtx *ctx, AvenGL *gl);
int game_reload(GameCtx *ctx, AvenGL *gl);
int game_update(GameCtx *ctx, AvenGL *gl, int width, int height);
 
typedef GameCtx (*GameInitFn)(AvenGL *gl, AvenArena *arena);
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

