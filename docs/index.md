---
layout: default
title: Zzapi — Pizza Factory Simulator
---

# 🍕 Zzapi — Pizza Factory Simulator

A real-time **C++17 + Dear ImGui** factory simulation built for **EC5209 — OOP with C++**
(GIST, Spring 2026). Raw dough enters, flows through machines and conveyor belts, and leaves
as a boxed pizza that you ship against incoming customer orders for money.

[View on GitHub](https://github.com/sb-p-0x7/Zzapi){: .btn }

<!-- After you take a screenshot of the running app, save it as docs/screenshot.png and
     uncomment the next line:
![Zzapi dashboard](screenshot.png)
-->

---

## What it demonstrates

This project shows the **four pillars of object-oriented programming** in a non-trivial,
interactive system:

- **Abstraction** — abstract `Machine`, `Pizza`, and `Scenario` base classes with pure virtual methods.
- **Encapsulation** — every data member is `private`/`protected`; state changes only through methods.
- **Inheritance** — two-level hierarchies (`Machine → NonConveyor/Conveyor → 8 concrete machines`).
- **Polymorphism** — the simulation loop is `for (Machine* m : pipeline) m->update(tick);` with **no type branching**.

---

## Features

- **Six ImGui windows:** Simulation Control, Factory Floor, Inspector, Event Log, Statistics, Orders.
- **Animated Factory Floor:** snake pipeline, drawn pizzas, moving conveyor belts, and **semicircle U-turn belts** on row wraps.
- **Click any machine or belt** to open it in the Inspector and **tune it live** (health, process time / belt speed, breakdown odds).
- **Five runtime scenarios:** Normal flow · Bottleneck · Random breakdowns · Overflow · **Free Play** (game mode with orders).
- **Fully decoupled UI/backend** — the two sides talk only through value structs in `bridge.h`.

---

## How it's wired (UI ⇄ backend)

The simulation never touches ImGui, and the UI never touches a simulation object. Only `app.cpp`
sees both sides; each frame it runs:

```cpp
FactorySnap snap = factory.snapshot();   // read-only snapshot
view.Render(snap, cmd);                  // buttons set cmd flags
controller.applyCmd(cmd);                // cmd -> factory control methods
cmd = FactoryCmd{};                      // clear (a command fires once)
controller.advance(dt);                  // step the simulation
```

The UI is always one frame behind — it reads `snap.state`, never `machine.state`.

---

## Build & run

```bash
# macOS / Linux  (GLFW + ImGui are fetched automatically by CMake)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/PizzaFactory

# Windows: cmake --build build --config Release  ->  build\Release\PizzaFactory.exe
```

---

## Docs

- [README (English)](https://github.com/sb-p-0x7/Zzapi/blob/main/README.md)
- [README (한국어)](https://github.com/sb-p-0x7/Zzapi/blob/main/README.ko.md)
- [DESIGN.md — architecture, UML & ER diagrams](https://github.com/sb-p-0x7/Zzapi/blob/main/DESIGN.md)
