# Zzapi — 피자 공장 시뮬레이터

> EC5209 *OOP with C++* (GIST, 2026 봄) — Factory Simulation 과제
> **C++17 + Dear ImGui**로 만든 실시간 피자 공장.
> 생반죽이 들어와 머신과 컨베이어 벨트를 거쳐 포장된 피자로 나오고,
> 들어오는 손님 주문에 맞춰 출고하면 돈을 법니다.

🇺🇸 English: [README.md](README.md)

> 참고: 프로그램의 **화면 텍스트는 모두 영어**입니다(영어 과제 제출용). 이 문서는 한국어 설명본입니다.

---

## 1. 빠른 시작

GLFW와 Dear ImGui는 CMake가 자동으로 내려받습니다 — 컴파일러와 CMake 3.20+ 외에 **설치할 것 없음**.

### macOS / Linux
```bash
./scripts/build.sh            # Debug 빌드
./scripts/build.sh Release    # Release 빌드
./build/PizzaFactory          # 실행
```

### Windows (Visual Studio 툴체인)
```bat
scripts\build.bat Release
build\Release\PizzaFactory.exe
```

### CMake 직접 사용 (모든 플랫폼)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

| 플랫폼 | 필요 사항 |
|---|---|
| macOS | Xcode Command Line Tools, CMake 3.20+ |
| Windows | Visual Studio 2019+ (Desktop C++), CMake 3.20+ |
| Linux | gcc/clang, CMake 3.20+, OpenGL + X11 개발 헤더 |

> **Windows 주의:** 소스가 UTF-8이라 빌드에서 MSVC `/utf-8`를 자동 지정합니다(텍스트 정상 출력).
> MinGW/Clang/GCC는 추가 플래그가 필요 없습니다.

---

## 2. 사용법

앱은 6개의 창을 띄웁니다(자유롭게 드래그 이동 가능):

