# C++ For Game Developer

Header-only C++ project exploring OOP, inheritance, polymorphism, filesystem utilities, and OpenGL rendering with GLX/X11.

## Build

```sh
make
./main.out [count]         # N windows, 800x600 each
./main.out [w] [h]         # 1 window with custom size
./main.out [count] [w1 h1 w2 h2 ...]  # N windows, each with own size
```

Requires: `clang++`, X11, OpenGL (`-lX11 -lGL`).

## Structure

| File | Description |
|---|---|
| `GameObject.hpp` | Base class `GameObject` + `Enemy` (polymorphic `ds_info()`) |
| `Player.hpp` | `GameCharacter : GameObject` → `PlayerCharacter` |
| `Weapon.hpp` | `Weapon` → `Sword`, `MagicStaff`, `Wizard` (multiple + diamond inheritance) |
| `Shape.hpp` | `Shape` → `Rectangle`, `Circle`, `Triangle` |
| `Window.hpp` | `WindowBuffer` — X11 window + software framebuffer |
| `GLContext.hpp` | GLX context + shader compilation + VAO/VBO rendering |
| `WindowMultiContext.hpp` | Multi-window manager with event dispatching per XID |
| `FileBuffer.hpp` | File reader (binary, into `vector<char>`) |
| `Filesystem.hpp` | File scanner by extension pattern, loads into `FileBuffer` |
| `Fonts.hpp` | Font loader (Win32/Linux X11) |
| `shaders/vert.vs.glsl` | GLSL 330 vertex shader |
| `shaders/frag.fs.glsl` | GLSL 330 fragment shader |

## Inheritance Trees

```
GameObject ← GameCharacter ← PlayerCharacter
          ← Enemy

Weapon ← Sword ─┐
       ← MagicStaff ─┼─ Wizard

Shape ← Rectangle
      ← Circle
      ← Triangle
```

## Usage

```sh
./main.out                # 1 window, 800x600
./main.out 3              # 3 windows, 800x600 each
./main.out 1920 1080      # 1 window, 1920x1080
./main.out 2 640 480 320 240  # 2 windows
```
