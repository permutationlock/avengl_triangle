#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/path.h>

#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>

#include "game.h"

#if defined(HOT_RELOAD)
    #include <aven/dl.h>
    #include <aven/watch.h>

    typedef struct {
        void *handle;
        GameTable vtable;
    } GameInfo;
    typedef Result(GameInfo) GameInfoResult;

    typedef enum {
        GAME_INFO_LOAD_ERROR_NONE = 0,
        GAME_INFO_LOAD_ERROR_OPEN,
        GAME_INFO_LOAD_ERROR_SYM,
    } GameInfoError;

    GameInfo game_info;

    static GameInfoResult game_info_load(AvenStr path) {
        GameInfo game_dll = { 0 };

        game_dll.handle = aven_dl_open(path);
        if (game_dll.handle == NULL) {
            return (GameInfoResult){ .error = GAME_INFO_LOAD_ERROR_OPEN };
        }

        GameTable *table = aven_dl_sym(game_dll.handle, aven_str("game_table"));
        if (table == NULL) {
            return (GameInfoResult){ .error = GAME_INFO_LOAD_ERROR_SYM };
        }

        game_dll.vtable = *table;

        return (GameInfoResult){ .payload = game_dll };
    }
    
    static void game_info_error_print(int error) {
        switch (error) {
            case GAME_INFO_LOAD_ERROR_OPEN:
                fprintf(stderr, "error opening dll\n");
                break;
            case GAME_INFO_LOAD_ERROR_SYM:
                fprintf(stderr, "error finding symbol in dll\n");
                break;
            default:
                fprintf(stderr, "unknown error\n");
                break;
        }
    }
#else // !defined(HOT_RELOAD)
    #include "game/game.c"

    typedef struct {
        GameTable vtable;
    } GameInfo;
    
    static const GameInfo game_info = { .vtable = game_table };
#endif // !defined(HOT_RELOAD)

static void error_callback(int error, const char* description) {
    (void)error;
    fprintf(stderr, "GLFW Error: %s\n", description);
}

static void key_callback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods
) {
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

GLFWwindow *window;
AvenGL gl;
GameCtx ctx;

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

#ifdef HOT_RELOAD
    #error "hot reloading dll incompatible with emcc"
#endif

void main_loop(void) {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    game_info.vtable.update(&ctx, &gl, width, height);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
#endif // defined(__EMSCRIPTEN__)

#define ARENA_SIZE (4096 * 2000)

#if defined(_MSC_VER)
int WinMain(void) {
#else // !defined(_MSC_VER)
int main(void) {
#endif // !defined(_MSC_VER)
    aven_fs_utf8_mode();

    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    int width = 480;
    int height = 480;

    glfwInit();
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    // glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(
        (int)width,
        (int)height,
        "AvenGL Test",
        NULL,
        NULL
    );
    if (window == NULL) {
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        window = glfwCreateWindow(
            (int)width,
            (int)height,
            "AvenGL Test",
            NULL,
            NULL
        );
        if (window == NULL) {
            printf("test failed: glfwCreateWindow\n");
            return 1;
        }
    }

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

#if defined(HOT_RELOAD)
    AvenStr exe_path = aven_str(".");
    {
        AvenPathResult info_result = aven_path_exe(&arena);
        if (info_result.error == 0) {
            exe_path = info_result.payload;
        }
    }
    AvenStr exe_dir_path = aven_path_rel_dir(exe_path, &arena);
    AvenStr game_dll_path = aven_path(
        &arena,
        exe_dir_path.ptr,
        "game",
#if defined(_WIN32)
        "game.dll",
#else // !defined(_WIN32)
        "game.so",
#endif // !defined(_WIN32)
        NULL
    );
    {
        GameInfoResult result = game_info_load(game_dll_path);
        if (result.error != 0) {
            game_info_error_print(result.error);
            return 1;
        }
        game_info = result.payload;
    }

    AvenStr watch_dir_path = aven_path(&arena, exe_dir_path.ptr, "watch", NULL);
    AvenWatchHandle game_watch_handle = aven_watch_init(watch_dir_path);
    if (game_watch_handle == AVEN_WATCH_HANDLE_INVALID) {
        fprintf(stderr, "FAILED TO WATCH: %s\n", watch_dir_path.ptr);
        return 1;
    }
    bool game_valid = true;
#else // !defined(HOT_RELOAD)
    (void)arena;
#endif // !defined(HOT_RELOAD)

    AvenArena game_arena = arena;
    gl = aven_gl_load(glfwGetProcAddress);
    ctx = game_info.vtable.init(&gl, &game_arena);

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
#else // !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(window)) {
#if defined(HOT_RELOAD)
        AvenWatchResult watch_result = aven_watch_check(game_watch_handle, 0);
        if (watch_result.error != 0) {
            fprintf(stderr, "FAILED TO WATCH: %s\n", watch_dir_path.ptr);
            return 1;
        }
        if (watch_result.payload != 0) {
            if (game_info.handle != NULL) {
                aven_dl_close(game_info.handle);
                game_info.handle = NULL;
            }
            GameInfoResult info_result = game_info_load(game_dll_path);
            if (info_result.error != 0) {
                game_info_error_print(info_result.error);
                game_valid = false;
            } else {
                printf("reloading\n");
                game_info = info_result.payload;
                game_arena = arena;
                game_info.vtable.reload(&ctx, &gl, &game_arena);
                game_valid = true;
            }
        }
        if (!game_valid) {
            continue;
        }
#endif // defined(HOT_RELOAD)
        glfwGetFramebufferSize(window, &width, &height);

        game_info.vtable.update(&ctx, &gl, width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
#endif // !defined(__EMSCRIPTEN__)

    return 0;
}