| 창 | 역할 |
|---|---|
| **Simulation Control** | `Start`/`Pause`/`Reset`, **Speed** 슬라이더(1×–5×), **Scenario** 드롭다운, 실시간 틱 카운터와 잔고. |
| **Factory Floor** | 애니메이션 파이프라인. 머신은 상태별 색으로 칠해지고, 벨트 위 피자가 실시간으로 그려져 이동합니다. 노드(또는 아래 머신 목록의 행)를 클릭해 선택하면 진행/벨트 부하 바가 보입니다. |
| **Inspector** | 선택한 머신의 상세: 상태, 내구도 바, 진행 바, queue depth, output count, process time + **Force Break**/**Instant Repair**. |
| **Event Log** | 타임스탬프 스크롤 로그(출고/고장/시나리오 로드). **Clear** 버튼 + 자동 스크롤 토글. |
| **Statistics** | 누적 합계: 완성품, WIP, 고장, 손실, 수익. |
| **Orders** | 들어온 손님 주문(요구 사이즈·토핑), 보상, 남은 시간 카운트다운 바. |

**머신 상태 색:** 🟦 Idle · 🟩 Working · 🟥 Broken(점멸) · ⬛ Off.

**Start**를 누르고 반죽이 스네이크 파이프라인을 따라 흐르는 걸 지켜보세요.
머신에 **Force Break**를 걸면 라인이 밀리고 손실 카운터가 움직이는 걸 볼 수 있습니다.

---

## 3. 아키텍처 — UI ⇄ 백엔드 완전 분리

시뮬레이션 로직은 ImGui를 전혀 모르고, UI는 시뮬 객체를 전혀 만지지 않습니다.
둘은 오직 [`src/bridge.h`](src/bridge.h)의 **순수 값 구조체** 두 개로만 통신합니다:

```
   ┌────────────────────┐   FactorySnap  (읽기 전용 복사)   ┌─────────────────────┐
   │  DashboardView      │ ◄──────────────────────────────── │  Factory            │
   │  (ImGui, src/views) │                                   │  (sim, src/models)  │
   │  스냅샷을 그림       │   FactoryCmd   (한 프레임 플래그)  │  Machine* 등 소유    │
   └────────────────────┘ ────────────────────────────────► └─────────────────────┘
            ▲                                                          ▲
            └───────────────── FactoryController ──────────────────────┘
                        cmd → Factory 제어 메서드로 매핑
```

양쪽을 보는 유일한 파일은 [`src/app.cpp`](src/app.cpp). 매 프레임:

```cpp
FactorySnap snap = factory.snapshot();   // 1. 읽기 전용 스냅샷
view.Render(snap, cmd);                  // 2. 버튼이 cmd 플래그에 표시
controller.applyCmd(cmd);                // 3. cmd → factory.start()/forceBreak()/…
cmd = FactoryCmd{};                      // 4. 두 번 적용되지 않게 즉시 비움
controller.advance(dt);                  // 5. 시뮬 진행 (speed × 기본 틱레이트)
```

그래서 UI는 항상 **한 프레임 뒤** — `machine.state`가 아니라 `snap.state`를 읽습니다.
버튼 클릭은 쪽지(`cmd.forceBreak = true`)만 남기고, `app.cpp`가 적절한 시점에 백엔드로 전달합니다.

### 타입 계층 (시뮬 루프에 concrete 타입 분기 없음)

```
Machine (추상)
 ├─ NonConveyorMachine (추상) ── 피자 1개를 N틱 동안 가공
 │    └─ DoughStretcher · SauceSpreader · CheeseSpreader · ToppingApplier
 │       · Oven · Cutter · Packager
 └─ ConveyorMachine (추상) ───── 벨트 슬롯 위로 피자 운반
      └─ ConveyorBelt

Pizza (추상) ├─ RawDough (시작) └─ BoxedPizza (끝)
Scenario (추상) ├─ FreePlay ├─ NormalFlow └─ RandomBreakdown
```

`Factory::step()`은 `for (Machine* m : pipeline) m->update(tick);` — 순수 다형성.
새 머신은 `transform()` + `displayName()`을 가진 subclass 하나면 되고, 각 머신이 자기
`MachineSnap`을 채우므로 **시뮬 루프·UI 루프는 0줄도 바뀌지 않습니다.**
모든 클래스의 데이터 멤버는 `private`/`protected`입니다.

전체 설계·UML·ER 다이어그램은 [DESIGN.md](DESIGN.md) 참고.

---

## 4. 파이프라인

```
IN ▸ Dough Stretcher → [Conveyor] → Sauce → Cheese → Topping → Oven → Cutter → [Conveyor] → Packager ▸ OUT
```

| 머신 | 피자에 미치는 영향 |
|---|---|
| Dough Stretcher | dough → `STRETCHED`, 사이즈 설정 |
| Sauce Spreader | 소스 추가 |
| Cheese Spreader | 치즈 추가 |
| Topping Applier | 토핑 추가 |
| Oven | dough → `BAKED` |
| Cutter | 조각으로 자름 |
| Packager | `RawDough` → `BoxedPizza` (완성) |
| Conveyor | 머신 사이 이동(가공 없음) |

머신이 고장났거나 꽉 차면 자연스럽게 밀리고(백프레셔), 가득 찬/고장난 단계에서 떨어진
제품은 **손실(lost products)**로 집계됩니다.

### 시나리오 (실행 중 드롭다운)
- **Free Play** — 기본 게임 모드, 약한 고장 확률.
- **Normal flow** — 균형 잡힌 파이프라인, 고장 없음.
- **Random breakdowns** — 고장 확률 상향.

---

## 5. 과제 요구사항 → 구현 위치

| 요구사항 (factory_project_v3) | 구현 |
|---|---|
| 추상 루트 + 2단계 이상 상속 | `Machine` → `NonConveyor/Conveyor` → concrete |
| 시뮬 루프에 타입 분기 없음 | `Factory::step()`이 `Machine*` 순회 |
| 새 머신 = 루프/UI 0줄 수정 | 각 머신이 `displayName/icon/snapshot` 제공 |
| public 데이터 멤버 없음 | 전 클래스 `private`/`protected` |
| 추상 product, 시작/끝 단계 | `Pizza` → `RawDough` / `BoxedPizza` |
| UI/백엔드 분리, 단일 seam | `bridge.h` snapshot/cmd, `app.cpp`만 양쪽 봄 |
| 시나리오 드롭다운(다형성) | `Scenario` + 스냅샷의 `scenarioNames` |
| 필수 ImGui 창 5개 + 위젯 | Simulation Control / Factory Floor / Inspector / Event Log / Statistics (+ Orders) |

필수 ImGui 위젯 모두 포함: `Button`, `SliderInt`, `Combo`, `ProgressBar`,
`TextColored`, `BeginChild/EndChild`, `Selectable`.

---

## 6. 프로젝트 구조

```
Zzapi/
├── CMakeLists.txt          # 크로스플랫폼 빌드 (GLFW + ImGui 자동 fetch)
├── DESIGN.md               # 아키텍처, UML, ER 다이어그램
├── README.md / README.ko.md
├── scripts/
│   ├── build.sh / build.bat   # 빌드 편의 스크립트
│   └── sim_test.cpp           # 헤드리스 백엔드 드라이버 (ImGui 없음)
└── src/
    ├── bridge.h            # UI ⇄ 백엔드 계약 (POD: FactorySnap / FactoryCmd)
    ├── main.cpp            # GLFW + ImGui 보일러플레이트
    ├── app.{h,cpp}         # 양쪽을 보는 유일한 seam
    ├── models/             # 백엔드 (ImGui 무의존)
    │   ├── pizza.{h,cpp}  machine.{h,cpp}  factory.{h,cpp}
    │   └── order.{h,cpp}  scenario.{h,cpp}
    ├── controllers/
    │   └── factory_controller.{h,cpp}   # cmd → factory, 틱 cadence
    └── views/
        └── dashboard_view.{h,cpp}       # snapshot → ImGui (bridge.h만 의존)
```

### 헤드리스 백엔드 테스트
```bash
g++ -std=c++17 scripts/sim_test.cpp src/models/*.cpp -Isrc -o /tmp/simtest && /tmp/simtest
```
1200틱을 돌려 머신 상태·주문·이벤트 로그를 출력합니다 — GUI 없이 시뮬을 검증할 때 유용.

---

## 7. 참고 / 알려진 한계
- **폰트:** 번들 폰트는 라틴/한글 + BMP 기호(▶ ⏸ ↻ ⚠)를 커버합니다. 0x1F000+ 컬러 이모지
  (🍕, 🔥…)는 기본 ImGui 래스터라이저가 못 그려서 의도적으로 쓰지 않습니다.
- **밸런스:** 공장은 항상 **Medium** 피자를 생산하므로 주문도 Medium으로 생성되어 충족
  가능합니다(맞는 피자를 출고하면 가장 오래된 주문이 처리됨). 사이즈 가변 생산 + 사이즈
  기반 주문 매칭은 추후 게임플레이 확장 과제입니다.
