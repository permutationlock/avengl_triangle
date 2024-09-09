#ifndef AVEN_BUILD_TARGET_H
#define AVEN_BUILD_TARGET_H

typedef struct {
    char *cc;
    char *ar;
    Optional(char *) ld;
    Optional(char *) rc;
    Optional(char *) cc_flags;
    Optional(char *) cc_flags;
    Optional(char *) ld_flags;
    Optional(char *) ar_flags;
    Optional(char *) rc_flags;
    char *cc_inc_flag;
    char *cc_def_flag;
    char *cc_obj_flag;
    char *cc_out_flag;
    char *ld_lib_flag;
    char *ld_shr_flag;
    char *ld_out_flag;
    Optional(char *) ar_out_flag;
    Optional(char *) rc_out_flag;
    bool cc_flag_sep;
    bool ar_flag_sep;
    bool ld_flag_sep;
} AvenBuildTargetToolchainDefaults;

typedef struct {
    char *name;
    AvenBuildTargetToolchainDefaults defaults;
} AvenBuildTargetToolchain;

typedef Slice(AvenBuildTargetToolchain) AvenBuildTargetToolchainSlice;

typedef struct {
    char *obj_ext;
    char *exe_ext;
    char *dll_ext;
    char *lib_ext;
    Optional(char *) res_ext;
} AvenBuildTargetSystemDefaults;

typedef struct {
    char *name;
    AvenBuildTargetSystemDefaults defaults;
} AvenBuildTargetSystem;

typedef Slice(AvenBuildTargetSystem) AvenBuildTargetSystemSlice;

static AvenBuildTargetToolchain aven_build_toolchain_data[] = {
    {
        .name = "gnu",
        .defaults = {
            .cc = "gcc",
            .ar = "ar",
            .ar_flags = { .valid = true, .value = "-rcs" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = "-l",
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
        },
    },
    {
        .name = "llvm",
        .defaults = {
            .cc = "clang",
            .ar = "llvm-ar",
            .ar_flags = { .valid = true, .value = "-rcs" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = "-l",
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
        },
    },
    {
        .name = "tinycc",
        .defaults = {
            .cc = "tcc",
            .ar = "tcc",
            .ar_flags = { .valid = true, .value = "-ar -rcs" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = "-l",
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
        },
    },
    {
        .name = "zig",
        .defaults = {
            .cc = "zig",
            .ar = "zig",
            .rc = "zig",
            .cc_flags = { .valid = true, .value = "cc" },
            .ld_flags = { .valid = true, .value = "cc" },
            .ar_flags = { .valid = true, .value = "ar -rcs" },
            .rc_flags = { .valid = true, .value = "rc" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = "-l",
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
        },
    },
};
static AvenBuildTargetToolchainSlice aven_build_target_toolchains = {
    .ptr = aven_build_target_toolchain_data,
    .len = countof(aven_build_target_toolchain_data),
};

static AvenBuildTargetSystem aven_build_target_system_data[] = {
    {
        .name = "posix",
        .defaults = {
            .obj_ext = ".o",
            .exe_ext = "",
            .dll_ext = ".so",
            .lib_ext = ".a",
        },
    },
    {
        .name = "windows-gnu",
        .defaults = {
            .obj_ext = ".o",
            .exe_ext = ".exe",
            .dll_ext = ".dll",
            .lib_ext = ".a",
            .res_ext = { .valid = true, .value = ".o" },
        },
    },
    {
        .name = "windows-msvc",
        .defaults = {
            .obj_ext = ".obj",
            .exe_ext = ".exe",
            .dll_ext = ".dll",
            .lib_ext = ".lib",
            .res_ext = { .valid = true, .value = ".res" },
        },
    },
}
static AvenBuildTargetSystemSlice aven_build_target_systems = {
    .ptr = aven_build_target_system_data,
    .len = countof(aven_build_target_system_data),
};

static AvenArg aven_build_target_config_arg_data = {
    {
        .name = "-system",
        .description = "Target operating system",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#if defined(AVEN_BUILD_TARGET_SYSTEM)
            .data = AVEN_BUILD_TARGET_SYSTEM,
#elif defined(_WIN32)
    #if defined(_MSC_VER)
            .data = "windows-msvc",
    #else
            .data = "windows-gnu",
    #endif
#else
            .data = "posix",
#endif
        },
    },
    {
        .name = "-toolchain",
        .description = "Build toolchain",
        .type = AVEN_ARG_TYPE_STRING,
#if defined(AVEN_BUILD_TARGET_TOOLCHAIN)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = AVEN_BUILD_TARGET_TOOLCHAIN,
        },
#elif defined(__clang__)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = "llvm",
        },
#elif defined(__GNU__)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = "gnu",
        },
#elif defined(_WIN32) and defined(_MSC_VER)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = "msvc",
        },
#elif defined(__TINYC__)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = "tinycc",
        },
#else
        .optional = true,
#endif
    },

};
AvenArgSlice aven_build_target_config_args = {
    .ptr = aven_build_target_config_arg_data,
    .len = countof(aven_build_target_config_arg_data),
};

static AvenBuildArgSlice aven_build_target_build_args(
    AvenBuildTargetSystemSlice systems,
    AvenBuildTargetToolchainSlice toolchains,
    AvenStr system_name,
    AvenStr toolchain_name,
    AvenArena *arena
) {
    AvenArgSlice args = { .len = aven_build_common_args.len };
    args.ptr = aven_arena_create_array(AvenArg, arena, args.len);
}

#endif // AVEN_BUILD_TARGET_H
