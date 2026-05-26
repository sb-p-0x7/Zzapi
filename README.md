# 🍕 Zzapi

ImGui 기반 피자 공장 시뮬레이터

## 필요 도구

| 플랫폼 | 필요 사항 |
|--------|----------|
| **macOS** | Xcode Command Line Tools, CMake 3.20+ |
| **Windows** | Visual Studio 2019+ (C++ 워크로드), CMake 3.20+ |

> 📦 GLFW와 ImGui는 CMake가 자동으로 다운로드합니다. 별도 설치 불필요!

## 빌드 방법

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

## 프로젝트 구조
```
Zzapi/
├── CMakeLists.txt          # 빌드 설정 (크로스플랫폼)
├── README.md
├── .gitignore
├── scripts/
│   ├── build.sh            # Mac/Linux 빌드 스크립트
│   └── build.bat           # Windows 빌드 스크립트
└── src/
    └── main.cpp            # 메인 애플리케이션

## 현재 구현된 핵심 기능 (Core Features)

현재 피자 공장 시뮬레이션의 백엔드(데이터 모델 및 제어 로직)가 다음과 같이 구축되어 있습니다.

### 1. 피자 객체 (Pizza)
- 피자의 상태(크기 `size`, 소스 유무 `hasSauce`, 치즈 유무 `hasCheese`, 구워짐 여부 `isBaked` 등)를 bool과 enum 형태로 직관적으로 추적합니다.
- `enum class PizzaSize` (SMALL, MEDIUM, LARGE)로 사이즈를 구분합니다.

### 2. 머신 파이프라인 (Machines & Factory Pipeline)
- 공장은 여러 기계를 거치는 일련의 파이프라인(컨베이어 벨트) 구조로 설계되어 있습니다.
- **기계 종류**: 반죽기(`DoughStretcher`), 소스 도포기(`SauceSpreader`), 치즈 도포기(`CheeseSpreader`), 오븐(`Oven`), 커터, 포장기 등이 존재합니다.
- **기계 전원 제어 (`isPoweredOn`)**: 각 기계는 전원 스위치를 가집니다. 사용자가 기계의 전원을 끄면 피자 반죽이 해당 기계에 들어가도 작업을 건너뛰고 그대로 통과합니다. 이를 활용해 '치즈 없는 피자', '소스 없는 피자' 등을 만들 수 있습니다.

### 3. 주문 관리 시스템 (Order & OrderManager)
- **주문 자동 생성**: 일정 주기마다 무작위 조건(요구 사이즈, 소스/치즈 유무, 굽기 등)을 가진 피자 주문(`Order`)이 생성됩니다.
- **타이머 시스템**: 각 주문에는 제한 시간이 있으며, 시간이 초과되면 주문이 실패(`FAILED`) 처리됩니다.
- **납품 검증**: 컨베이어 벨트 끝에 도달한 피자는 `OrderManager`에 의해 대기 중인 주문 조건과 비교됩니다. 조건이 일치하면 주문이 완료(`COMPLETED`)되어 보상을 얻고, 일치하지 않는 피자는 폐기(`Loss`) 처리됩니다.

### 4. 팩토리 메인 컨트롤러 (FactoryController)
- 프레임마다 파이프라인 배열을 역순으로 순회하여, 한 프레임에 피자가 여러 기계를 통과하는 것을 방지하고 병목 현상 및 기계 고장 등을 관리합니다.
- **메인 스폰 스위치 (`isSpawningEnabled`)**: 불필요한 반죽 낭비를 막기 위해, 유저가 필요할 때만 피자 반죽 투입을 시작/정지할 수 있습니다.

```
