# Aven GL Triangle

The repo contains a self-contained OpenGL ES2 application that can be built on
most systems with most C compilers[^1]. It primarily serves
as an example of the [libaven][1] C build system.

## Dependencies

There are no library or header build dependencies apart from a C compiler that
supports at least the C99 or C11 standards for POSIX or Windows.

In most cases dynamic linking/loading must be supported. The locally built
and statically linked GLFW library will dynamically load the system shared
objects for OpenGL graphics and window managment. Thus, e.g. a static linking
only [musl][2] toolchain like `zig cc` with `-target x86_64-linux-musl` won't
work.

If you are targeting a special system that has its own GLFW implementation,
e.g. the emulated POSIX environment of [Emscripten][3],
options are avaliable to omit locally building and linking GLFW and to link any
desired system libraries.

## Building

If you are on a Linux machine with the `CC` environment variable set to either
`gcc` or `clang`, then the build system can be compiled using
the provided tiny two command `Makefile`.

```
make
./build
```

The build executable can also be built directly using practically any C
compiler, but some flags and/or macros will require tweaking for compilers
other than `gcc` or `clang`.

```
gcc -o build build.c
./build
```

The TinyCC compiler needs either `-std=c11` flag or a definition of the
`__BIGGEST_ALIGNMENT__` macro to the appropratiate
value for the given system, e.g. 16 for `x86_64` Linux.

```
tcc -std=c11 -o build build.c
./build
```

The `cproc` compiler uses the system C preprocessor and is therefore not
identifiable from preprocessor macros. Thus we must manually inform the build
system to use `cproc` for further compilation and linking.

```
cproc -DAVEN_BUILD_COMMON_DEFAULT_CC=\"cproc\" -o build build.c
./build
```

On a Windows machine there are several one line `.bat` files provided as
examples, but just as on Linux you should be able to simply compile the
`build.c` file with e.g. `cl.exe`.

```
cl.exe /std:c11 /Fe:build.exe build.c
.\build.exe
```

MinGW and `clang` (MSVC and GNU) should also work out-of-the-box on Windows.

```
gcc.exe -o build.exe build.c
.\build.exe
```

A more involved example would be using the `zig` toolchain. The Zig C compiler
is simply a wrapper around `clang` and thus is not identifiable
via the preprocessor. Arguments must be provided to indicate how
the Zig tools should run, e.g. `zig cc` for C compilation and linking and
`zig rc` for Windows resource compilation. See `zig_make.bat` for an example
command that should work on both Windows and Linux.

## Source watching and hot reloading

If you have built the build system via any of the above methods, add a `watch`
argument to run the `build` executable in hot reload mode.

```
./build watch
```

The project will build and run while the build system watches for changes to
source files.
If a file in the root of `src/` is modified the application will close,
rebuild, and run. However, if changes are made to files in
`src/game/`, they will hot reload into the running executable.

## Cross-compiling

Build defaults are set using the preprocessor macros defined by the C compiler
that is used to compile the build system. As seen above, these defaults can be
manually overridden by defining macros, or the corresponding flags can be set at
runtime.

To see a full list of available command line options you can run the build
executable witht the `help` argument.

```
./build help
```

Here is an example using command line arguments to do a release build
of an `x86_64` Windows executable on a Linux mchine using MinGW-w64.

```
gcc -o build build.c
./build -cc "x86_64-w64-mingw32-gcc" -ar "x86_64-w64-mingw32-ar" \
    -windres "x86_64-w64-mingw32-windres" -ccflags "-DNDEBUG -O3" \
    -glfw-ccflags "-DNDEBUG -O3" -stb-ccflags "-DNDEBUG -O3" -winutf8 1 \
    -exext ".exe" -soext ".dll" -obext ".o" -wrext ".o" \
    -syslibs "kernel32 user32 shell32 gdi32" -ldwinflag "-mwindows"
```

## Building for web

With the `emcc` compiler we can use the build system to make a web page to host
our application. Emscripten provides its own emulated GLFW so we don't need to
build our own.

```
./build -cc emcc -ccflags "-DNDEBUG -O3" -stb-ccflags "-DNDEBUG -O3" -no-glfw \
    -exext ".html .js .wasm" -syslibs "" -ldflags "-s ASYNCIFY -s USE_GLFW=3"
```

Host the provided web page using your favorite web server, e.g. the provided
`server.go`, open it in a browser, and voila! A graphical app in the web.

```
go run server.go &
firefox 127.0.0.1:8081/avengl_triangle.html
```

## Cleaning up

Run the build executable with the `clean` argument to remove all build
artifacts.

```
./build clean
```

Note that if you passed special command line arguments to the build command, you
will need to pass those same arguments to the clean command. The clean command
only attempts to remove files and directories that it is able to create.

[^1]: No Apple support, and I have only tested on `x86_64` Linux and Windows.
    Setting up a cross-compilation toolchain for
    MacOS that supports the various graphical frameworks seems like a pain and
    I have no Apple device to test with. The way that GLFW implements Mac
    windowing support also looks suspect to me.

[1]: https://github.com/permutationlock/libaven
[2]: https://musl.libc.org/
[3]: https://emscripten.org/

