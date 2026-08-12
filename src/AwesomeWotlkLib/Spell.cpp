#include "Spell.h"
#include "Hooks.h"
#include "GameClient.h"
#include <Detours/detours.h>

static Console::CVar* s_cvar_enableStancePatch;
static bool s_stancePatchEnabled = false;  // mirrors the CVar, set from the handler's newVal
static bool s_stancePatchAttached = false; // whether the detours are currently installed

// --- Shapeshift-form prediction -------------------------------------------------------------
// Writing UNIT_FIELD_BYTES_2 unblocks the *cast*, but it does not move what GetShapeshiftForm
// reports: that walks the client's shapeshift bar (0x0053D4B0) and, for each entry, asks the
// predicate at 0x00802CB0 whether the player matches -- and that predicate resolves the player's
// active shapeshift *aura* (player+0xF60, caster GUID via ObjectMgr), which only exists once the
// server confirms the stance. So `[stance:N]` still reports the old stance for the rest of the
// macro. GetShapeshiftForm has exactly two callers in the client: the Lua wrapper (0x0053DE10)
// and the [stance:]/[form:] macro-conditional handler (0x005EEFB0, which calls it at +7), so
// overriding it there fixes both at once.
//
// Bar layout, read straight out of GetShapeshiftForm: count at 0x00BE8E28, array base at
// 0x00BE8E2C, 8-byte entries whose first dword is the spell id (it feeds them to Spell.dbc,
// registry index 0x194 -- the same table GetSpellBaseCooldown uses). The function returns the
// 1-based index of the matching entry, which is exactly the N in `stance:N`.
static uint32_t* const s_shapeshiftBarCount = (uint32_t*)0x00BE8E28;
static uintptr_t* const s_shapeshiftBarArray = (uintptr_t*)0x00BE8E2C;

static int s_predictedFormIndex = 0; // 1-based bar index we forced; 0 = no prediction active
static DWORD s_predictedAtMs = 0;
static constexpr DWORD kPredictionTimeoutMs = 1500; // drop the lie if the server never confirms

static int ShapeshiftBarIndexForSpell(uint32_t spellId)
{
    const uint32_t count = *s_shapeshiftBarCount;
    const uintptr_t base = *s_shapeshiftBarArray;
    if (!base || count == 0 || count > 64) // 64: sanity bound, the real bar is <= 10
        return 0;

    for (uint32_t i = 1; i <= count; ++i) {
        if (*(uint32_t*)(base + (i - 1) * 8) == spellId)
            return (int)i;
    }
    return 0;
}

typedef int(__cdecl* GetShapeshiftFormFn)(int a1);
static GetShapeshiftFormFn GetShapeshiftForm_orig = (GetShapeshiftFormFn)0x0053D4B0;
static int __cdecl GetShapeshiftForm_hk(int a1)
{
    const int real = GetShapeshiftForm_orig(a1);
    if (s_predictedFormIndex == 0)
        return real;

    // Stand down as soon as the real aura agrees, or if it never showed up (failed/rejected cast).
    if (real == s_predictedFormIndex || (GetTickCount() - s_predictedAtMs) > kPredictionTimeoutMs) {
        s_predictedFormIndex = 0;
        return real;
    }
    return s_predictedFormIndex;
}

uintptr_t spellTablePtr = GetDbcTable(0x00000194);
static int lua_GetSpellBaseCooldown(lua_State* L) {
    uint8_t rowBuffer[680];
    uint32_t spellId = luaL_checknumber(L, 1);

    if (!GetLocalizedRow((void*)(spellTablePtr - 0x18), spellId, rowBuffer))
        return 0;

    SpellRec* spell = (SpellRec*)rowBuffer;
    uint32_t cdTime = spell->RecoveryTime ? spell->RecoveryTime : spell->CategoryRecoveryTime;
    uint32_t gcdTime = spell->StartRecoveryTime;

    if (cdTime == 0) {
        for (int i = 0; i < 3; i++) {
            if (spell->Effect[i] == 0)
                continue;

            uint32_t triggeredSpellId = spell->EffectTriggerSpell[i];
            if (triggeredSpellId == 0 || triggeredSpellId == spellId)
                continue;

            uint8_t rowBufferTrig[680];
            if (!GetLocalizedRow((void*)(spellTablePtr - 0x18), triggeredSpellId, rowBufferTrig))
                continue;

            SpellRec* spellTrig = (SpellRec*)rowBufferTrig;
            uint32_t trigCd = spellTrig->RecoveryTime ? spellTrig->RecoveryTime : spellTrig->CategoryRecoveryTime;
            uint32_t trigGcd = spellTrig->StartRecoveryTime;

            if (trigCd > cdTime)
                cdTime = trigCd;
            if (trigGcd > gcdTime)
                gcdTime = trigGcd;
        }
    }
    lua_pushnumber(L, cdTime);
    lua_pushnumber(L, gcdTime);
    return 2;
}

