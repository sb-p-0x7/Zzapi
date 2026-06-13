#pragma once
// =============================================================================
// belt_render — Factory Floor 컨베이어 벨트 드로잉 프리미티브
//
//   * 시간 기반 슬릿(빗금) 애니메이션을 가진 "벨트 모양"만 그린다.
//   * 배치(스네이크 레이아웃)·피자 위치·고장 틴트는 호출측(DashboardView)이 결정.
//   * 의존성은 imgui 뿐 — bridge.h 도 보지 않는다 (순수 기하 + ImDrawList).
// =============================================================================
#include "imgui.h"

namespace belt {

// 직선 컨베이어 벨트 (a → b). reverse=true 면 슬릿 애니메이션 방향을 반전.
void DrawBelt(ImDrawList* dl, ImVec2 a, ImVec2 b, bool reverse);

// 반원(U-turn) 벨트: center c 중심, 반지름 R, 각도 a0 → a1 호 위에 밴드 + 슬릿.
void DrawBeltArc(ImDrawList* dl, ImVec2 c, float R, float a0, float a1, bool reverse);

}  // namespace belt
