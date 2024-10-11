#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>

#include "config.h"

#define AVEN_IMPLEMENTATION

#include "deps/libaven/include/aven.h"
#include "deps/libaven/include/aven/arena.h"
#include "deps/libaven/include/aven/arg.h"
#include "deps/libaven/include/aven/build.h"
#include "deps/libaven/include/aven/build/common.h"
#include "deps/libaven/include/aven/path.h"
#include "deps/libaven/include/aven/proc.h"
#include "deps/libaven/include/aven/watch.h"

#include "deps/libaven/build.h"
#include "deps/libavengl/build.h"

#include "build.h"

static AvenArg custom_arg_data[] = {
    {
        .name = "watch",
        .description = "Automatically run and reload code on changes to src",
        .type = AVEN_ARG_TYPE_BOOL,
    },
};

typedef enum {
    REBUILD_STATE_NONE = 0,
    REBUILD_STATE_EXE,
    REBUILD_STATE_GAME,
} RebuildState;
 
#define ARENA_SIZE (4096 * 2000)

int main(int argc, char **argv) {
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    // Construct and parse build arguments

    AvenArgSlice custom_args = slice_array(custom_arg_data);
    AvenArgSlice triangle_args = avengl_triangle_build_args(&arena);
    AvenArgSlice common_args = aven_build_common_args();

    AvenArgSlice args = {
        .len = common_args.len + triangle_args.len + custom_args.len,
    };
    args.ptr = aven_arena_create_array(AvenArg, &arena, args.len);

    {
        size_t i = 0;
        for (size_t j = 0; j < custom_args.len; j += 1) {
            slice_get(args, i) = slice_get(custom_args, j);
            i += 1;
        }
        for (size_t j = 0; j < triangle_args.len; j += 1) {
            slice_get(args, i) = slice_get(triangle_args, j);
            i += 1;
        }
        for (size_t j = 0; j < common_args.len; j += 1) {
            slice_get(args, i) = slice_get(common_args, j);
            i += 1;
        }
    }

    int error = aven_arg_parse(
        args,
        argv,
        argc,
        aven_build_common_overview().ptr,
        aven_build_common_usage().ptr
    );
    if (error != 0) {
        if (error != AVEN_ARG_ERROR_HELP) {
            fprintf(stderr, "ARG PARSE ERROR: %d\n", error);
            return error;
        }
        return 0;
    }

    AvenBuildCommonOpts opts = aven_build_common_opts(args, &arena);
    AvenGlTriangleBuildOpts triangle_opts = avengl_triangle_build_opts(
        args,
        &arena
    );
    bool opt_watch = aven_arg_get_bool(args, "watch");

    // Build setup

    AvenStr root_dir = aven_str(".");
    AvenStr work_dir = aven_str("build_work");
    AvenStr out_dir = aven_str("build_out");

    AvenStr libaven_path = aven_path(
        &arena,
        "deps",
        "libaven",
        NULL
    );

    AvenBuildStep work_dir_step = aven_build_step_mkdir(work_dir);
    AvenBuildStep out_dir_step = aven_build_step_mkdir(out_dir);

    // Single translation unit build steps

    AvenBuildStep triangle_step = avengl_triangle_build_step_exe(
        &opts,
        &triangle_opts,
        libaven_path,
        root_dir,
        &work_dir_step,
        &out_dir_step,
        &arena
    );

    AvenBuildStep root_step = aven_build_step_root();
    aven_build_step_add_dep(&root_step, &triangle_step, &arena);

    // Dynamic hot-reload build steps
    
    AvenBuildStep hot_dll_step = avengl_triangle_build_step_hot_dll(
        &opts,
        &triangle_opts,
        libaven_path,
        root_dir,
        &work_dir_step,
        &out_dir_step,
        &arena
    );
    AvenBuildStep watch_dir_step = aven_build_common_step_subdir(
        &out_dir_step,
        aven_str("watch"),
        &arena
    );
    AvenBuildStep hot_signal_step = aven_build_step_trunc(
        aven_path(&arena, watch_dir_step.out_path.value.ptr, "lock", NULL)
    );
    aven_build_step_add_dep(&hot_signal_step, &watch_dir_step, &arena);
    aven_build_step_add_dep(&hot_signal_step, &hot_dll_step, &arena);

    AvenBuildStep hot_exe_step = avengl_triangle_build_step_hot_exe(
        &opts,
        &triangle_opts,
        libaven_path,
        root_dir,
        &work_dir_step,
        &out_dir_step,
        &arena
    );
    AvenBuildStep hot_exe_root_step = aven_build_step_root();
    aven_build_step_add_dep(&hot_exe_root_step, &hot_signal_step, &arena);
    aven_build_step_add_dep(&hot_exe_root_step, &hot_exe_step, &arena);

    AvenBuildStep hot_game_root_step = aven_build_step_root();
    aven_build_step_add_dep(&hot_game_root_step, &hot_signal_step, &arena);

    // Run build steps according to command line options

    if (opts.clean) {
        aven_build_step_clean(&root_step);
        aven_build_step_clean(&hot_game_root_step);
    } else if (opt_watch) {
        AvenWatchHandle src_handle = aven_watch_init(
            avengl_triangle_build_src_path(root_dir, &arena)
        );
        AvenWatchHandle game_handle = aven_watch_init(
            avengl_triangle_build_src_game_path(root_dir, &arena)
        );
        AvenWatchHandle handle_data[] = { src_handle, game_handle };
        AvenWatchHandleSlice handles = slice_array(handle_data);

        Optional(AvenProcId) exe_pid = { .valid = false };

        RebuildState rebuild_state = REBUILD_STATE_EXE;
        for (;;) {
            switch (rebuild_state) {
                case REBUILD_STATE_EXE:
                    if (exe_pid.valid) {
                        aven_proc_kill(exe_pid.value);
                        exe_pid.valid = false;
                    }
                    aven_build_step_reset(&hot_exe_root_step);
                    error = aven_build_step_run(&hot_exe_root_step, arena);
                    if (error != 0) {
                        fprintf(stderr, "BUILD FAILED: %d\n", error);
                    }
                    break;
                case REBUILD_STATE_GAME:
                    aven_build_step_reset(&hot_game_root_step);
                    error = aven_build_step_run(&hot_game_root_step, arena);
                    if (error != 0) {
                        fprintf(stderr, "BUILD FAILED: %d\n", error);
                    }
                    break;
                default:
                    break;
            }
            rebuild_state = REBUILD_STATE_NONE;

            if (exe_pid.valid) {
                AvenProcWaitResult result = aven_proc_check(
                    exe_pid.value
                );
                if (result.error == 0) {
                    if (result.payload == 0) {
                        printf("APPLICATION EXITED CLEANLY\n");
                    } else {
                        printf("APPLICATION FAILED: %d\n", result.payload);
                    }
                    exe_pid.valid = false;
                } else if (result.error != AVEN_PROC_WAIT_ERROR_TIMEOUT) {
                    aven_proc_kill(exe_pid.value);
                    exe_pid.valid = false;
                }
            }

            if (!exe_pid.valid) {
                printf("RUNNING:\n");

                AvenStr cmd_parts[] = { hot_exe_step.out_path.value };
                AvenStrSlice cmd = slice_array(cmd_parts);

                AvenProcIdResult result = aven_proc_cmd(cmd, arena);
                if (result.error != 0) {
                    fprintf(stderr, "RUN FAILED: %d\n", result.error);
                } else {
                    exe_pid.valid = true;
                    exe_pid.value = result.payload;
                }
            }

            AvenWatchResult result = aven_watch_check_multiple(handles, -1);
            if (result.error != 0) {
                fprintf(stderr, "WATCH CHECK FAILED: %d\n", result.error);
                return 1;
            }

            for (size_t i = 0; i < handles.len; i += 1) {
                if ((result.payload & (((uint32_t)1) << i)) == 0) {
                    continue;
                }
                if (slice_get(handles, i) == src_handle) {
                    rebuild_state = REBUILD_STATE_EXE;
                    break;
                }
                if (slice_get(handles, i) == game_handle) {
                    rebuild_state = REBUILD_STATE_GAME;
                }
            }

            // Ignore extraneous source modifications some editors produce
            while (result.payload != 0) {
                result = aven_watch_check_multiple(handles, 100);
                if (result.error != 0) {
                    fprintf(stderr, "WATCH CHECK FAILED: %d\n", result.error);
                    return 1;
                }
            }
        }
    } else {
        error = aven_build_step_run(&root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BUILD FAILED: %d\n", error);
            return 1;
        }
    }

    return 0;
}

