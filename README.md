# Zzapi — Pizza Factory Simulator

> EC5209 *OOP with C++* (GIST, Spring 2026) — Factory Simulation Project
> A real-time pizza factory built in **C++17** with a **Dear ImGui** GUI.
> Raw dough enters, flows through machines and conveyor belts, and leaves as a boxed pizza
> that you ship against incoming customer orders for money.

🇰🇷 한국어 문서: [README.ko.md](README.ko.md)

---

## 1. Quick start

GLFW and Dear ImGui are fetched automatically by CMake — **nothing to install** beyond a
compiler and CMake 3.20+.

### macOS / Linux
```bash
./scripts/build.sh            # Debug build
./scripts/build.sh Release    # Release build
./build/PizzaFactory          # run
```

### Windows (Visual Studio toolchain)
```bat
scripts\build.bat Release
build\Release\PizzaFactory.exe
```

### Plain CMake (any platform)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

| Platform | Needs |
|---|---|
| macOS | Xcode Command Line Tools, CMake 3.20+ |
| Windows | Visual Studio 2019+ (Desktop C++), CMake 3.20+ |
| Linux | gcc/clang, CMake 3.20+, OpenGL + X11 dev headers |

> **Windows note:** the build sets MSVC `/utf-8` automatically (sources are UTF-8), so text
> renders correctly. MinGW/Clang/GCC need no extra flag.

---

## 2. How to use it

The app opens six windows (drag them around freely):

| Window | What it does |
|---|---|
| **Simulation Control** | `Start` / `Pause` / `Reset`, a **Speed** slider (1×–5×), a **Scenario** dropdown, the live tick counter and cash. |
| **Factory Floor** | The animated pipeline. Machines are colour-coded by state; pizzas are drawn on belts and move in real time. Click a node — or a row in the machine list below — to select it. Each row shows a progress / conveyor-load bar. |
| **Inspector** | Full detail for the selected machine: state, health bar, progress bar, queue depth, output count, process time, plus **Force Break** and **Instant Repair**. |
| **Event Log** | Timestamped scrolling log (shipments, breakdowns, scenario loads). **Clear** button + auto-scroll toggle. |
| **Statistics** | Running totals: finished goods, WIP, breakdowns, lost products, earnings. |
| **Orders** | Incoming customer orders (size + toppings required), the reward, and a countdown bar. |

**Machine state colours:** 🟦 Idle · 🟩 Working · 🟥 Broken (flashing) · ⬛ Off.

Press **Start**, watch dough flow left-to-right through the snaking pipeline, and try
**Force Break** on a machine to see the line back up and the loss counters move.

---

## 3. Architecture — UI ⇄ backend are fully decoupled

The simulation logic never touches ImGui, and the UI never touches a simulation object.
They communicate only through two **plain value structs** in [`src/bridge.h`](src/bridge.h):

```
   ┌────────────────────┐   FactorySnap  (read-only copy)   ┌─────────────────────┐
   │  DashboardView      │ ◄──────────────────────────────── │  Factory            │
   │  (ImGui, src/views) │                                   │  (sim, src/models)  │
   │  draws snapshot     │   FactoryCmd   (one-frame flags)  │  owns Machine* etc. │
   └────────────────────┘ ────────────────────────────────► └─────────────────────┘
            ▲                                                          ▲
            └───────────────── FactoryController ──────────────────────┘
                         maps cmd → Factory control methods
```

The only file that sees both sides is [`src/app.cpp`](src/app.cpp). Each frame:

```cpp
FactorySnap snap = factory.snapshot();   // 1. read-only snapshot
view.Render(snap, cmd);                  // 2. buttons set cmd flags
controller.applyCmd(cmd);                // 3. cmd → factory.start()/forceBreak()/…
cmd = FactoryCmd{};                      // 4. clear so a command never fires twice
controller.advance(dt);                  // 5. step the sim (speed × base tick rate)
```

The UI is therefore always **one frame behind** — it reads `snap.state`, never
`machine.state`. A button click leaves a note (`cmd.forceBreak = true`); `app.cpp` delivers
that note to the backend on the backend's terms.

### Type hierarchy (no `if/else` on concrete type in the sim loop)

```
Machine (abstract)
 ├─ NonConveyorMachine (abstract) ── processes one pizza for N ticks
 │    └─ DoughStretcher · SauceSpreader · CheeseSpreader · ToppingApplier
 │       · Oven · Cutter · Packager
 └─ ConveyorMachine (abstract) ───── carries pizzas across belt slots
      └─ ConveyorBelt

Pizza (abstract) ├─ RawDough (pipeline start) └─ BoxedPizza (pipeline end)
Scenario (abstract) ├─ FreePlay ├─ NormalFlow └─ RandomBreakdown
```

