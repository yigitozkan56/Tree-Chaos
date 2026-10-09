# 3D Kaos Motoru (3D Chaos Engine)

**3D Kaos Motoru** is a lightweight, high-performance Windows C++ 3D procedural fractal tree simulation engine. Built from scratch without heavy external graphics libraries, it utilizes native Win32 API and GDI software rasterization to render intricate, organically growing 3D tree structures in real time.

---

## 🌟 Key Features

- **Procedural 3D Growth Simulation**: Generates organic fractal branching structures dynamically in 3D space with stochastic angular deviation and customizable branching probabilities.
- **Spatial Partitioning Collision Avoidance**: Employs a 3D hash grid (`unordered_map` spatial grid) to perform proximity checks, preventing newly grown branches from intersecting or occupying occupied spatial cells.
- **Custom Software Rasterizer**: Direct frame-buffer pixel manipulation (`uint32_t` buffer) rendered seamlessly via Win32 `StretchDIBits`.
- **Interactive 3D Camera**: Free-look camera system supporting full rotation (Yaw/Pitch) and 6-DOF positional navigation (WASD + Q/E + Mouse Scroll).
- **Customizable Aesthetics**:
  - 5 HSV-based color palettes (Spectrum, Ocean/Cyan, Fire/Amber, Forest Green, Neon/Purple).
  - Dynamic branch node visualization.
- **Real-Time Control & Tuning**: Adjust branching probability, length scaling factor, shortening intervals, and root length on the fly during growth.
- **Branch Path Tracing**: Supports selection and visual path tracing from branch tips back to the tree root.

---

## 🎮 Keybindings & Controls

### 📹 Camera Navigation
| Key / Input | Action |
|---|---|
| **W / S** | Move camera forward / backward |
| **A / D** | Strafe camera left / right |
| **Q / E** | Move camera up / down |
| **Left Mouse Drag** | Orbit / Rotate camera view (Yaw & Pitch) |
| **Mouse Wheel** | Zoom in / out |

### 🌿 Tree Growth & Simulation
| Key | Action |
|---|---|
| **Space** | Advance growth by one turn/step |
| **Insert** | Toggle auto-play (continuous growth simulation) |
| **R** | Reset tree simulation to root state |
| **C** | Clear selected branch path highlight |

### ⚙️ Simulation Settings & Parameters
| Key | Action |
|---|---|
| **Tab** | Toggle settings HUD overlay |
| **1 – 5** | Change branch color palette |
| **N** | Toggle node/junction indicators |
| **O / L** | Increase / decrease branch splitting probability |
| **Up / Down** | Increase / decrease shortening interval frequency |
| **Left / Right** | Increase / decrease shortening multiplier ratio |
| **U / J** | Increase / decrease initial root branch length |

---

## 🛠️ Building and Running

### Prerequisites
- Operating System: Windows
- C++ Compiler with C++11 or later support (e.g., MSVC, GCC / MinGW)

### Compiling with GCC / MinGW

```bash
g++ -O2 tree.cpp -o tree.exe -mwindows -lgdi32 -luser32
./tree.exe
```

### Compiling with MSVC (Developer Command Prompt)

```cmd
cl /O2 /EHsc tree.cpp user32.lib gdi32.lib
tree.exe
```

---

## 📜 License

This project is open-source under the [MIT License](LICENSE). Copyright (c) 2026 Yiğit Özkan.
