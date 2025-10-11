-- File: Core.lua
-- Main addon logic, event handling, and slash command processing.

local addonName, ACVar = ...
local L = ACVar.L or {}
local CONSTANTS = ACVar.CONSTANTS

local _G = _G
local tonumber = tonumber
local tostring = tostring
local lower = string.lower
local format = string.format
local trim = string.trim

local GetCVar = GetCVar
local SetCVar = SetCVar
local CreateFrame = CreateFrame
local SlashCmdList = SlashCmdList

local InCombatLockdown = InCombatLockdown

ACVar.reloadIsPending = false

-- Queue for CVar sets attempted before the CVar is registered by the client/injector
ACVar._pendingCVarSets = {}
ACVar._pendingLookup = {}

-- SavedVariables helpers (persist selected CVars across sessions)
function ACVar:_ensureSavedTable()
    if type(_G.AwesomeEpochDB) ~= "table" then
        _G.AwesomeEpochDB = {}
    end
end

function ACVar:_persistCVar(cvarName, value, cvarDef)
    if cvarDef and cvarDef.persist then
        self:_ensureSavedTable()
        _G.AwesomeEpochDB[cvarName] = value
    end
end

function ACVar:ApplyPersistedCVars()
    self:_ensureSavedTable()
    local CVARS = self.CVARS or {}
    for _, cvarList in pairs(CVARS) do
        for _, cvarDef in ipairs(cvarList) do
            if cvarDef.persist and _G.AwesomeEpochDB[cvarDef.name] ~= nil then
                -- Use normal setter; it will defer if needed and sync UI later
                self:SetCVarValue(cvarDef.name, _G.AwesomeEpochDB[cvarDef.name], cvarDef)
            end
        end
    end
end

-- Lightweight ensure loop: every 0.25s, try to apply pending CVar sets if they now exist
ACVar._ensureFrame = CreateFrame("Frame")
ACVar._ensureElapsed = 0
-- UI resync window to keep controls in sync while CVars appear after reload
ACVar._uiResyncTimeLeft = 5.0   -- seconds
ACVar._uiResyncTick = 0.0       -- accumulator for periodic resync
ACVar._ensureFrame:SetScript("OnUpdate", function(_, elapsed)
    ACVar._ensureElapsed = (ACVar._ensureElapsed or 0) + elapsed
    if ACVar._ensureElapsed < 0.25 then return end
    ACVar._ensureElapsed = 0

    for i = #ACVar._pendingCVarSets, 1, -1 do
        local entry = ACVar._pendingCVarSets[i]
        if GetCVar(entry.name) ~= nil then
            pcall(SetCVar, entry.name, entry.value)
            -- Persist after apply if this CVar is configured to be saved
            ACVar:_persistCVar(entry.name, entry.value, entry.cvarDef)
            if entry.cvarDef and entry.cvarDef.reloadRequired then
                ACVar.reloadIsPending = true
            end
            ACVar._pendingLookup[entry.name] = nil
            table.remove(ACVar._pendingCVarSets, i)
            ACVar:PrintCVarChange(entry.name, entry.value)
            -- If the UI is initialized, refresh the specific control to reflect the applied value
            if ACVar.Frame and ACVar.UpdateUIForCVar and entry.cvarDef and not InCombatLockdown() then
                ACVar:UpdateUIForCVar(entry.cvarDef)
            end
        end
    end

    -- UI resync moved to event-based flow (PLAYER_REGEN_ENABLED). No periodic resync here.
end)

-- Compute effective toggle value with fallbacks when CVars are not yet available
function ACVar:_effectiveToggleValue(cvarDef)
    local val = self:GetCVarValue(cvarDef.name)
    if val == nil and self._pendingLookup and self._pendingLookup[cvarDef.name] then
        val = self._pendingLookup[cvarDef.name].value
    end
    if val == nil then
        val = (cvarDef.default ~= nil) and cvarDef.default or cvarDef.min
    end
    return val
end

