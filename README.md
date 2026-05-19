# 🍕 PizzaFactory

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
```