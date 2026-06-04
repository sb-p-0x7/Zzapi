#pragma once

#include "pizza.h"
#include <vector>
#include <string>

// =============================================================================
// Machine (추상 베이스 클래스)
// 모든 머신의 공통 인터페이스와 속성을 정의합니다.
// =============================================================================
class Machine {
protected:
  std::string name;
  float speed;       // 처리 속도
  float durability;  // 내구도
  float maxDurability;
  bool isBroken;
  bool isPoweredOn;
  int repairTime;
  int currentRepairTimer;

public:
  Machine(const std::string& name, float speed, float durability)
      : name(name), speed(speed), durability(durability),
        maxDurability(durability), isBroken(false), isPoweredOn(true),
        repairTime(180), currentRepairTimer(0) {} // 기본 180프레임(약 3초) 수리

  virtual ~Machine() = default;

  // 핵심 파이프라인 인터페이스
  virtual bool canInsert() const = 0;           // 투입 가능 여부 확인
  virtual bool insertPizza(Pizza* pizza) = 0;   // 피자 투입
  virtual void process() = 0;                   // 처리 수행
  virtual bool hasPizzaToEject() const = 0;     // 배출 가능 여부 확인
  virtual Pizza* ejectPizza() = 0;              // 피자 배출

  // 매 프레임 호출되는 로직 (예: 수리 타이머)
  virtual void tick();

  // Getters
  std::string getName() const { return name; }
  float getSpeed() const { return speed; }
  float getDurability() const { return durability; }
  float getMaxDurability() const { return maxDurability; }
  bool getIsBroken() const { return isBroken; }
  bool getIsPoweredOn() const { return isPoweredOn; }
  void setPower(bool on) { isPoweredOn = on; }
  void togglePower() { isPoweredOn = !isPoweredOn; }
  void setSpeed(float s) { speed = s; }
  int getRepairTime() const { return repairTime; }
  int getCurrentRepairTimer() const { return currentRepairTimer; }
  void forceBreak();
  void instantRepair();
  virtual void resetState();

  void decreaseDurability(float amount = 1.0f);
};

// =============================================================================
// NonConveyorMachine (추상)
// 고정형 머신: 한 사이클에 capacity만큼의 피자를 처리합니다.
// =============================================================================
class NonConveyorMachine : public Machine {
protected:
  int capacity;                        // 한 사이클에 처리 가능한 피자 수
  std::vector<Pizza*> pizzasInProcess; // 현재 처리 중인 피자들
  int m_processTimer   = 0;            // 현재 처리된 프레임 수
  int m_requiredFrames = 0;            // 배출까지 필요한 프레임 수 (60 / speed)

public:
  NonConveyorMachine(const std::string& name, float speed, float durability,
                     int capacity)
      : Machine(name, speed, durability), capacity(capacity) {}

  int getCapacity() const { return capacity; }
  void setCapacity(int c) { capacity = c; }
  int getProcessTimer()   const { return m_processTimer; }
  int getRequiredFrames() const { return m_requiredFrames; }
  const std::vector<Pizza*>& getPizzasInProcess() const { return pizzasInProcess; }
  void resetState() override;

  bool canInsert() const override;
  bool insertPizza(Pizza* pizza) override;
  bool hasPizzaToEject() const override;
  Pizza* ejectPizza() override;

  // process()는 각 서브클래스에서 구현
};

// =============================================================================
// ConveyorMachine (추상)
// 컨베이어 벨트 기반 머신: length(슬롯 수)와 moveSpeed를 가집니다.
// =============================================================================
class ConveyorMachine : public Machine {
protected:
  int length;                   // 벨트 길이 (슬롯 수)
  float moveSpeed;              // 벨트 이동 속도
  std::vector<Pizza*> belt;     // 벨트 위의 피자들 (nullptr = 빈 슬롯)

public:
  ConveyorMachine(const std::string& name, float speed, float durability,
                  int length, float moveSpeed)
      : Machine(name, speed, durability), length(length),
        moveSpeed(moveSpeed), belt(length, nullptr) {}

  int getLength() const { return length; }
  float getMoveSpeed() const { return moveSpeed; }
  const std::vector<Pizza*>& getBelt() const { return belt; }
  void resetState() override;

