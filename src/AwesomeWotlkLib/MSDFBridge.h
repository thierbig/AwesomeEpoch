#pragma once
// Bridge to the isolated MSDF/D3D font-rendering static library (see msdf/). That library is
// compiled against upstream's client-abstraction headers, which conflict with this fork's own
// headers of the same name, so it must never share a translation unit with fork code. Only the
// primitive-typed entry points below cross the boundary; this header pulls in none of upstream's
// types, so fork code (e.g. Entry.cpp) can call into MSDF safely.

namespace D3D {
    // Installs the Direct3D9 device-level detours. Must be called inside an open Detour
    // transaction (the fork's attach sequence provides one).
    void initialize();
}

namespace MSDF {
    void initialize();
    // Applies an MSDFMode value (0 = disabled, 1 = enabled, 2 = enabled-unsafe). The fork owns
    // the "MSDFMode" CVar and forwards its parsed value here.
    void setMode(int mode);
}
