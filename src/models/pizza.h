#pragma once

#include <string>
#include <vector>

// =============================================================================
// Pizza 관련 Enum 정의
// =============================================================================

enum class DoughState { RAW, STRETCHED, BAKED };

enum class ToppingType { PEPPERONI, MUSHROOM, OLIVE, PEPPER, ONION, HAM };

enum class PizzaSize { SMALL, MEDIUM, LARGE };

// =============================================================================
// Pizza - 피자 객체
// 각 머신이 이 객체의 속성을 변화시킵니다.
// =============================================================================
class Pizza {
private:
  int id;
  DoughState doughState;
  PizzaSize size;

  bool hasSauce;

  bool hasCheese;

  std::vector<ToppingType> toppings;
  bool hasTopping;

  bool isBaked;      // 구워졌는지 여부
  int sliceCount;    // 0이면 아직 자르지 않음
  bool isCut;
  bool isPackaged;

public:
  // 생성자: 원재료 상태의 피자
  Pizza(int id);

  // --- Getters ---
  int getId() const;
  DoughState getDoughState() const;
  PizzaSize getSize() const;
  bool getHasSauce() const;
  bool getHasCheese() const;
  const std::vector<ToppingType>& getToppings() const;
  bool getHasTopping() const;
  bool getIsBaked() const;
  int getSliceCount() const;
  bool getIsCut() const;
  bool getIsPackaged() const;

  // --- Setters (머신들이 호출) ---
  void setDoughState(DoughState state);
  void setSize(PizzaSize s);
  void setSauce(bool v);
  void setCheese(bool v);
  void addTopping(ToppingType topping);
  void setHasTopping(bool v);
  void setIsBaked(bool v);
  void setSliceCount(int count);
  void setIsCut(bool v);
  void setPackaged(bool packaged);
};