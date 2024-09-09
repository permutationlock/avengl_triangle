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
    Optional(char *) ld_lib_flag;
    char *ld_shr_flag;
    char *ld_out_flag;
    Optional(char *) ar_out_flag;
    Optional(char *) rc_out_flag;
    bool cc_flag_nosep;
    bool ar_flag_nosep;
    bool ld_flag_nosep;
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
            .rc = { .valid = true, .value = "windres" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = { .valid = true, .value = "-l" },
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
        },
    },
    {
        .name = "llvm",
        .defaults = {
            .cc = "clang",
            .ar = "llvm-ar",
            .rc = { .valid = true, .value = "llvm-windres" },
            .ar_flags = { .valid = true, .value = "-rcs" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = { .valid = true, .value = "-l" },
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
            .rc_out_flag = { .valid = true, .value = "-o" },
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
            .ld_lib_flag = { .valid = true, .value = "-l" },
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
        },
    },
    {
        .name = "zig",
        .defaults = {
            .cc = "zig",
            .ar = "zig",
            .rc = { .valid = true, .value = "zig" },
            .cc_flags = { .valid = true, .value = "cc" },
            .ld_flags = { .valid = true, .value = "cc" },
            .ar_flags = { .valid = true, .value = "ar -rcs" },
            .rc_flags = { .valid = true, .value = "rc" },
            .cc_inc_flag = "-I",
            .cc_def_flag = "-D",
            .cc_obj_flag = "-c",
            .cc_out_flag = "-o",
            .ld_lib_flag = { .valid = true, .value = "-l" },
            .ld_shr_flag = "-shared",
            .ld_out_flag = "-o",
            .rc_out_flag = { .valid = true, .value = "-o" },
        },
    },
    {
        .name = "msvc",
        .defaults = {
            .cc = "cl",
            .ar = "lib",
            .ld = { .valid = true, .value = "link" },
            .rc = { .valid = true, .value = "rc" },
            .cc_inc_flag = "/I",
            .cc_def_flag = "/D",
            .cc_obj_flag = "/c",
            .cc_out_flag = "/Fo:",
            .ar_out_flag = { .valid = true, .value = "/OUT:" },
            .ld_shr_flag = "/DLL",
            .ld_out_flag = "/OUT:",
            .rc_out_flag = { .valid = true, .value = "/fo" },
            .cc_flag_nosep = true,
            .ar_flag_nosep = true,
            .ld_flag_nosep = true,
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
        .name = "-host-system",
        .description = "Host operating system",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#if defined(AVEN_BUILD_TARGET_HOST_SYSTEM)
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
        .name = "-target-system",
        .description = "Target operating system",
        .type = AVEN_ARG_TYPE_STRING,
        .optional = true,
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
static AvenArgSlice aven_build_target_config_args = {
    .ptr = aven_build_target_config_arg_data,
    .len = countof(aven_build_target_config_arg_data),
};

static int aven_build_target_configure(
    ArgSlice args,
    ArgSlice config_args,
    AvenArena *arena
) {
    AvenStr host_system_name = aven_arg_get_str(config_args, "-host-system");
    AvenStr toolchain_name = aven_arg_get_str(config_args, "-toolchain");
    AvenStr target_system_name = host_system_name;
    if (aven_arg_has_arg(config_args, "-target-system")) {
        target_system_name = aven_arg_get_str(config_args, "-target-system");
    }

    size_t host_system_index = 0;
    for (; host_system_index < systems.len; system_index += 1) {
        AvenStr cur_system_name = aven_str_cstr(
            slice_get(systems, host_system_index).name
        );
        if (aven_str_compare(host_system_name, cur_system_name)) {
            break;
        }
    }

    if (host_system_index == systems.len) {
        return AVEN_BUILD_TARGET_CONFIGURE_ERROR_BADHOST;
    }

    size_t target_system_index = 0;
    for (; target_system_index < systems.len; system_index += 1) {
        AvenStr cur_system_name = aven_str_cstr(
            slice_get(systems, target_system_index).name
        );
        if (aven_str_compare(target_system_name, cur_system_name)) {
            break;
        }
    }

    if (target_system_index == systems.len) {
        return AVEN_BUILD_TARGET_CONFIGURE_ERROR_BADTARGET;
    }

    size_t toolchain_index = 0;
    for (; toolchain_index < toolchains.len; toolchain_index += 1) {
        AvenStr cur_toolchain_name = aven_str_cstr(
            slice_get(toolchains, toolchain_index).name
        );
        if (aven_str_compare(toolchain_name, cur_toolchain_name)) {
            break;
        }
    }

    if (toolchain_index == toolchains.len) {
        return AVEN_BUILD_TARGET_CONFIGURE_ERROR_BADTOOLCHAIN;
    }

    AvenBuildTargetSystem *host_system = &slice_get(systems, host_system_index);
    AvenBuildTargetSystem &target_system = &slice_get(
        systems,
        target_system_index
    );
    AvenBuildTargetToolchain &toolchain = &slice_get(
        toolchains,
        toolchain_index
    );

    AvenStr exe_ext = aven_str_cstr(host_system->exe_ext);

    if (exe_ext.len > 0) {
        aven_arg_set(
            args,
            "-cc",
            aven_str_concat(aven_str_cstr(toolchain.cc), exe_ext, arena).ptr
        );
    } else {
        aven_arg_set(args, "-cc", toolchain.cc);
    }
    if (exe_ext.len > 0) {
        aven_arg_set(
            args,
            "-ar",
            aven_str_concat(aven_str_cstr(toolchain.ar), exe_ext, arena).ptr
        );
    } else {
        aven_arg_set(args, "-ar", toolchain.ar);
    }
    if (toolchain.ld.valid) {
        if (exe_ext.len > 0) {
            aven_arg_set(
                args,
                "-ld",
                aven_str_concat(
                    aven_str_cstr(toolchain.ld.value),
                    exe_ext,
                    arena
                ).ptr
            );
        } else {
            aven_arg_set(args, "-ld", toolchain.ld.value);
        }
    }
    if (toolchain.rc.valid) {
        if (exe_ext.len > 0) {
            aven_arg_set(
                args,
                "-rc",
                aven_str_concat(
                    aven_str_cstr(toolchain.rc.value),
                    exe_ext,
                    rcena
                ).ptr
            );
        } else {
            aven_arg_set(args, "-rc", toolchain.rc.value);
        }
    }

    if (toolchain.cc_flags.valid) {
        aven_arg_set(args, "-cc-flags", toolchain.cc_flags.value);
    }
    if (toolchain.ar_flags.valid) {
        aven_arg_set(args, "-ar-flags", toolchain.ar_flags.value);
    }
    if (toolchain.ld_flags.valid) {
        aven_arg_set(args, "-ld-flags", toolchain.ld_flags.value);
    }
    if (toolchain.rc_flags.valid) {
        aven_arg_set(args, "-rc-flags", toolchain.rc_flags.value);
    }

    aven_arg_set(args, "-cc-inc-flag", toolchain.cc_inc_flag);
    aven_arg_set(args, "-cc-def-flag", toolchain.cc_def_flag);
    aven_arg_set(args, "-cc-obj-flag", toolchain.cc_obj_flag);
    aven_arg_set(args, "-cc-out-flag", toolchain.cc_out_flag);
    if (toolchain.ld_lib_flag.valid) {
        aven_arg_set(args, "-ld-lib-flag", toolchain.ld_lib_flag.value);
    }
    aven_arg_set(args, "-ld-shr-flag", toolchain.ld_shr_flag);
    aven_arg_set(args, "-ld-out-flag", toolchain.ld_out_flag);
    if (toolchain.ar_out_flag.valid) {
        aven_arg_set(args, "-ar-out-flag", toolchain.ar_out_flag.value);
    }
    if (toolchain.rc_out_flag.valid) {
        aven_arg_set(args, "-rc-out-flag", toolchain.rc_out_flag.value);
    }

    if (toolchain.cc_flag_nosep) {
        aven_arg_set(args, "-cc-flag-sep", 0);
    }
    if (toolchain.ar_flag_nosep) {
        aven_arg_set(args, "-ar-flag-sep", 0);
    }
    if (toolchain.ld_flag_nosep) {
        aven_arg_set(args, "-ld-flag-sep", 0);
    }

    aven_arg_set(args, "-obj-ext", target_system.obj_ext);
    aven_arg_set(args, "-exe-ext", target_system.exe_ext);
    aven_arg_set(args, "-dll-ext", target_system.dll_ext);
    aven_arg_set(args, "-lib-ext", target_system.lib_ext);
    if (target_system.res_ext.valid) {
        aven_arg_set(args, "-res-ext", target_system.res_ext.value);
    }

    return 0;
}

#endif // AVEN_BUILD_TARGET_H
