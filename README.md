# 🍕 Zzapi — 피자 공장 시뮬레이터

ImGui 기반의 피자 공장 시뮬레이션 게임입니다.
플레이어는 컨베이어 벨트 위의 기계들을 켜고 끄면서, 들어오는 주문에 맞는 피자를 만들어 납품합니다.

---

## 빌드 & 실행

### 필요 도구

| 플랫폼 | 필요 사항 |
|--------|----------|
| **macOS** | Xcode Command Line Tools, CMake 3.20+ |
| **Windows** | Visual Studio 2019+ (C++ 워크로드), CMake 3.20+ |

> 📦 GLFW와 ImGui는 CMake가 자동으로 다운로드합니다. 별도 설치 불필요!

### macOS / Linux
```bash
chmod +x scripts/build.sh
./scripts/build.sh          # Debug 빌드
./scripts/build.sh Release  # Release 빌드
./build/PizzaFactory        # 실행
```

### Windows
```cmd
scripts\build.bat           # Debug 빌드
scripts\build.bat Release   # Release 빌드
build\Debug\PizzaFactory.exe  # 실행
```

### 직접 CMake 사용
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

---

## 아키텍처 개요

프로젝트는 **MVC (Model-View-Controller)** 패턴을 따릅니다.

```
┌──────────────────────────────────────────────────────────┐
│  App  (src/app.h, app.cpp)                               │
│  ┌─────────────┐ ┌──────────────────┐ ┌───────────────┐  │
│  │    Model     │ │   Controller     │ │     View      │  │
│  │ (데이터)     │◄│ (비즈니스 로직)   │ │ (ImGui 렌더링)│  │
│  └─────────────┘ └──────────────────┘ └───────────────┘  │
└──────────────────────────────────────────────────────────┘
```

### 실행 흐름

`App` 클래스가 MVC 세 컴포넌트를 소유하며, `main.cpp`의 ImGui 렌더 루프에서 매 프레임 아래 순서로 호출됩니다:

1. **`App::Init()`** — Model, Controller, View를 생성하고 서로 연결
2. **매 프레임 `App::Update()`** →
   - `FactoryController::Update()` — 게임 로직 (파이프라인 처리, 주문 관리)
   - `DashboardView::Render()` — ImGui UI 렌더링

---

## 핵심 모델 (Models)

### Pizza (`src/models/pizza.h`)

파이프라인을 따라 이동하며 기계에 의해 속성이 변화하는 피자 객체입니다.

| 속성 | 타입 | 설명 |
|------|------|------|
| `id` | `int` | 고유 식별자 |
| `doughState` | `DoughState` | 반죽 상태 (`RAW` → `STRETCHED` → `BAKED`) |
| `size` | `PizzaSize` | 크기 (`SMALL`, `MEDIUM`, `LARGE`) |
| `hasSauce` | `bool` | 소스가 발라졌는지 |
| `hasCheese` | `bool` | 치즈가 올라갔는지 |
| `isBaked` | `bool` | 오븐에서 구워졌는지 |
| `isCut` | `bool` | 커팅되었는지 |
| `isPackaged` | `bool` | 포장되었는지 |

### Machine (`src/models/machines.h`)

모든 기계의 추상 베이스 클래스입니다. 두 가지 계열로 나뉩니다:

| 계열 | 설명 | 예시 |
|------|------|------|
| **NonConveyorMachine** | 한 사이클에 `capacity`개의 피자를 일괄 처리하는 고정형 기계 | DoughStretcher, Oven 등 |
| **ConveyorMachine** | `length`개의 슬롯을 가진 벨트 위에서 피자를 이동시키는 기계 | ConveyorBelt |

#### 구현된 기계 종류

| 클래스 | 역할 | 피자에 미치는 영향 |
|--------|------|-------------------|
| `DoughStretcher` | 반죽 늘리기 | `doughState` → `STRETCHED`, `size` 설정 |
| `SauceSpreader` | 소스 바르기 | `hasSauce` → `true` |
| `CheeseSpreader` | 치즈 뿌리기 | `hasCheese` → `true` |
| `ToppingApplier` | 토핑 올리기 | `hasTopping` → `true` |
| `Oven` | 굽기 | `doughState` → `BAKED`, `isBaked` → `true` |
| `Cutter` | 자르기 | `isCut` → `true` |
| `PackagingMachine` | 포장 | `isPackaged` → `true` |
| `ConveyorBelt` | 기계 사이 이동 | 속성 변경 없음 (이동만) |

#### 기계 공통 기능

- **전원 제어 (`isPoweredOn`)**: 기계의 전원을 끄면 `process()` 호출 시 아무 작업도 하지 않습니다. 피자는 기계를 **그대로 통과**하지만 속성이 변경되지 않습니다. 이를 활용해 "소스 없는 피자", "굽지 않은 피자" 등 다양한 조합을 만들 수 있습니다.
- **내구도 (`durability`)**: 기계에 용량의 85% 이상 피자가 차면 내구도가 감소합니다. 내구도가 0이 되면 기계가 고장(`isBroken = true`)나며, 일정 프레임(`repairTime`) 후 자동 수리됩니다.

### Order & OrderManager (`src/models/order.h`)

주문 시스템입니다. 하나의 파일에 `Order`와 `OrderManager` 두 클래스가 정의되어 있습니다.

**Order** — 개별 주문을 나타냅니다.

| 속성 | 설명 |
|------|------|
| `requiredSize` | 요구하는 피자 크기 |
| `requiresSauce` | 소스 필요 여부 |
| `requiresCheese` | 치즈 필요 여부 |
| `requiresBake` | 굽기 필요 여부 |
| `timeLeft` | 남은 제한 시간 (프레임 단위) |
| `reward` | 완료 시 보상 |

