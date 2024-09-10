#ifndef AVENGL_TRIANGLE_BUILD_H
#define AVENGL_TRIANGLE_BUILD_H

static inline AvenArgSlice avengl_triangle_build_args(AvenArena *arena) {
    AvenArg triangle_arg_data[] = {
        {
            .name = "-no-glfw",
            .description = "Don't build GLFW locally",
            .type = AVEN_ARG_TYPE_BOOL,
#if defined(BUILD_DEFAULT_NOGLFW)
            .value = {
                .type = AVEN_ARG_TYPE_BOOL,
                .data = { .arg_bool = BUILD_DEFAULT_NOGLFW },
            },
#endif
        },
        {
            .name = "-syslibs",
            .description = "System libraries to link",
            .type = AVEN_ARG_TYPE_STRING,
            .value = {
                .type = AVEN_ARG_TYPE_STRING,
#if defined(BUILD_DEFAULT_SYSLIBS)
                .data = { .arg_str = BUILD_DEFAULT_SYSLIBS },
#elif defined(_WIN32)
        #if defined(_MSC_VER) and !defined(__clang__)
                .data = {
                    .arg_str = "kernel32.lib user32.lib gdi32.lib shell32.lib"
                },
        #else
                .data = { .arg_str = "kernel32 user32 gdi32 shell32" },
        #endif
#else
                .data = { .arg_str = "m dl" },
#endif
            },
        },
        libaven_build_arg_windres_manifest,
    };
    AvenArgSlice triangle_args = {
        .ptr = triangle_arg_data,
        .len = countof(triangle_arg_data),
    };

    AvenArgSlice args = { .len = triangle_args.len + libavengl_build_args.len };
    args.ptr = aven_arena_create_array(AvenArg, arena, args.len); 
    size_t i = 0;
    for (size_t j = 0; j < libavengl_build_args.len; j += 1) {
        slice_get(args, i) = slice_get(libavengl_build_args, j);
        i += 1;
    }
    for (size_t j = 0; j < triangle_args.len; j += 1) {
        slice_get(args, i) = slice_get(triangle_args, j);
        i += 1;
    }

    return args;
}

typedef struct {
    LibAvenGLBuildOpts libavengl;
    AvenStrSlice syslibs;
    bool no_glfw;
    bool winutf8;
} AvenGLTriangleBuildOpts;

static inline AvenGLTriangleBuildOpts avengl_triangle_build_opts(
    AvenArgSlice args,
    AvenArena *arena
) {
    AvenGLTriangleBuildOpts opts = { 0 };
    opts.libavengl = libavengl_build_opts(args, arena);
    opts.syslibs = aven_str_split(
        aven_str_cstr(aven_arg_get_str(args, "-syslibs")),
        ' ',
        arena
    );
    opts.no_glfw = aven_arg_get_bool(args, "-no-glfw");
    opts.winutf8 = aven_arg_get_int(
        args,
        libaven_build_arg_windres_manifest.name
    ) != 0;

    return opts;
}

static inline AvenStr avengl_triangle_build_src_path(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "src", NULL);
}

static inline AvenStr avengl_triangle_build_src_game_path(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "src", "game", NULL);
}

static inline AvenBuildStep avengl_triangle_build_step_hot_dll(
    AvenBuildCommonOpts *opts,
    AvenGLTriangleBuildOpts *triangle_opts,
    AvenStr libaven_path,
    AvenStr root_path,
    AvenBuildStep *work_dir_step,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    (void)triangle_opts;

    AvenStr libavengl_root_path = aven_path(
        arena,
        root_path.ptr,
        "deps",
        "libavengl",
        NULL
    );

    AvenStr include_data[] = {
        libaven_build_include_path(libaven_path, arena),
        libavengl_build_include_path(libavengl_root_path, arena),
        libavengl_build_include_gles2(libavengl_root_path, arena),
    };
    AvenStrSlice includes = {
        .ptr = include_data,
        .len = countof(include_data)
    };
    AvenStrSlice macros = { 0 };

    AvenBuildStep *game_step = aven_arena_create(AvenBuildStep, arena);
    *game_step = aven_build_common_step_cc_ex(
        opts,
        includes,
        macros,
        aven_path(arena, root_path.ptr, "src", "game", "game.c", NULL),
        work_dir_step,
        arena
    );

    AvenBuildStep *obj_refs[] = { game_step };
    AvenBuildStepPtrSlice objs = {
        .ptr = obj_refs,
        .len = countof(obj_refs),
    };

    AvenBuildStep *game_dir_step = aven_arena_create(AvenBuildStep, arena);
    *game_dir_step = aven_build_common_step_subdir(
        out_dir_step,
        aven_str("game"),
        arena
    );

    return aven_build_common_step_ld_so(
        opts,
        objs,
        game_dir_step,
        aven_str("game"),
        arena
    );
}