`Factory::step()` is `for (Machine* m : pipeline) m->update(tick);` — pure polymorphism.
Adding a new machine is one subclass with `transform()` + `displayName()`; **the sim loop and
the UI loop change by zero lines** because each machine fills its own `MachineSnap`.
Every data member on every class is `private`/`protected`.

See [DESIGN.md](DESIGN.md) for the full design doc, UML and ER diagrams.

---

## 4. The pipeline

```
IN ▸ Dough Stretcher → [Conveyor] → Sauce → Cheese → Topping → Oven → Cutter → [Conveyor] → Packager ▸ OUT
```

| Machine | Effect on the pizza |
|---|---|
| Dough Stretcher | dough → `STRETCHED`, sets size |
| Sauce Spreader | adds sauce |
| Cheese Spreader | adds cheese |
| Topping Applier | adds topping |
| Oven | dough → `BAKED` |
| Cutter | cuts into slices |
| Packager | `RawDough` → `BoxedPizza` (finished) |
| Conveyor | moves pizzas between machines (no processing) |

If a machine is broken or full, items back up naturally; items dropped at a full/broken
stage are counted as **lost products**.

### Scenarios (runtime dropdown)
- **Free Play** — default game mode, light breakdown chance.
- **Normal flow** — balanced pipeline, no breakdowns.
- **Random breakdowns** — elevated breakdown probability.

---

## 5. Assignment requirements → where they live

| Requirement (factory_project_v3) | Implementation |
|---|---|
| Abstract root + ≥2 inheritance levels | `Machine` → `NonConveyor/Conveyor` → concrete |
| Sim loop has no type branching | `Factory::step()` over `Machine*` |
| New machine = 0 loop/UI edits | each machine provides `displayName/icon/snapshot` |
| No public data members | all fields `private`/`protected` |
| Abstract product, start+end stages | `Pizza` → `RawDough` / `BoxedPizza` |
| UI / backend decoupled, one seam | `bridge.h` snapshot/cmd, only `app.cpp` sees both |
| Scenario dropdown (polymorphic) | `Scenario` + `scenarioNames` in snapshot |
| 5 required ImGui windows + widgets | Simulation Control / Factory Floor / Inspector / Event Log / Statistics (+ Orders) |

Required ImGui widgets are all present: `Button`, `SliderInt`, `Combo`, `ProgressBar`,
`TextColored`, `BeginChild/EndChild`, `Selectable`.

---

## 6. Project layout

```
Zzapi/
├── CMakeLists.txt          # cross-platform build (auto-fetches GLFW + ImGui)
├── DESIGN.md               # architecture, UML, ER diagrams
├── README.md / README.ko.md
├── scripts/
│   ├── build.sh / build.bat   # convenience build scripts
│   └── sim_test.cpp           # headless backend driver (no ImGui)
└── src/
    ├── bridge.h            # UI ⇄ backend contract (POD: FactorySnap / FactoryCmd)
    ├── main.cpp            # GLFW + ImGui boilerplate
    ├── app.{h,cpp}         # the one seam that sees both sides
    ├── models/             # backend (ImGui-free)
    │   ├── pizza.{h,cpp}  machine.{h,cpp}  factory.{h,cpp}
    │   └── order.{h,cpp}  scenario.{h,cpp}
    ├── controllers/
    │   └── factory_controller.{h,cpp}   # cmd → factory, tick cadence
    └── views/
        └── dashboard_view.{h,cpp}       # snapshot → ImGui (bridge.h only)
```

### Headless backend test
```bash
g++ -std=c++17 scripts/sim_test.cpp src/models/*.cpp -Isrc -o /tmp/simtest && /tmp/simtest
```
Runs 1200 ticks and prints machine states, orders, and the event log — handy for verifying
the simulation without opening the GUI.

---

## 7. Notes & known limitations
- **Fonts:** the bundled fonts cover Latin/Korean + BMP symbols (▶ ⏸ ↻ ⚠). Astral-plane
  colour emoji (🍕, 🔥…) are intentionally avoided because the default ImGui rasterizer
  cannot render them.
- **Balance:** the factory always produces **Medium** pizzas, so orders are generated at
  Medium too and are fulfillable (shipping a matching pizza clears the oldest order).
  Variable-size production with size-based order matching is a planned gameplay extension.