-- Synchronize all toggle checkboxes with current/effective values
function ACVar:_syncToggleCheckboxes()
    local CVARS = self.CVARS or {}
    local addonPrefix = (self.CONSTANTS and self.CONSTANTS.ADDON_NAME) or "AwesomeEpoch"
    for _, cvarList in pairs(CVARS) do
        for _, cvarDef in ipairs(cvarList) do
            if cvarDef.type == "toggle" then
                local frameName = addonPrefix .. "_" .. cvarDef.name .. "Checkbox"
                local checkbox = _G[frameName]
                if checkbox then
                    local eff = self:_effectiveToggleValue(cvarDef)
                    local numEff, numMax = tonumber(eff), tonumber(cvarDef.max)
                    local checked = (numEff ~= nil and numMax ~= nil) and (numEff == numMax) or tostring(eff) == tostring(cvarDef.max)
                    checkbox:SetChecked(checked)
                end
            end
        end
    end
end

-- Patch UpdateUIForCVar to handle toggle numeric/string normalization and late CVars
function ACVar:_patchUpdateUIForCVar()
    if self._updateUiPatched or type(self.UpdateUIForCVar) ~= "function" then return end
    local orig = self.UpdateUIForCVar
    self.UpdateUIForCVar = function(selfObj, cvarDef)
        -- Update base control first
        orig(selfObj, cvarDef)
        -- Normalize toggle checkbox state based on effective value
        if cvarDef and cvarDef.type == "toggle" then
            local addonPrefix = (selfObj.CONSTANTS and selfObj.CONSTANTS.ADDON_NAME) or "AwesomeEpoch"
            local frameName = addonPrefix .. "_" .. cvarDef.name .. "Checkbox"
            local checkbox = _G[frameName]
            if checkbox then
                local eff = selfObj:_effectiveToggleValue(cvarDef)
                local numEff, numMax = tonumber(eff), tonumber(cvarDef.max)
                local checked = (numEff ~= nil and numMax ~= nil) and (numEff == numMax) or tostring(eff) == tostring(cvarDef.max)
                checkbox:SetChecked(checked)
            end
        end
    end
    self._updateUiPatched = true
end

-- Wrap ShowFrame to force a checkbox sync immediately after the UI updates
function ACVar:_ensureShowFrameWrap()
    if self._showFrameWrapped or type(self.ShowFrame) ~= "function" then return end
    local orig = self.ShowFrame
    self.ShowFrame = function(selfObj, tabName)
        if InCombatLockdown and InCombatLockdown() then
            selfObj._showAfterCombat = true
            return
        end
        orig(selfObj, tabName)
        if selfObj._syncToggleCheckboxes then
            selfObj:_syncToggleCheckboxes()
        end
    end
    self._showFrameWrapped = true
end

-- ToggleFrame wrapper disabled: UI.lua already defers in combat; wrapping here caused recursion with API shim
function ACVar:_ensureToggleFrameWrap()
    self._toggleFrameWrapped = true
end

local function formatMessage(template, ...)
    return CONSTANTS.COLORS.SUCCESS..L.ADDON_NAME..":"..CONSTANTS.COLORS.RESET.." "..string.format(template, ...)
end

function ACVar:PrintMessage(message, ...)
    _G.DEFAULT_CHAT_FRAME:AddMessage(formatMessage(message, ...))
end

function ACVar:PrintCVarChange(cvarName, value)
    ACVar:PrintMessage(
        L.MSG_SET_VALUE,
        CONSTANTS.COLORS.HIGHLIGHT..cvarName..CONSTANTS.COLORS.RESET,
        CONSTANTS.COLORS.VALUE..tostring(value)..CONSTANTS.COLORS.RESET
    )
end

function ACVar:GetCVarValue(cvarName)
    local value = GetCVar(cvarName)
    return tonumber(value) or value
end

