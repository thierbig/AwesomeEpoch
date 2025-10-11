-- File: Constants.lua
-- Holds all static definitions for the addon.

local addonName, ACVar = ...
local L = ACVar.L or {} -- Get the locale table loaded previously
_G["AwesomeEpoch"] = ACVar -- Public API table (no back-compat)

-- This table holds constants used throughout the addon.
ACVar.CONSTANTS = {
    ADDON_NAME = L.ADDON_NAME,
    COLORS = {
        SUCCESS = "|cff00ff00",
        HIGHLIGHT = "|cffffd100",
        VALUE = "|cff00ccff",
        ERROR = "|cffff0000",
        RESET = "|r",
        TAB_ACTIVE = {1, 1, 0},
        TAB_INACTIVE = {0.8, 0.8, 0.8},
        DESC_TEXT = {0.6, 0.6, 0.6}
    },
    FRAME = {
        MAIN_WIDTH = 768,
        MAIN_HEIGHT = 580,
        POPUP_WIDTH = 350,
        POPUP_HEIGHT = 120,
        BUTTON_WIDTH = 100,
        BUTTON_HEIGHT = 25,
        TAB_HEIGHT = 25
    }
}

-- This table holds TTS_VOICES that is populated at runtime.
ACVar.TTS_VOICES = {}

local function updateTts()
    wipe(ACVar.TTS_VOICES)
    for i, voiceInfo in pairs(C_VoiceChat and C_VoiceChat.GetTtsVoices() or {}) do
        ACVar.TTS_VOICES[voiceInfo.voiceID] = voiceInfo.name
    end
end

updateTts()
 
local TtsUpdateFrame = CreateFrame("Frame")
TtsUpdateFrame:RegisterEvent("VOICE_CHAT_TTS_VOICES_UPDATE")
TtsUpdateFrame:SetScript("OnEvent", updateTts)
 
-- This table defines every CVar control that will appear in the UI.
local function IsDllPresent()
    if type(_G.QueueInteract) == "function" then return true end
    if type(GetCVar) == "function" then
        local v = GetCVar("interactionMode")
        if v ~= nil and v ~= "" then return true end
    end
    if type(C_CVar) == "table" and type(C_CVar.GetCVar) == "function" then
        local v = C_CVar.GetCVar("interactionMode")
        if v ~= nil and v ~= "" then return true end
    end
    return false
end
local HAS_DLL = IsDllPresent()
 
local CVARS = {
    [L.CATEGORY_NAMEPLATES] = {
        { name = "nameplateStacking", label = L.CVAR_LABEL_ENABLE_STACKING, type = "toggle", min = 0, max = 1, persist = true },
        { name = "nameplateStackFriendly", label = L.CVAR_LABEL_STACK_FRIENDLY, type = "toggle", min = 0, max = 1, reloadRequired = false, persist = true },
        { name = "nameplateExtendWorldFrameHeight", label = L.CVAR_LABEL_EXTEND_WORLD_FRAME_HEIGHT, desc = L.DESC_EXTEND_WORLD_FRAME_HEIGHT, type = "toggle", min = 0, max = 1, persist = true },
        { name = "nameplateUpperBorderOnlyBoss", label = L.CVAR_LABEL_UPPER_BORDER_ONLY_BOSS, type = "toggle", min = 0, max = 1, persist = true },
        { name = "nameplateStackFriendlyMode", label = L.CVAR_LABEL_FRIENDLY_DETECT_MODE, type = "mode", modes = { {value = 0, label = L.MODE_LABEL_REACTION_API}, {value = 1, label = L.MODE_LABEL_COLOR_PARSING} }, reloadRequired = false, persist = true },
        { name = "nameplateDistance", label = L.CVAR_LABEL_NAMEPLATE_DISTANCE, type = "slider", min = 1, max = 200, step = 1, default = 40, persist = true },
        { name = "nameplateMaxRaiseDistance", label = L.CVAR_LABEL_MAX_RAISE_DISTANCE, type = "slider", min = 0, max = 500, step = 1, default = 200, persist = true },
        { name = "nameplateXSpace", label = L.CVAR_LABEL_X_SPACE, type = "slider", min = 5, max = 200, step = 1, default = 10, persist = true },
        { name = "nameplateYSpace", label = L.CVAR_LABEL_Y_SPACE, type = "slider", min = 5, max = 75, step = 1, default = 15, persist = true },
        { name = "nameplateUpperBorder", label = L.CVAR_LABEL_UPPER_BORDER, type = "slider", min = 0, max = 100, step = 1, default = 50, persist = true },
        { name = "nameplateOriginPos", label = L.CVAR_LABEL_ORIGIN_POS, type = "slider", min = 0, max = 60, step = 1, default = 30, persist = true },
        { name = "nameplateSpeedRaise", label = L.CVAR_LABEL_SPEED_RAISE, type = "slider", min = 0, max = 5, step = 0.1, default = 1.0, persist = true },
        { name = "nameplateSpeedReset", label = L.CVAR_LABEL_SPEED_RESET, type = "slider", min = 0, max = 5, step = 0.1, default = 1.0, persist = true },
        { name = "nameplateSpeedLower", label = L.CVAR_LABEL_SPEED_LOWER, type = "slider", min = 0, max = 5, step = 0.1, default = 1.0, persist = true },
        { name = "nameplateHitboxHeight", label = L.CVAR_LABEL_HITBOX_HEIGHT, desc = L.DESC_HITBOX_DISABLED, type = "slider", min = 0, max = 50, step = 1, default = 0, persist = true },
        { name = "nameplateHitboxWidth", label = L.CVAR_LABEL_HITBOX_WIDTH, desc = L.DESC_HITBOX_DISABLED, type = "slider", min = 0, max = 200, step = 1, default = 0, persist = true },
        { name = "nameplateFriendlyHitboxHeight", label = L.CVAR_LABEL_FRIENDLY_HITBOX_HEIGHT, desc = L.DESC_HITBOX_DISABLED, type = "slider", min = 0, max = 50, step = 1, default = 0, persist = true },
        { name = "nameplateFriendlyHitboxWidth", label = L.CVAR_LABEL_FRIENDLY_HITBOX_WIDTH, desc = L.DESC_HITBOX_DISABLED, type = "slider", min = 0, max = 200, step = 1, default = 0, persist = true },
    },
}

if HAS_DLL then
    CVARS[L.CATEGORY_INTERACTION] = {
        { name = "interactionMode", label = L.CVAR_LABEL_INTERACTION_MODE, type = "mode", modes = { {value = 0, label = L.MODE_LABEL_PLAYER_RADIUS}, {value = 1, label = L.MODE_LABEL_CONE_ANGLE} }, persist = true },
        { name = "interactionAngle", label = L.CVAR_LABEL_INTERACTION_ANGLE, type = "slider", min = 1, max = 360, step = 1, default = 90, persist = true },
    }
end

ACVar.CVARS = CVARS