  bool canInsert() const override;
  bool insertPizza(Pizza* pizza) override;
  Pizza* advance();
  bool hasPizzaToEject() const override;
  Pizza* ejectPizza() override;

  // process()는 각 서브클래스에서 구현
};

// =============================================================================
// 구체적인 ConveyorMachine 서브클래스
// =============================================================================

class ConveyorBelt : public ConveyorMachine {
  int m_tickCounter = 0;
public:
  ConveyorBelt(float speed = 1.0f, float durability = 100.0f,
               int length = 3, float moveSpeed = 1.0f)
      : ConveyorMachine("ConveyorBelt", speed, durability, length, moveSpeed) {}

  void process() override;
  void resetState() override { ConveyorMachine::resetState(); m_tickCounter = 0; }
};

// =============================================================================
// 구체적인 NonConveyorMachine 서브클래스들
// =============================================================================

// 도우 스트레쳐: 반죽을 늘려서 피자 도우를 만듦
class DoughStretcher : public NonConveyorMachine {
private:
  PizzaSize targetSize;

public:
  DoughStretcher(float speed = 1.0f, float durability = 100.0f,
                 int capacity = 1, PizzaSize targetSize = PizzaSize::MEDIUM)
      : NonConveyorMachine("DoughStretcher", speed, durability, capacity),
        targetSize(targetSize) {}

  void setTargetSize(PizzaSize size) { targetSize = size; }
  PizzaSize getTargetSize() const { return targetSize; }

  void process() override;
};

// 소스 스프레더: 소스를 바름
class SauceSpreader : public NonConveyorMachine {
public:
  SauceSpreader(float speed = 1.0f, float durability = 100.0f,
                int capacity = 1)
      : NonConveyorMachine("SauceSpreader", speed, durability, capacity) {}

  void process() override;
};

// 치즈 스프레더: 치즈를 뿌림
class CheeseSpreader : public NonConveyorMachine {
public:
  CheeseSpreader(float speed = 1.0f, float durability = 100.0f,
                 int capacity = 1)
      : NonConveyorMachine("CheeseSpreader", speed, durability, capacity) {}

  void process() override;
};

// 토핑 어플라이어: 토핑을 올림
class ToppingApplier : public NonConveyorMachine {
private:
  std::vector<ToppingType> availableToppings;

public:
  ToppingApplier(float speed = 1.0f, float durability = 100.0f,
                 int capacity = 1,
                 const std::vector<ToppingType>& toppings = {})
      : NonConveyorMachine("ToppingApplier", speed, durability, capacity),
        availableToppings(toppings) {}

  void addAvailableTopping(ToppingType t) { availableToppings.push_back(t); }
  const std::vector<ToppingType>& getAvailableToppings() const { return availableToppings; }

  void process() override;
};

// 오븐: 피자를 구움
class Oven : public NonConveyorMachine {
private:
  float temperature;
  float bakeTime;

public:
  Oven(float speed = 1.0f, float durability = 100.0f, int capacity = 1,
       float temperature = 250.0f, float bakeTime = 10.0f)
      : NonConveyorMachine("Oven", speed, durability, capacity),
        temperature(temperature), bakeTime(bakeTime) {}

  void setTemperature(float temp) { temperature = temp; }
  float getTemperature() const { return temperature; }

  void process() override;
};

// 커터: 피자를 자름
class Cutter : public NonConveyorMachine {
private:
  int sliceCount;

public:
  Cutter(float speed = 1.0f, float durability = 100.0f, int capacity = 1,
         int sliceCount = 8)
      : NonConveyorMachine("Cutter", speed, durability, capacity),
        sliceCount(sliceCount) {}

  void setSliceCount(int count) { sliceCount = count; }
  int getSliceCount() const { return sliceCount; }

  void process() override;
};

// 패키지 머신: 피자를 포장함
class PackagingMachine : public NonConveyorMachine {
private:
  std::string boxType;

public:
  PackagingMachine(float speed = 1.0f, float durability = 100.0f,
                   int capacity = 1, const std::string& boxType = "standard")
      : NonConveyorMachine("PackagingMachine", speed, durability, capacity),
        boxType(boxType) {}

  void process() override;
};