static inline AvenBuildStep avengl_triangle_build_step_hot_exe(
    AvenBuildCommonOpts *opts,
    AvenGLTriangleBuildOpts *triangle_opts,
    AvenStr libaven_path,
    AvenStr root_path,
    AvenBuildStep *work_dir_step,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    AvenStr libavengl_root_path = aven_path(
        arena,
        root_path.ptr,
        "deps",
        "libavengl",
        NULL
    );

    AvenBuildStep *libavengl_dir_step = aven_arena_create(AvenBuildStep, arena);
    *libavengl_dir_step = aven_build_common_step_subdir(
        work_dir_step, aven_str("libavengl"), arena
    );

    AvenStr include_data[] = {
        libaven_build_include_path(libaven_path, arena),
        libavengl_build_include_path(libavengl_root_path, arena),
        libavengl_build_include_gles2(libavengl_root_path, arena),
        libavengl_build_include_glfw(libavengl_root_path, arena),
    };
    AvenStrSlice includes = {
        .ptr = include_data,
        .len = countof(include_data)
    };
    AvenStr macro_data[] = { aven_str("HOT_RELOAD") };
    AvenStrSlice macros = { .ptr = macro_data, .len = countof(macro_data) };

    if (triangle_opts->no_glfw) {
        includes.len -= 1;
    }

    AvenBuildStep *main_step = aven_arena_create(AvenBuildStep, arena);
    *main_step = aven_build_common_step_cc_ex(
        opts,
        includes,
        macros,
        aven_path(arena, root_path.ptr, "src", "main.c", NULL),
        work_dir_step,
        arena
    );

    AvenBuildStep *obj_refs[3] = {
        main_step,
    };
    AvenBuildStepPtrSlice objs = {
        .ptr = obj_refs,
        .len = 1,
    };

    if (!triangle_opts->no_glfw) {
        AvenBuildStep *glfw_step = aven_arena_create(AvenBuildStep, arena);
        *glfw_step = libavengl_build_step_glfw(
            opts,
            &triangle_opts->libavengl,
            libavengl_root_path,
            libavengl_dir_step,
            arena
        );
        objs.len += 1;
        slice_get(objs, objs.len - 1) = glfw_step;
    }

    if (triangle_opts->winutf8) {
        AvenBuildStep *libaven_dir_step = aven_arena_create(
            AvenBuildStep,
            arena
        );
        *libaven_dir_step = aven_build_common_step_subdir(
            work_dir_step,
            aven_str("libaven"),
            arena
        );
        AvenBuildStep *windres_step = aven_arena_create(AvenBuildStep, arena);
        *windres_step = libaven_build_step_windres_manifest(
            opts,
            libaven_path,
            libaven_dir_step,
            arena
        );
        objs.len += 1;
        slice_get(objs, objs.len - 1) = windres_step;
    }
    assert(objs.len <= countof(obj_refs));

    return aven_build_common_step_ld_exe_ex(
        opts,
        triangle_opts->syslibs,
        objs,
        out_dir_step,
        aven_str("avengl_triangle"),
        true,
        arena
    );
}

static inline AvenBuildStep avengl_triangle_build_step_exe(
    AvenBuildCommonOpts *opts,
    AvenGLTriangleBuildOpts *triangle_opts,
    AvenStr libaven_path,
    AvenStr root_path,
    AvenBuildStep *work_dir_step,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    AvenStr libavengl_root_path = aven_path(
        arena,
        root_path.ptr,
        "deps",
        "libavengl",
        NULL
    );

    AvenBuildStep *libavengl_dir_step = aven_arena_create(AvenBuildStep, arena);
    *libavengl_dir_step = aven_build_common_step_subdir(
        work_dir_step, aven_str("libavengl"), arena
    );

    AvenStr include_data[] = {
        libaven_build_include_path(libaven_path, arena),
        libavengl_build_include_path(libavengl_root_path, arena),
        libavengl_build_include_gles2(libavengl_root_path, arena),
        libavengl_build_include_glfw(libavengl_root_path, arena),
    };
    AvenStrSlice includes = {
        .ptr = include_data,
        .len = countof(include_data)
    };
    AvenStrSlice macros = { 0 };

    if (triangle_opts->no_glfw) {
        includes.len -= 1;
    }

    AvenBuildStep *main_step = aven_arena_create(AvenBuildStep, arena);
    *main_step = aven_build_common_step_cc_ex(
        opts,
        includes,
        macros,
        aven_path(arena, root_path.ptr, "src", "main.c", NULL),
        work_dir_step,
        arena
    );

    AvenBuildStep *obj_refs[3] = {
        main_step,
    };
    AvenBuildStepPtrSlice objs = {
        .ptr = obj_refs,
        .len = 1,
    };

    if (!triangle_opts->no_glfw) {
        AvenBuildStep *glfw_step = aven_arena_create(AvenBuildStep, arena);
        *glfw_step = libavengl_build_step_glfw(
            opts,
            &triangle_opts->libavengl,
            libavengl_root_path,
            libavengl_dir_step,
            arena
        );
        objs.len += 1;
        slice_get(objs, objs.len - 1) = glfw_step;
    }

    if (triangle_opts->winutf8) {
        AvenBuildStep *libaven_dir_step = aven_arena_create(
            AvenBuildStep,
            arena
        );
        *libaven_dir_step = aven_build_common_step_subdir(
            work_dir_step,
            aven_str("libaven"),
            arena
        );
        AvenBuildStep *windres_step = aven_arena_create(AvenBuildStep, arena);
        *windres_step = libaven_build_step_windres_manifest(
            opts,
            libaven_path,
            libaven_dir_step,
            arena
        );
        objs.len += 1;
        slice_get(objs, objs.len - 1) = windres_step;
    }
    assert(objs.len <= countof(obj_refs));

    return aven_build_common_step_ld_exe_ex(
        opts,
        triangle_opts->syslibs,
        objs,
        out_dir_step,
        aven_str("avengl_triangle"),
        true,
        arena
    );
}

#endif // AVENGL_TRIANGLE_BUILD_H