// Base 3.3.5a 12340 cast entry point; byte-identical to upstream noname08662 v37's Spell.cpp.
typedef int(__cdecl* SpellCastFn)(int a1, int a2, int a3, int a4, int a5);
static SpellCastFn Spell_OnCastOriginal = (SpellCastFn)0x0080DA40;
int __cdecl Spell_OnCastHook(int spellId, int a2, int a3, int a4, int a5)
{
    bool success = Spell_OnCastOriginal(spellId, a2, a3, a4, a5);
    if (success && s_stancePatchEnabled && Spell::IsForm(spellId))
    {
        CGUnit_C* player = ObjectMgr::GetCGUnitPlayer();
        if (player)
        {
            auto maybeForm = Spell::GetFormFromSpell(spellId);
            if (maybeForm.has_value())
            {
                Spell::ShapeshiftForm form = maybeForm.value();
                uint32_t formValue = static_cast<uint32_t>(form);

                player->SetValueBytes(UNIT_FIELD_BYTES_2, OFFSET_SHAPESHIFT_FORM, formValue);

                // Keep GetShapeshiftForm (and so [stance:N]) in step with the byte we just wrote,
                // until the server's aura catches up.
                s_predictedFormIndex = ShapeshiftBarIndexForSpell(spellId);
                s_predictedAtMs = GetTickCount();
            }
        }
    }

    return success;
}

// Keep the detours installed only while the patch is on, so `enableStancePatch 0` really does
// restore stock behaviour rather than just going quiet. The patch ships on, and initialize()
// attaches -- see the note there for why that is not left to this handler.
static int CVarHandler_enableStancePatch(Console::CVar*, const char*, const char* value, LPVOID)
{
    const bool wanted = (std::atoi(value) == 1);
    s_stancePatchEnabled = wanted;
    if (wanted == s_stancePatchAttached)
        return 1;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (wanted) {
        DetourAttach(&(LPVOID&)Spell_OnCastOriginal, Spell_OnCastHook);
        DetourAttach(&(LPVOID&)GetShapeshiftForm_orig, GetShapeshiftForm_hk);
    }
    else {
        DetourDetach(&(LPVOID&)Spell_OnCastOriginal, Spell_OnCastHook);
        DetourDetach(&(LPVOID&)GetShapeshiftForm_orig, GetShapeshiftForm_hk);
        s_predictedFormIndex = 0;
    }

    if (DetourTransactionCommit() == NO_ERROR)
        s_stancePatchAttached = wanted;
    else
        s_stancePatchEnabled = s_stancePatchAttached; // commit failed, keep the hook inert

    return 1;
}

static int lua_openmisclib(lua_State* L)
{
    lua_pushcfunction(L, lua_GetSpellBaseCooldown);
    lua_setglobal(L, "GetSpellBaseCooldown");
    return 0;
}

void Spell::initialize()
{
    Hooks::FrameXML::registerLuaLib(lua_openmisclib);
    Hooks::FrameXML::registerCVar(&s_cvar_enableStancePatch, "enableStancePatch", NULL, (Console::CVarFlags)1, "1", CVarHandler_enableStancePatch);

    // Ships enabled, so attach here instead of leaving it to CVarHandler_enableStancePatch: the
    // client does not reliably invoke a CVar's handler for its *initial* value, and depending on
    // that would leave enableStancePatch reading 1 with no detour installed -- the feature would
    // look on and do nothing until it was toggled. Registering while already attached is safe: the
    // handler's `wanted == s_stancePatchAttached` check makes a registration-time call a no-op, and
    // `enableStancePatch 0` still detaches both hooks properly.
    DetourAttach(&(LPVOID&)Spell_OnCastOriginal, Spell_OnCastHook);
    DetourAttach(&(LPVOID&)GetShapeshiftForm_orig, GetShapeshiftForm_hk);
    s_stancePatchEnabled = true;
    s_stancePatchAttached = true;
}