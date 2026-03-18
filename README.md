# Interactive Fireworks Display
COSC 3307 - 3D Computer Graphics | Nipissing University

A real-time 3D fireworks simulation using C++ and OpenGL. Rockets launch into the night sky and explode into colourful particle bursts with realistic physics.

## Controls

| Key | Action |
|---|---|
| Arrow keys | Rotate camera |
| W / S | Move forward / backward |
| A / D | Roll camera |
| SPACE or 1 | Launch firework |
| Q | Quit |

## How to Build

Requires Visual Studio 2022 and CMake.

```
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

## Libraries Used
- OpenGL, GLEW, GLFW, GLM, SOIL
