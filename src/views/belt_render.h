#pragma once
// =============================================================================
// belt_render — Factory Floor conveyor-belt drawing primitives
//
//   * Draws only the "belt shape" with a time-based slit (hatch) animation.
//   * Placement (snake layout), pizza positions, and breakdown tint are decided by the caller (DashboardView).
//   * Depends only on imgui — it doesn't even see bridge.h (pure geometry + ImDrawList).
// =============================================================================
#include "imgui.h"

namespace belt {

// Straight conveyor belt (a -> b). reverse=true flips the slit-animation direction.
void DrawBelt(ImDrawList* dl, ImVec2 a, ImVec2 b, bool reverse);

// Semicircle (U-turn) belt: centered at c, radius R, a band + slits along the arc from angle a0 -> a1.
void DrawBeltArc(ImDrawList* dl, ImVec2 c, float R, float a0, float a1, bool reverse);

}  // namespace belt
