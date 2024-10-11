#ifndef AVENGL_TRIANGLE_BUILD_H
#define AVENGL_TRIANGLE_BUILD_H

static inline AvenArgSlice avengl_triangle_build_args(AvenArena *arena) {
    AvenArgSlice gl_args = libavengl_build_args();
    AvenArgSlice libaven_args = libaven_build_args();

    AvenArgSlice args = { .len = 1 + gl_args.len };
    args.ptr = aven_arena_create_array(AvenArg, arena, args.len); 
    size_t i = 0;
    for (size_t j = 0; j < gl_args.len; j += 1) {
        slice_get(args, i) = slice_get(gl_args, j);
        i += 1;
    }
    for (size_t j = 0; j < libaven_args.len; j += 1) {
        slice_get(args, i) = slice_get(libaven_args, j);
        i += 1;
    }

    return args;
}

typedef struct {
    LibAvenGlBuildOpts libavengl;
    LibAvenBuildOpts libaven;
} AvenGlTriangleBuildOpts;

static inline AvenGlTriangleBuildOpts avengl_triangle_build_opts(
    AvenArgSlice args,
    AvenArena *arena
) {
    AvenGlTriangleBuildOpts opts = { 0 };
    opts.libavengl = libavengl_build_opts(args, arena);
    opts.libaven = libaven_build_opts(args, arena);

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
    AvenGlTriangleBuildOpts *triangle_opts,
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

    AvenBuildStep *libavengl_dir_step = aven_arena_create(AvenBuildStep, arena);
    *libavengl_dir_step = aven_build_common_step_subdir(
        work_dir_step, aven_str("libavengl"), arena
    );

    AvenBuildStep *stb_step = aven_arena_create(AvenBuildStep, arena);
    *stb_step = libavengl_build_step_stb(
        opts,
        &triangle_opts->libavengl,
        libaven_build_include_path(libaven_path, arena),
        libavengl_root_path,
        libavengl_dir_step,
        arena
    );

    AvenStr include_data[] = {
        libaven_build_include_path(libaven_path, arena),
        libavengl_build_include_path(libavengl_root_path, arena),
        libavengl_build_include_gles2(libavengl_root_path, arena),
    };
    AvenStrSlice includes = slice_array(include_data);
    AvenStr macro_data[] = { aven_str("HOT_RELOAD") };
    AvenStrSlice macros = slice_array(macro_data);

    AvenBuildStep *game_step = aven_arena_create(AvenBuildStep, arena);
    *game_step = aven_build_common_step_cc_ex(
        opts,
        includes,
        macros,
        aven_path(arena, root_path.ptr, "src", "game", "game.c", NULL),
        work_dir_step,
        arena
    );

    AvenBuildStep *obj_refs[] = { game_step, stb_step };
    AvenBuildStepPtrSlice objs = slice_array(obj_refs);

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
    AvenGlTriangleBuildOpts *triangle_opts,
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
    AvenStrSlice includes = slice_array(include_data);
    AvenStr macro_data[] = { aven_str("HOT_RELOAD") };
    AvenStrSlice macros = slice_array(macro_data);

    if (triangle_opts->libavengl.no_glfw) {
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

    AvenBuildStep *obj_refs[4] = {
        main_step,
    };
    AvenBuildStepPtrSlice objs = {
        .ptr = obj_refs,
        .len = 1,
    };

    if (!triangle_opts->libavengl.no_glfw) {
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

    if (triangle_opts->libaven.winutf8) {
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
        triangle_opts->libavengl.syslibs,
        objs,
        out_dir_step,
        aven_str("avengl_triangle"),
        true,
        arena
    );
}

static inline AvenBuildStep avengl_triangle_build_step_exe(
    AvenBuildCommonOpts *opts,
    AvenGlTriangleBuildOpts *triangle_opts,
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

    AvenBuildStep *stb_step = aven_arena_create(AvenBuildStep, arena);
    *stb_step = libavengl_build_step_stb(
        opts,
        &triangle_opts->libavengl,
        libaven_build_include_path(libaven_path, arena),
        libavengl_root_path,
        libavengl_dir_step,
        arena
    );

    AvenStr include_data[] = {
        libaven_build_include_path(libaven_path, arena),
        libavengl_build_include_path(libavengl_root_path, arena),
        libavengl_build_include_gles2(libavengl_root_path, arena),
        libavengl_build_include_glfw(libavengl_root_path, arena),
    };
    AvenStrSlice includes = slice_array(include_data);
    AvenStrSlice macros = { 0 };

    if (triangle_opts->libavengl.no_glfw) {
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

    AvenBuildStep *obj_refs[4] = {
        main_step,
        stb_step,
    };
    AvenBuildStepPtrSlice objs = {
        .ptr = obj_refs,
        .len = 2,
    };

    if (!triangle_opts->libavengl.no_glfw) {
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

    if (triangle_opts->libaven.winutf8) {
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
        triangle_opts->libavengl.syslibs,
        objs,
        out_dir_step,
        aven_str("avengl_triangle"),
        true,
        arena
    );
}

#endif // AVENGL_TRIANGLE_BUILD_H
