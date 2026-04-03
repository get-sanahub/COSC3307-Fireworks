# COSC 3307 – Interactive Fireworks Display
3D Computer Graphics | Nipissing University

Real-time 3D fireworks simulation built with C++ and OpenGL 3.3.

## Features
- Four firework types: Starburst, Fountain, Trail Rocket, Cascade
- Phong shading (ambient + diffuse + specular) on 3D scene objects
- 3D scene with fountain vase, roads, human figures, and moon
- Billboard quad particles with additive blending
- Dynamic explosion lighting, procedural grass texture, night sky

## Controls

| Key | Action |
|-----|--------|
| `1` / `Space` | Starburst |
| `2` | Fountain |
| `3` | Trail rocket (Phong demo) |
| `4` | Cascade |
| Arrow keys | Camera pitch / yaw |
| `W` / `S` | Move forward / back |
| `A` / `D` | Roll |
| `Q` | Quit |

## Build

Requires CMake 3.10+ and Visual Studio 2022.

```
mkdir Build && cd Build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Debug
cd Debug && Fireworks_d.exe
```

## Libraries
OpenGL 3.3, GLFW 3, GLEW, GLM 0.9.5.4