**OrderManager** — 주문의 생성, 추적, 검증을 담당합니다.
- `tick()`: 일정 주기(600프레임 ≈ 10초)마다 랜덤 조건의 주문을 생성하고, 기존 주문들의 타이머를 감소시킵니다. 시간 초과 시 주문은 `FAILED` 처리됩니다.
- `verifyPizza(Pizza*)`: 완성된 피자가 대기 중인 주문 조건과 일치하는지 검사합니다.

### PizzaFactoryModel (`src/models/pizza_factory_model.h`)

앱의 전체 상태를 보유하는 최상위 모델입니다.

| 속성 | 설명 |
|------|------|
| `pipeline` | 기계들의 순서 배열 (기본 9대) |
| `orderManager` | 주문 관리자 |
| `finishedPizzas` | 주문 매칭에 실패하여 쌓인 피자 |
| `lostPizzas` | 기계 고장/용량 초과로 유실된 피자 |
| `isSpawningEnabled` | 반죽 투입 메인 스위치 |

> 캡슐화 원칙을 따라, 모든 데이터 필드는 `private`이며 Getter/Setter 메서드를 통해 접근합니다.

---

## 파이프라인 동작 원리

### 기본 파이프라인 구성

`PizzaFactoryModel::InitDefaultPipeline()`에서 아래 순서로 기계들이 배치됩니다:

```
[반죽기] → [벨트] → [소스] → [벨트] → [치즈] → [벨트] → [오븐] → [벨트] → [포장기]
   1         2        3        4        5        6        7        8        9
```

### 프레임당 처리 흐름 (`FactoryController::Update()`)

매 프레임마다 아래 순서로 실행됩니다:

#### Step 1 — 주문 매니저 업데이트
```
OrderManager::tick()
  → 600프레임마다 새 주문 생성
  → 기존 주문들의 타이머 감소 (시간 초과 시 FAILED)
```

#### Step 2 — 피자 반죽 투입
```
isSpawningEnabled == true && 60프레임 주기마다:
  → 새 Pizza 객체 생성 (id 자동 부여)
  → 파이프라인 첫 번째 기계(반죽기)에 투입 시도
  → 실패 시 (기계 고장 or 꽉 참) → lostPizzas로 이동
```

#### Step 3 — 파이프라인 역순 업데이트

**파이프라인을 뒤에서 앞으로 (index 8 → 0) 순회**합니다.
역순으로 처리하는 이유는 한 프레임에 피자가 여러 기계를 연속 통과하는 것을 방지하기 위해서입니다.

각 기계에 대해:

```
1. machine->tick()          — 수리 타이머 등 상태 업데이트
2. if (고장) → skip          — 고장난 기계는 아무 것도 하지 않음
3. machine->process()       — 피자 속성 변경 (전원 꺼져 있으면 건너뜀)
4. if (배출할 피자가 있으면):
   ├── 마지막 기계인 경우:
   │   ├── OrderManager.verifyPizza() → 주문 매칭 성공 시 → 주문 완료, 피자 소멸
   │   └── 매칭 실패 → lostPizzas로 폐기
   └── 중간 기계인 경우:
       ├── 다음 기계에 insert 성공 → 피자 이동
       └── 다음 기계 고장 or 꽉 참 → lostPizzas로 유실
```

### 시각화 예시

아래는 3프레임에 걸쳐 피자 하나가 이동하는 모습입니다:

```
Frame 1:  [Pizza A] → [  빈  ] → [  빈  ] → ...
Frame 2:  [  빈  ] → [Pizza A] → [  빈  ] → ...
Frame 3:  [  빈  ] → [  빈  ] → [Pizza A] → ...
```

---

## 프로젝트 구조

```
Zzapi/
├── CMakeLists.txt                    # 빌드 설정 (크로스플랫폼)
├── README.md
├── .gitignore
├── scripts/
│   ├── build.sh                      # Mac/Linux 빌드 스크립트
│   └── build.bat                     # Windows 빌드 스크립트
└── src/
    ├── main.cpp                      # GLFW/ImGui 초기화 및 렌더 루프
    ├── app.h / app.cpp               # MVC 컴포넌트 관리 (최상위)
    ├── models/
    │   ├── pizza.h / pizza.cpp               # Pizza 객체
    │   ├── machines.h / machines.cpp         # Machine 계층 구조
    │   ├── order.h / order.cpp               # Order + OrderManager
    │   └── pizza_factory_model.h / .cpp      # 전체 상태 모델
    ├── controllers/
    │   └── factory_controller.h / .cpp       # 게임 로직 (파이프라인 처리)
    └── views/
        └── dashboard_view.h / .cpp           # ImGui 대시보드 UI (TODO)
```

---

## 구현 현황

| 영역 | 상태 | 설명 |
|------|------|------|
| Pizza 모델 | ✅ 완료 | 사이즈(S/M/L), 소스·치즈·굽기 등 bool 속성 |
| Machine 계층 구조 | ✅ 완료 | 추상 클래스 + 7종 구체 기계 + 전원 제어 |
| 파이프라인 처리 로직 | ✅ 완료 | 역순 순회, 고장/용량 초과 시 Loss 처리 |
| 주문 시스템 | ✅ 완료 | 자동 생성, 타이머, 납품 검증 |
| 팩토리 스폰 토글 | ✅ 완료 | 반죽 투입 시작/정지 제어 |
| 캡슐화 | ✅ 완료 | PizzaFactoryModel 데이터 은닉 |
| ImGui 대시보드 UI | 🚧 미구현 | 주문 표시, 기계 전원 버튼 등 UI 작업 필요 |
