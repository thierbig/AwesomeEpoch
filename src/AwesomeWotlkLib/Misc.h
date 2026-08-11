#pragma once

namespace Misc {
void initialize();
// Registers the per-session chat/combat log CVars. Called directly from the attach sequence
// (the fork's Misc::initialize() is currently disabled), so these register even though the
// other Misc CVars do not.
void registerLogSessionCVars();
}