function ACVar:SetCVarValue(cvarName, value, cvarDef)
    -- Apply immediately if the CVar exists; otherwise defer until it appears
    if GetCVar(cvarName) ~= nil then
        pcall(SetCVar, cvarName, value)
        -- Persist right away if needed
        ACVar:_persistCVar(cvarName, value, cvarDef)
        if cvarDef and cvarDef.reloadRequired then
            self.reloadIsPending = true
        end
        -- If the UI is initialized, refresh the specific control immediately
        if ACVar.Frame and ACVar.UpdateUIForCVar and cvarDef and ACVar.Frame:IsShown() and not InCombatLockdown() then
            ACVar:UpdateUIForCVar(cvarDef)
        end
        return
    end

    -- Defer until the CVar is registered by the client/injection
    local pending = ACVar._pendingLookup[cvarName]
    if pending then
        pending.value = value
        pending.cvarDef = cvarDef or pending.cvarDef
    else
        pending = { name = cvarName, value = value, cvarDef = cvarDef }
        table.insert(ACVar._pendingCVarSets, pending)
        ACVar._pendingLookup[cvarName] = pending
    end
    -- Persist even if deferred so it survives reloads
    ACVar:_persistCVar(cvarName, value, cvarDef)
end

function ACVar.FormatNumber(value)
    return tonumber(format("%.2f", value or 0)) or 0
end

-- ### Slash Command Handler ###
local function processSlashCommand(msg)
    msg = lower(trim(msg))

    if msg == "" or msg == "toggle" then
        ACVar:ToggleFrame()
    elseif msg == "show" then
        ACVar:ShowFrame()
    elseif msg == "hide" then
        ACVar:HideFrame()
    elseif msg == "reset" or msg == "resetposition" then
        ACVar:ResetFramePosition()
    elseif msg == "help" then
        ACVar:PrintMessage(L.MSG_HELP_HEADER)
        ACVar:PrintMessage(L.MSG_HELP_TOGGLE)
        ACVar:PrintMessage(L.MSG_HELP_SHOW)
        ACVar:PrintMessage(L.MSG_HELP_HIDE)
        ACVar:PrintMessage(L.MSG_HELP_RESET)
        ACVar:PrintMessage(L.MSG_HELP_HELP)
    else
        _G.DEFAULT_CHAT_FRAME:AddMessage(CONSTANTS.COLORS.ERROR..L.MSG_UNKNOWN_COMMAND)
    end
end

-- ### Initialization and Event Handling ###
function ACVar:OnLoad()
    -- Defer UI creation until first open (/awesome) to reduce taint at load
    -- self:CreateMainFrame()
    -- self:CreateReloadPopup()
    -- self:CreateDefaultConfirmationPopup()
    -- Disabled to reduce taint from modifying GameMenuFrame; use /awesome slash command instead
    self:PrintMessage(L.MSG_LOADED)
    -- Load and apply persisted CVar values (deferred if CVars not yet registered)
    if self.ApplyPersistedCVars then
         self:ApplyPersistedCVars()
    end
    -- Ensure UI functions are wrapped to sync toggles reliably (guarded)
    if self._patchUpdateUIForCVar then
        self:_patchUpdateUIForCVar()
    end
    if self._ensureShowFrameWrap then
        self:_ensureShowFrameWrap()
    end
    if self._ensureToggleFrameWrap then
        self:_ensureToggleFrameWrap()
    end
end

