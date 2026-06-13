#include "belt_render.h"
#include <cmath>

namespace belt {

// ── 직선 컨베이어 벨트 (시간 기반 슬릿 애니메이션) ───────────────────────────
void DrawBelt(ImDrawList* dl, ImVec2 a, ImVec2 b, bool reverse) {
    dl->AddLine(a, b, IM_COL32(62, 67, 73, 255), 26.0f);
    ImVec2 dir(b.x - a.x, b.y - a.y);
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (len <= 0.0f) return;
    dir.x /= len; dir.y /= len;
    ImVec2 n(-dir.y, dir.x);
    dl->AddLine(ImVec2(a.x + n.x*13, a.y + n.y*13), ImVec2(b.x + n.x*13, b.y + n.y*13), IM_COL32(38,40,44,255), 2.0f);
    dl->AddLine(ImVec2(a.x - n.x*13, a.y - n.y*13), ImVec2(b.x - n.x*13, b.y - n.y*13), IM_COL32(38,40,44,255), 2.0f);
    float off = std::fmod((float)ImGui::GetTime() * 26.0f, 14.0f);
    if (reverse) off = 14.0f - off;
    for (float d = off; d < len; d += 14.0f) {
        ImVec2 m(a.x + dir.x*d, a.y + dir.y*d);
        dl->AddLine(ImVec2(m.x - n.x*9, m.y - n.y*9), ImVec2(m.x + n.x*9, m.y + n.y*9), IM_COL32(48,53,58,255), 2.0f);
    }
}

// ── 반원(semicircle) U-turn 컨베이어 벨트 ────────────────────────────────────
//    center c 를 중심으로 반지름 R, 각도 a0→a1 의 호 위에 두꺼운 벨트 밴드를 그린다.
void DrawBeltArc(ImDrawList* dl, ImVec2 c, float R, float a0, float a1, bool reverse) {
    const int SEG = 30;
    ImU32 band = IM_COL32(62, 67, 73, 255), edge = IM_COL32(38, 40, 44, 255);
    for (int i = 0; i < SEG; ++i) {
        float g0 = a0 + (a1 - a0) * (float)i / SEG;
        float g1 = a0 + (a1 - a0) * (float)(i + 1) / SEG;
        ImVec2 in0(c.x + (R-13)*std::cos(g0), c.y + (R-13)*std::sin(g0));
        ImVec2 ou0(c.x + (R+13)*std::cos(g0), c.y + (R+13)*std::sin(g0));
        ImVec2 in1(c.x + (R-13)*std::cos(g1), c.y + (R-13)*std::sin(g1));
        ImVec2 ou1(c.x + (R+13)*std::cos(g1), c.y + (R+13)*std::sin(g1));
        dl->AddQuadFilled(in0, ou0, ou1, in1, band);
        dl->AddLine(in0, in1, edge, 2.0f);
        dl->AddLine(ou0, ou1, edge, 2.0f);
    }
    float arcLen = std::fabs(a1 - a0) * R;
    float off = std::fmod((float)ImGui::GetTime() * 26.0f, 14.0f);
    if (reverse) off = 14.0f - off;
    for (float d = off; d < arcLen; d += 14.0f) {
        float g = a0 + (a1 - a0) * (d / arcLen);
        dl->AddLine(ImVec2(c.x + (R-9)*std::cos(g), c.y + (R-9)*std::sin(g)),
                    ImVec2(c.x + (R+9)*std::cos(g), c.y + (R+9)*std::sin(g)),
                    IM_COL32(48, 53, 58, 255), 2.0f);
    }
}

}  // namespace belt
