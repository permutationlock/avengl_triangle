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
    AvenGlShapeCtx ctx;
    AvenGlShapeGeometry geometry;
    AvenGlShapeBuffer buffer;
} GameShapes;

typedef struct {
    AvenGlTextFont font;
    AvenGlTextCtx ctx;
    AvenGlTextGeometry geometry;
    AvenGlTextBuffer buffer;
} GameText;

typedef struct {
    GameText text;
    GameShapes shapes;
    AvenArena init_arena;
    AvenArena arena;
    AvenTimeInst last_update;
    float angle;
} GameCtx;

GameCtx game_init(AvenGl *gl, AvenArena *arena);
void game_deinit(GameCtx *ctx, AvenGl *gl);
int game_reload(GameCtx *ctx, AvenGl *gl);
int game_update(GameCtx *ctx, AvenGl *gl, int width, int height);
 
typedef GameCtx (*GameInitFn)(AvenGl *gl, AvenArena *arena);
typedef int (*GameReloadFn)(GameCtx *ctx, AvenGl *gl);
typedef int (*GameUpdateFn)(GameCtx *ctx, AvenGl *gl, int width, int height);
typedef void (*GameDeinitFn)(GameCtx *ctx, AvenGl *gl);

typedef struct {
    GameInitFn init;
    GameReloadFn reload;
    GameUpdateFn update;
    GameDeinitFn deinit;
} GameTable;

#endif // GAME_H