local eventFrame = CreateFrame("Frame")
eventFrame:RegisterEvent("ADDON_LOADED")
eventFrame:RegisterEvent("PLAYER_LOGIN")
eventFrame:RegisterEvent("PLAYER_ENTERING_WORLD")
eventFrame:RegisterEvent("PLAYER_REGEN_ENABLED")
eventFrame:SetScript("OnEvent", function(self, event, arg)
    if event == "ADDON_LOADED" and arg == addonName then
        ACVar:OnLoad()
        self:UnregisterEvent("ADDON_LOADED")
        return
    end

    if event == "PLAYER_REGEN_ENABLED" then
        -- Out of combat: safe to resync UI and honor any deferred show requests
        if ACVar.Frame and ACVar.Frame:IsShown() and ACVar.UpdateAllUI then
            ACVar:UpdateAllUI()
        end
        if ACVar._syncToggleCheckboxes then
            ACVar:_syncToggleCheckboxes()
        end
        if ACVar.BuildPendingPanel then
            ACVar:BuildPendingPanel()
        end
        -- Ensure Game Menu button exists after combat if it wasn't created yet
        if ACVar.AddGameMenuButton and not _G.GameMenuButtonAwesomeCVar then
            ACVar:AddGameMenuButton()
        end
        if ACVar._showAfterCombat then
            ACVar._showAfterCombat = nil
            if ACVar.ShowFrame then ACVar:ShowFrame() end
        end
        return
    end

    if (event == "PLAYER_LOGIN" or event == "PLAYER_ENTERING_WORLD") then
        -- Avoid UI mutations in combat to reduce taint; defer to PLAYER_REGEN_ENABLED
        if not (InCombatLockdown and InCombatLockdown()) then
            if ACVar.UpdateAllUI then
                ACVar:UpdateAllUI()
            end
            if ACVar._syncToggleCheckboxes then
                ACVar:_syncToggleCheckboxes()
            end
            if ACVar.BuildPendingPanel then
                ACVar:BuildPendingPanel()
            end
            -- Ensure Game Menu button is present when safe
            if ACVar.AddGameMenuButton and not _G.GameMenuButtonAwesomeCVar then
                ACVar:AddGameMenuButton()
            end
        end
        return
    end
end)

-- Register Slash Commands
SLASH_AWESOME1 = "/awesome"
SLASH_AWESOME2 = "/awesomecvar"
SlashCmdList["AWESOME"] = processSlashCommand

-- Hide Blizzard blocked-action popups and taint error spam (cosmetic only; actions remain blocked)
do
    local function installTaintSuppressor()
        -- Disable taint log and Lua error popups to reduce noise
        if SetCVar then
            pcall(SetCVar, "taintLog", 0)
            pcall(SetCVar, "scriptErrors", 0)
        end

        -- Hide StaticPopup dialogs for addon action blocked/forbidden
        if type(hooksecurefunc) == "function" then
            hooksecurefunc("StaticPopup_Show", function(which)
                if which == "ADDON_ACTION_BLOCKED" or which == "ADDON_ACTION_FORBIDDEN" then
                    for i = 1, (STATICPOPUP_NUMDIALOGS or 4) do
                        local f = _G["StaticPopup"..i]
                        if f and f.which == which then f:Hide() end
                    end
                end
            end)
        end

        -- Filter UIErrorsFrame messages that are taint-related
        if UIErrorsFrame and UIErrorsFrame.GetScript then
            local orig = UIErrorsFrame:GetScript("OnEvent")
            UIErrorsFrame:SetScript("OnEvent", function(frame, event, ...)
                if event == "UI_ERROR_MESSAGE" then
                    local msg = ...
                    if type(msg) == "string" then
                        local drop = false
                        if _G.INTERFACE_ACTION_BLOCKED and msg == _G.INTERFACE_ACTION_BLOCKED then drop = true end
                        if _G.INTERFACE_ACTION_FORBIDDEN and msg == _G.INTERFACE_ACTION_FORBIDDEN then drop = true end
                        if _G.ADDON_ACTION_BLOCKED and msg == _G.ADDON_ACTION_BLOCKED then drop = true end
                        if _G.ADDON_ACTION_FORBIDDEN and msg == _G.ADDON_ACTION_FORBIDDEN then drop = true end
                        if not drop then
                            if msg:find("Interface action failed because of an AddOn", 1, true)
                            or msg:find("blocked because of", 1, true)
                            or msg:find("protected function", 1, true) then
                                drop = true
                            end
                        end
                        if drop then return end
                    end
                end
                if orig then return orig(frame, event, ...) end
            end)
        end
    end
    installTaintSuppressor()
end
