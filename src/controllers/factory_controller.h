#pragma once
// =============================================================================
// FactoryController — cmd → factory 제어 메서드 매핑 + 틱 cadence 결정
//
//   * 백엔드(Factory)만 안다. ImGui도 View도 모른다.
//   * 한 프레임 동안만 true 인 FactoryCmd를 읽어 Factory의 공개 제어 API를 호출.
//   * 시간 cadence(실시간 dt → 논리 틱)는 여기서 결정한다. Factory는 논리 틱만 안다.
//   * 머신 객체에 직접 접근하지 않는다. Factory 공개 메서드 + snapshot 만 사용.
// =============================================================================
#include "../bridge.h"

class Factory;

class FactoryController
{
public:
    void Init(Factory* factory);

    /// cmd(한 프레임 명령)를 Factory 제어 메서드로 매핑
    void applyCmd(const FactoryCmd& cmd);

    /// 실시간 경과(dt)를 받아 running 이면 speed에 비례해 step() 진행
    void advance(float dt);

private:
    Factory* m_factory = nullptr;
    float    m_acc     = 0.0f;   // 틱 누적기
    int      m_speed   = 1;      // 1..5 (cmd로 갱신)
};
