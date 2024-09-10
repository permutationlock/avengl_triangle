# Aven GL Triangle

The repo contains a self-contained OpenGL ES2 triangle that can be built on most
systems with any C compiler[^1]. It primarily serves
as an example of the simple [libaven][1] C build system.

## Dependencies

There are no library or header build dependencies apart from a C compiler that
supports the C99 or C11 for a hosted (not freestanding) POSIX or Windows target.

Generally dynamic linking/loading must be supported since GLFW will dynamically
load
the OS shared objects for OpenGL graphics and window managment. E.g. a static
[musl][2] toolchain won't work (but dynamically linked musl libc works fine).

If you are targeting a special system with static libraries available for window
management and OpenGL, e.g. the emulated POSIX environment of [Emscripten][3],
options are avaliable to omit building and linking a local GLFW and to link any
desired system libraries.

## Building

If you are on a Linux machine with `gcc` or `clang` as the system `CC`
environment variable, then the build system can be built and run using:

```
make
./build
```

The build executable can also be built directly using practically any C
compiler, but some flags and/or macros may require tweaking.

```
gcc -Wall -Wextra -o build build.c
```

The TinyCC compiler needs to either define `__BIGGEST_ALIGNMENT__` or use C11
for the `alignof` operator.

```
tcc -std=c11 -o build build.c
./build
```

The `cproc` compiler uses the system C preprocessor and thus is not identifiable
from preprocessor macros. Thus we must manually inform the build system to use
`cproc` for further compilation and linking.

```
cproc -D AVEN_BUILD_COMMON_DEFAULT_CC=\"cproc\" -o build build.c
./build
```

On a Windows machine there are several one line `.bat` files provided as
examples, but just as on Linux you should be able to simply compile the
`build.c` file with e.g. `cl.exe`, `clang.exe`, or `gcc.exe`.

```
cl.exe /std:c11 /W3 /Fe:build.exe build.c
.\build.exe
```

```
gcc.exe -Wall -Wextra -o build.exe build.c
.\build.exe
```

A more involved case is building the project using only the `zig` binary.
Arguments must be provided to indicate which tool to run, e.g. `zig cc` or
`zig rc`. See `zig_make.bat` for an example command that should work on Windows
or Linux.

## Source watching and hot reloading

The real power of a C build executable shows its face with the `watch` command.
If you have built the build system via any of the above methods, add a `watch`
argument to run the `build` executable in hot reload mode.

```
./build watch
```

The project will build adn run and watch for changes to source files.
If a file in the root of `src/` is modified, then the game will close,
rebuild, and run. However, if changes are made to files in
`src/game/`, they will hot reload into the running executable.

## Cross-compiling

Build defaults are set using the preprocessor macros defined by the C compiler
that is used to compile the build system. As seen above, these defaults can be
manually overridden by defining macros, or the corresponding flags can be set at
runtime.

Here is a runtime example building a Windows executable from Linux using
MinGW-w64.

```
./build -cc "x86_64-w64-mingw32-gcc" -ar "x86_64-w64-mingw32-ar" \
    -windres "x86_64-w64-mingw32-windres" -ccflags "-Wall -Wextra" \
    -glfw-ccflags "" -exext ".exe" -soext ".dll" -wrext ".o" \
    -syslibs "kernel32 user32 shell32 gdi32" -ldwinflag "-mwindows"
```

## Building for web

With the `emcc` compiler we can use the build system to make a web page to host
our application. Emscripten provides its own emulated GLFW so we don't need to
build our own.

```
./build -cc "emcc" -ldflags "-s ASYNCIFY -s USE_GLFW=3" \
    -exext ".html .js .wasm" -no-glfw -syslibs ""
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

[^1]: Only tested on Linux and Windows. The C compiler must support at least
    the C99 or C11 standard.

[1]: https://github.com/permutationlock/libaven
[2]: https://musl.libc.org/
[3]: https://emscripten.org/
