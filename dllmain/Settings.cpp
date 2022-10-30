﻿#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>
#include "dllmain.h"
#include "Settings.h"
#include "settings_string.h"
#include "trainer_string.h"
#include "Patches.h"
#include "input.hpp"
#include "Utils.h"
#include "Trainer.h"

std::shared_ptr<class Config> pConfig = std::make_shared<Config>();

const std::string sSettingOverridesPath = "re4_tweaks\\setting_overrides\\";
const std::string sHDProjectOverrideName = "HDProject.ini";

const char* sLeonCostumeNames[] = {"Jacket", "Normal", "Vest", "RPD", "Mafia"};
const char* sAshleyCostumeNames[] = {"Normal", "Popstar", "Armor"};
const char* sAdaCostumeNames[] = {"RE2", "Spy", "Normal"};

bool bIsUsingHDProject = false;

std::vector<uint32_t> ParseKeyCombo(std::string_view in_combo)
{
	// Convert combo to uppercase to match Settings::key_map
	std::string combo(in_combo);
	std::transform(combo.begin(), combo.end(), combo.begin(),
		[](unsigned char c) { return std::toupper(c); });

	std::vector<uint32_t> new_combo;
	std::string cur_token;

	// Parse combo tokens into buttons bitfield (tokens seperated by "+")
	for (size_t i = 0; i < combo.length(); i++)
	{
		char c = combo[i];

		if (c == '+')
		{
			// seperator, try parsing previous token

			if (cur_token.length())
			{
				uint32_t token_num = pInput->KeyMap_getVK(cur_token);
				if (!token_num)
				{
					// parse failed...
					new_combo.clear();
					return new_combo;
				}
				new_combo.push_back(token_num);
			}

			cur_token.clear();
		}
		else
		{
			// Must be a character key, convert to upper and add to our cur_token
			cur_token += ::toupper(c);
		}
	}

	if (cur_token.length())
	{
		// Get VK for the current token and push it into the vector
		uint32_t token_num = pInput->KeyMap_getVK(cur_token);
		if (!token_num)
		{
			// parse failed...
			new_combo.clear();
			return new_combo;
		}
		new_combo.push_back(token_num);
	}

	return new_combo;
}

void Config::ReadSettings()
{
	// Read default settings file first
	std::string sDefaultIniPath = rootPath + WrapperName.substr(0, WrapperName.find_last_of('.')) + ".ini";
	ReadSettings(sDefaultIniPath);

	// Try reading in trainer.ini settings
	std::string sTrainerIniPath = rootPath + "\\re4_tweaks\\trainer.ini";
	if (std::filesystem::exists(sTrainerIniPath))
		ReadSettings(sTrainerIniPath);

	// Try reading any setting override files
	auto override_path = rootPath + sSettingOverridesPath;

	if (!std::filesystem::exists(override_path))
		return;

	std::vector<std::filesystem::path> override_inis;
	for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(override_path))
	{
		auto& path = dirEntry.path();

		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return std::tolower(c); });

		if (ext != ".ini")
			continue;

		override_inis.push_back(path);
	}

	// Sort INI filenames alphabetically
	std::sort(override_inis.begin(), override_inis.end());

	// Process override INIs
	for (const auto& path : override_inis)
		ReadSettings(path.string());

	// Special case for HDProject settings, make sure it overrides all other INIs
	auto hdproject_path = override_path + sHDProjectOverrideName;
	if (std::filesystem::exists(hdproject_path))
	{
		bIsUsingHDProject = true;
		ReadSettings(hdproject_path);
	}
}

void Config::ParseHotkeys()
{
	pInput->ClearHotkeys();

	ParseConfigMenuKeyCombo(pConfig->sConfigMenuKeyCombo);
	ParseConsoleKeyCombo(pConfig->sConsoleKeyCombo);
	ParseToolMenuKeyCombo(pConfig->sDebugMenuKeyCombo);
	ParseMouseTurnModifierCombo(pConfig->sMouseTurnModifierKeyCombo);
	ParseJetSkiTrickCombo(pConfig->sJetSkiTrickCombo);
	ParseImGuiUIFocusCombo(pConfig->sTrainerFocusUIKeyCombo);

	Trainer_ParseKeyCombos();
}

void Config::ReadSettings(std::string_view ini_path)
{
	CmdIniReader iniReader(ini_path);

	#ifdef VERBOSE
	con.AddLogChar("Reading settings from: %s", ini_path.data());
	#endif

	spd::log()->info("Reading settings from: \"{}\"", ini_path.data());

	pConfig->HasUnsavedChanges = false;

	// DISPLAY
	pConfig->fFOVAdditional = iniReader.ReadFloat("DISPLAY", "FOVAdditional", pConfig->fFOVAdditional);
	if (pConfig->fFOVAdditional > 0.0f)
		pConfig->bEnableFOV = true;
	else
		pConfig->bEnableFOV = false;

	pConfig->bDisableVsync = iniReader.ReadBoolean("DISPLAY", "DisableVsync", pConfig->bDisableVsync);

	pConfig->bUltraWideAspectSupport = iniReader.ReadBoolean("DISPLAY", "UltraWideAspectSupport", pConfig->bUltraWideAspectSupport);
	pConfig->bSideAlignHUD = iniReader.ReadBoolean("DISPLAY", "SideAlignHUD", pConfig->bSideAlignHUD);
	pConfig->bStretchFullscreenImages = iniReader.ReadBoolean("DISPLAY", "StretchFullscreenImages", pConfig->bStretchFullscreenImages);
	pConfig->bStretchVideos = iniReader.ReadBoolean("DISPLAY", "StretchVideos", pConfig->bStretchVideos);
	pConfig->bRemove16by10BlackBars = iniReader.ReadBoolean("DISPLAY", "Remove16by10BlackBars", pConfig->bRemove16by10BlackBars);

	pConfig->bReplaceFramelimiter = iniReader.ReadBoolean("DISPLAY", "ReplaceFramelimiter", pConfig->bReplaceFramelimiter);
	pConfig->bFixDPIScale = iniReader.ReadBoolean("DISPLAY", "FixDPIScale", pConfig->bFixDPIScale);
	pConfig->bFixDisplayMode = iniReader.ReadBoolean("DISPLAY", "FixDisplayMode", pConfig->bFixDisplayMode);
	pConfig->iCustomRefreshRate = iniReader.ReadInteger("DISPLAY", "CustomRefreshRate", pConfig->iCustomRefreshRate);
	pConfig->bOverrideLaserColor = iniReader.ReadBoolean("DISPLAY", "OverrideLaserColor", pConfig->bOverrideLaserColor);
	pConfig->bRainbowLaser = iniReader.ReadBoolean("DISPLAY", "RainbowLaser", pConfig->bRainbowLaser);

	pConfig->iLaserR = iniReader.ReadInteger("DISPLAY", "LaserR", pConfig->iLaserR);
	pConfig->iLaserG = iniReader.ReadInteger("DISPLAY", "LaserG", pConfig->iLaserG);
	pConfig->iLaserB = iniReader.ReadInteger("DISPLAY", "LaserB", pConfig->iLaserB);
	fLaserColorPicker[0] = pConfig->iLaserR / 255.0f;
	fLaserColorPicker[1] = pConfig->iLaserG / 255.0f;
	fLaserColorPicker[2] = pConfig->iLaserB / 255.0f;

	pConfig->bRestorePickupTransparency = iniReader.ReadBoolean("DISPLAY", "RestorePickupTransparency", pConfig->bRestorePickupTransparency);
	pConfig->bDisableBrokenFilter03 = iniReader.ReadBoolean("DISPLAY", "DisableBrokenFilter03", pConfig->bDisableBrokenFilter03);
	pConfig->bFixBlurryImage = iniReader.ReadBoolean("DISPLAY", "FixBlurryImage", pConfig->bFixBlurryImage);
	pConfig->bDisableFilmGrain = iniReader.ReadBoolean("DISPLAY", "DisableFilmGrain", pConfig->bDisableFilmGrain);
	pConfig->bEnableGCBlur = iniReader.ReadBoolean("DISPLAY", "EnableGCBlur", pConfig->bEnableGCBlur);

	std::string GCBlurTypeStr = iniReader.ReadString("DISPLAY", "GCBlurType", "");
	if (!GCBlurTypeStr.empty())
	{
		if (GCBlurTypeStr == std::string("Enhanced"))
		{
			pConfig->bUseEnhancedGCBlur = true;
			iGCBlurMode = 0;
		}
		else if (GCBlurTypeStr == std::string("Classic"))
		{
			pConfig->bUseEnhancedGCBlur = false;
			iGCBlurMode = 1;
		}
	}

	pConfig->bEnableGCScopeBlur = iniReader.ReadBoolean("DISPLAY", "EnableGCScopeBlur", pConfig->bEnableGCScopeBlur);
	pConfig->bWindowBorderless = iniReader.ReadBoolean("DISPLAY", "WindowBorderless", pConfig->bWindowBorderless);
	pConfig->iWindowPositionX = iniReader.ReadInteger("DISPLAY", "WindowPositionX", pConfig->iWindowPositionX);
	pConfig->iWindowPositionY = iniReader.ReadInteger("DISPLAY", "WindowPositionY", pConfig->iWindowPositionY);
	pConfig->bRememberWindowPos = iniReader.ReadBoolean("DISPLAY", "RememberWindowPos", pConfig->bRememberWindowPos);

	// AUDIO
	pConfig->iVolumeMaster = iniReader.ReadInteger("AUDIO", "VolumeMaster", pConfig->iVolumeMaster);
	pConfig->iVolumeMaster = min(max(pConfig->iVolumeMaster, 0), 100); // limit between 0 - 100

	pConfig->iVolumeBGM = iniReader.ReadInteger("AUDIO", "VolumeBGM", pConfig->iVolumeBGM);
	pConfig->iVolumeBGM = min(max(pConfig->iVolumeBGM, 0), 100); // limit between 0 - 100

	pConfig->iVolumeSE = iniReader.ReadInteger("AUDIO", "VolumeSE", pConfig->iVolumeSE);
	pConfig->iVolumeSE = min(max(pConfig->iVolumeSE, 0), 100); // limit between 0 - 100

	pConfig->iVolumeCutscene = iniReader.ReadInteger("AUDIO", "VolumeCutscene", pConfig->iVolumeCutscene);
	pConfig->iVolumeCutscene = min(max(pConfig->iVolumeCutscene, 0), 100); // limit between 0 - 100

	// MOUSE
	pConfig->bCameraImprovements = iniReader.ReadBoolean("MOUSE", "CameraImprovements", pConfig->bCameraImprovements);
	pConfig->bResetCameraWhenRunning = iniReader.ReadBoolean("MOUSE", "ResetCameraWhenRunning", pConfig->bResetCameraWhenRunning);
	pConfig->fCameraSensitivity = iniReader.ReadFloat("MOUSE", "CameraSensitivity", pConfig->fCameraSensitivity);
	pConfig->fCameraSensitivity = fmin(fmax(pConfig->fCameraSensitivity, 0.5f), 2.0f); // limit between 0.5 - 2.0
	pConfig->bUseMouseTurning = iniReader.ReadBoolean("MOUSE", "UseMouseTurning", pConfig->bUseMouseTurning);
	
	std::string MouseTurnTypeStr = iniReader.ReadString("MOUSE", "MouseTurnType", "");
	if (!MouseTurnTypeStr.empty())
	{
		if (MouseTurnTypeStr == std::string("TypeA"))
			pConfig->iMouseTurnType = MouseTurnTypes::TypeA;
		else if (MouseTurnTypeStr == std::string("TypeB"))
			pConfig->iMouseTurnType = MouseTurnTypes::TypeB;
	}

	pConfig->fTurnTypeBSensitivity = iniReader.ReadFloat("MOUSE", "TurnTypeBSensitivity", pConfig->fTurnTypeBSensitivity);
	pConfig->fTurnTypeBSensitivity = fmin(fmax(pConfig->fTurnTypeBSensitivity, 0.5f), 2.0f); // limit between 0.5 - 2.0
	pConfig->bUseRawMouseInput = iniReader.ReadBoolean("MOUSE", "UseRawMouseInput", pConfig->bUseRawMouseInput);
	pConfig->bDetachCameraFromAim = iniReader.ReadBoolean("MOUSE", "DetachCameraFromAim", pConfig->bDetachCameraFromAim);
	pConfig->bFixSniperZoom = iniReader.ReadBoolean("MOUSE", "FixSniperZoom", pConfig->bFixSniperZoom);
	pConfig->bFixSniperFocus = iniReader.ReadBoolean("MOUSE", "FixSniperFocus", pConfig->bFixSniperFocus);
	pConfig->bFixRetryLoadMouseSelector = iniReader.ReadBoolean("MOUSE", "FixRetryLoadMouseSelector", pConfig->bFixRetryLoadMouseSelector);

	// KEYBOARD
	pConfig->bFallbackToEnglishKeyIcons = iniReader.ReadBoolean("KEYBOARD", "FallbackToEnglishKeyIcons", pConfig->bFallbackToEnglishKeyIcons);
	pConfig->bAllowReloadWithoutAiming_kbm = iniReader.ReadBoolean("KEYBOARD", "AllowReloadWithoutAiming", pConfig->bAllowReloadWithoutAiming_kbm);
	pConfig->bReloadWithoutZoom_kbm = iniReader.ReadBoolean("KEYBOARD", "ReloadWithoutZoom", pConfig->bReloadWithoutZoom_kbm);

	// CONTROLLER
	pConfig->bOverrideControllerSensitivity = iniReader.ReadBoolean("CONTROLLER", "OverrideControllerSensitivity", pConfig->bOverrideControllerSensitivity);
	pConfig->fControllerSensitivity = iniReader.ReadFloat("CONTROLLER", "ControllerSensitivity", pConfig->fControllerSensitivity);
	pConfig->fControllerSensitivity = fmin(fmax(pConfig->fControllerSensitivity, 0.5f), 4.0f); // limit between 0.5 - 4.0
	pConfig->bRemoveExtraXinputDeadzone = iniReader.ReadBoolean("CONTROLLER", "RemoveExtraXinputDeadzone", pConfig->bRemoveExtraXinputDeadzone);
	pConfig->bOverrideXinputDeadzone = iniReader.ReadBoolean("CONTROLLER", "OverrideXinputDeadzone", pConfig->bOverrideXinputDeadzone);
	pConfig->fXinputDeadzone = iniReader.ReadFloat("CONTROLLER", "XinputDeadzone", pConfig->fXinputDeadzone);
	pConfig->fXinputDeadzone = fmin(fmax(pConfig->fXinputDeadzone, 0.0f), 3.5f); // limit between 0.0 - 3.5
	pConfig->bAllowReloadWithoutAiming_controller = iniReader.ReadBoolean("CONTROLLER", "AllowReloadWithoutAiming", pConfig->bAllowReloadWithoutAiming_controller);
	pConfig->bReloadWithoutZoom_controller = iniReader.ReadBoolean("CONTROLLER", "ReloadWithoutZoom", pConfig->bReloadWithoutZoom_controller);

	// FRAME RATE
	pConfig->bFixFallingItemsSpeed = iniReader.ReadBoolean("FRAME RATE", "FixFallingItemsSpeed", pConfig->bFixFallingItemsSpeed);
	pConfig->bFixTurningSpeed = iniReader.ReadBoolean("FRAME RATE", "FixTurningSpeed", pConfig->bFixTurningSpeed);
	pConfig->bFixQTE = iniReader.ReadBoolean("FRAME RATE", "FixQTE", pConfig->bFixQTE);
	pConfig->bFixAshleyBustPhysics = iniReader.ReadBoolean("FRAME RATE", "FixAshleyBustPhysics", pConfig->bFixAshleyBustPhysics);
	pConfig->bEnableFastMath = iniReader.ReadBoolean("FRAME RATE", "EnableFastMath", pConfig->bEnableFastMath);
	pConfig->bPrecacheModels = iniReader.ReadBoolean("FRAME RATE", "PrecacheModels", pConfig->bPrecacheModels);

	// MISC
	pConfig->bOverrideCostumes = iniReader.ReadBoolean("MISC", "OverrideCostumes", pConfig->bOverrideCostumes);

	std::string buf = iniReader.ReadString("MISC", "LeonCostume", "");
	if (!buf.empty())
	{
		if (buf == "Jacket") pConfig->CostumeOverride.Leon = LeonCostume::Jacket;
		if (buf == "Normal") pConfig->CostumeOverride.Leon = LeonCostume::Normal;
		if (buf == "Vest") pConfig->CostumeOverride.Leon = LeonCostume::Vest;
		if (buf == "RPD") pConfig->CostumeOverride.Leon = LeonCostume::RPD;
		if (buf == "Mafia") pConfig->CostumeOverride.Leon = LeonCostume::Mafia;

		iCostumeComboLeon = (int)pConfig->CostumeOverride.Leon;
	}

	buf = iniReader.ReadString("MISC", "AshleyCostume", "");
	if (!buf.empty())
	{
		if (buf == "Normal") pConfig->CostumeOverride.Ashley = AshleyCostume::Normal;
		if (buf == "Popstar") pConfig->CostumeOverride.Ashley = AshleyCostume::Popstar;
		if (buf == "Armor") pConfig->CostumeOverride.Ashley = AshleyCostume::Armor;

		iCostumeComboAshley = (int)pConfig->CostumeOverride.Ashley;
	}

	buf = iniReader.ReadString("MISC", "AdaCostume", "");
	if (!buf.empty())
	{
		if (buf == "RE2") pConfig->CostumeOverride.Ada = AdaCostume::RE2;
		if (buf == "Spy") pConfig->CostumeOverride.Ada = AdaCostume::Spy;
		if (buf == "Normal") pConfig->CostumeOverride.Ada = AdaCostume::Normal;

		iCostumeComboAda = (int)pConfig->CostumeOverride.Ada;

		// Normal is id 3, but we're lying to ImGui by pretending Normal is id 2 instead.
		if (pConfig->CostumeOverride.Ada == AdaCostume::Normal)
			iCostumeComboAda--;
	}

	pConfig->bAshleyJPCameraAngles = iniReader.ReadBoolean("MISC", "AshleyJPCameraAngles", pConfig->bAshleyJPCameraAngles);
	pConfig->iViolenceLevelOverride = iniReader.ReadInteger("MISC", "ViolenceLevelOverride", pConfig->iViolenceLevelOverride);
	pConfig->iViolenceLevelOverride = min(max(pConfig->iViolenceLevelOverride, -1), 2); // limit between -1 to 2
	pConfig->bAllowSellingHandgunSilencer = iniReader.ReadBoolean("MISC", "AllowSellingHandgunSilencer", pConfig->bAllowSellingHandgunSilencer);
	pConfig->bAllowMafiaLeonCutscenes = iniReader.ReadBoolean("MISC", "AllowMafiaLeonCutscenes", pConfig->bAllowMafiaLeonCutscenes);
	pConfig->bSilenceArmoredAshley = iniReader.ReadBoolean("MISC", "SilenceArmoredAshley", pConfig->bSilenceArmoredAshley);
	pConfig->bAllowAshleySuplex = iniReader.ReadBoolean("MISC", "AllowAshleySuplex", pConfig->bAllowAshleySuplex);
	pConfig->bAllowMatildaQuickturn = iniReader.ReadBoolean("MISC", "AllowMatildaQuickturn", pConfig->bAllowMatildaQuickturn);
	pConfig->bFixDitmanGlitch = iniReader.ReadBoolean("MISC", "FixDitmanGlitch", pConfig->bFixDitmanGlitch);
	pConfig->bUseSprintToggle = iniReader.ReadBoolean("MISC", "UseSprintToggle", pConfig->bUseSprintToggle);
	pConfig->bDisableQTE = iniReader.ReadBoolean("MISC", "DisableQTE", pConfig->bDisableQTE);
	pConfig->bAutomaticMashingQTE = iniReader.ReadBoolean("MISC", "AutomaticMashingQTE", pConfig->bAutomaticMashingQTE);
	pConfig->bSkipIntroLogos = iniReader.ReadBoolean("MISC", "SkipIntroLogos", pConfig->bSkipIntroLogos);
	pConfig->bSkipMenuLogo = iniReader.ReadBoolean("MISC", "SkipMenuLogo", pConfig->bSkipMenuLogo);
	pConfig->bEnableDebugMenu = iniReader.ReadBoolean("MISC", "EnableDebugMenu", pConfig->bEnableDebugMenu);
	pConfig->bEnableModExpansion = iniReader.ReadBoolean("MISC", "EnableModExpansion", pConfig->bEnableModExpansion);
	pConfig->bDisableAdaKnifeGC = iniReader.ReadBoolean("MISC", "DisableAdaKnifeGC", pConfig->bDisableAdaKnifeGC);
	pConfig->bDisableAdaKnifePS2 = iniReader.ReadBoolean("MISC", "DisableAdaKnifePS2", pConfig->bDisableAdaKnifePS2);

	// MEMORY
	pConfig->bAllowHighResolutionSFD = iniReader.ReadBoolean("MEMORY", "AllowHighResolutionSFD", pConfig->bAllowHighResolutionSFD);
	pConfig->bRaiseVertexAlloc = iniReader.ReadBoolean("MEMORY", "RaiseVertexAlloc", pConfig->bRaiseVertexAlloc);
	pConfig->bRaiseInventoryAlloc = iniReader.ReadBoolean("MEMORY", "RaiseInventoryAlloc", pConfig->bRaiseInventoryAlloc);

	// HOTKEYS
	pConfig->sConfigMenuKeyCombo = StrToUpper(iniReader.ReadString("HOTKEYS", "ConfigMenu", pConfig->sConfigMenuKeyCombo));
	pConfig->sConsoleKeyCombo = StrToUpper(iniReader.ReadString("HOTKEYS", "Console", pConfig->sConsoleKeyCombo));
	pConfig->sFlipItemUp = StrToUpper(iniReader.ReadString("HOTKEYS", "FlipItemUp", pConfig->sFlipItemUp));
	pConfig->sFlipItemDown = StrToUpper(iniReader.ReadString("HOTKEYS", "FlipItemDown", pConfig->sFlipItemDown));
	pConfig->sFlipItemLeft = StrToUpper(iniReader.ReadString("HOTKEYS", "FlipItemLeft", pConfig->sFlipItemLeft));
	pConfig->sFlipItemRight = StrToUpper(iniReader.ReadString("HOTKEYS", "FlipItemRight", pConfig->sFlipItemRight));

	pConfig->sQTE_key_1 = StrToUpper(iniReader.ReadString("HOTKEYS", "QTE_key_1", pConfig->sQTE_key_1));
	pConfig->sQTE_key_2 = StrToUpper(iniReader.ReadString("HOTKEYS", "QTE_key_2", pConfig->sQTE_key_2));
	
	// Check if the QTE bindings are valid for the current keyboard layout.
	// Try to reset them using VK Hex Codes if they aren't.
	if (pInput->KeyMap_getVK(pConfig->sQTE_key_1) == 0)
		pConfig->sQTE_key_1 = pInput->KeyMap_getSTR(0x44); // Latin D

	if (pInput->KeyMap_getVK(pConfig->sQTE_key_2) == 0)
		pConfig->sQTE_key_2 = pInput->KeyMap_getSTR(0x41); // Latin A

	pConfig->sDebugMenuKeyCombo = StrToUpper(iniReader.ReadString("HOTKEYS", "DebugMenu", pConfig->sDebugMenuKeyCombo));
	pConfig->sMouseTurnModifierKeyCombo = StrToUpper(iniReader.ReadString("HOTKEYS", "MouseTurningModifier", pConfig->sMouseTurnModifierKeyCombo));
	pConfig->sJetSkiTrickCombo = StrToUpper(iniReader.ReadString("HOTKEYS", "JetSkiTricks", pConfig->sJetSkiTrickCombo));

	// TRAINER
	pConfig->bTrainerEnable = iniReader.ReadBoolean("TRAINER", "Enable", pConfig->bTrainerEnable);
	pConfig->bTrainerPlayerSpeedOverride = iniReader.ReadBoolean("TRAINER", "EnablePlayerSpeedOverride", pConfig->bTrainerPlayerSpeedOverride);
	pConfig->fTrainerPlayerSpeedOverride = iniReader.ReadFloat("TRAINER", "PlayerSpeedOverride", pConfig->fTrainerPlayerSpeedOverride);
	pConfig->bTrainerUseNumpadMovement = iniReader.ReadBoolean("TRAINER", "UseNumpadMovement", pConfig->bTrainerUseNumpadMovement);
	pConfig->bTrainerUseMouseWheelUpDown = iniReader.ReadBoolean("TRAINER", "UseMouseWheelUpDown", pConfig->bTrainerUseMouseWheelUpDown);
	pConfig->fTrainerNumMoveSpeed = iniReader.ReadFloat("TRAINER", "NumpadMovementSpeed", pConfig->fTrainerNumMoveSpeed);
	pConfig->fTrainerNumMoveSpeed = fmin(fmax(pConfig->fTrainerNumMoveSpeed, 0.1f), 10.0f); // limit between 0.1 - 10
	pConfig->bTrainerEnableFreeCam = iniReader.ReadBoolean("TRAINER", "EnableFreeCamera", pConfig->bTrainerEnableFreeCam);
	pConfig->fTrainerFreeCamSpeed = iniReader.ReadFloat("TRAINER", "FreeCamSpeed", pConfig->fTrainerFreeCamSpeed);
	pConfig->fTrainerFreeCamSpeed = fmin(fmax(pConfig->fTrainerFreeCamSpeed, 0.1f), 10.0f); // limit between 0.1 - 10
	pConfig->bTrainerEnemyHPMultiplier = iniReader.ReadBoolean("TRAINER", "EnableEnemyHPMultiplier", pConfig->bTrainerEnemyHPMultiplier);
	pConfig->fTrainerEnemyHPMultiplier = iniReader.ReadFloat("TRAINER", "EnemyHPMultiplier", pConfig->fTrainerEnemyHPMultiplier);
	pConfig->fTrainerEnemyHPMultiplier = fmin(fmax(pConfig->fTrainerEnemyHPMultiplier, 0.1f), 15.0f); // limit between 0.1 - 15
	pConfig->bTrainerRandomHPMultiplier = iniReader.ReadBoolean("TRAINER", "UseRandomHPMultiplier", pConfig->bTrainerRandomHPMultiplier);
	pConfig->fTrainerRandomHPMultiMin = iniReader.ReadFloat("TRAINER", "RandomHPMultiplierMin", pConfig->fTrainerRandomHPMultiMin);
	pConfig->fTrainerRandomHPMultiMin = fmin(fmax(pConfig->fTrainerRandomHPMultiMin, 0.1f), 14.0f); // limit between 0.1 - 14
	pConfig->fTrainerRandomHPMultiMax = iniReader.ReadFloat("TRAINER", "RandomHPMultiplierMax", pConfig->fTrainerRandomHPMultiMax);
	pConfig->fTrainerRandomHPMultiMax = fmin(fmax(pConfig->fTrainerRandomHPMultiMax, fTrainerRandomHPMultiMin), 15.0f); // limit between fTrainerRandomHPMultiMin - 15
	pConfig->bTrainerDisableEnemySpawn = iniReader.ReadBoolean("TRAINER", "DisableEnemySpawn", pConfig->bTrainerDisableEnemySpawn);
	pConfig->bTrainerDeadBodiesNeverDisappear = iniReader.ReadBoolean("TRAINER", "DeadBodiesNeverDisappear", pConfig->bTrainerDeadBodiesNeverDisappear);
	pConfig->bTrainerAllowEnterDoorsWithoutAsh = iniReader.ReadBoolean("TRAINER", "AllowEnterDoorsWithoutAshley", pConfig->bTrainerAllowEnterDoorsWithoutAsh);

	// ESP
	pConfig->bShowESP = iniReader.ReadBoolean("ESP", "ShowESP", pConfig->bShowESP);
	pConfig->bEspShowInfoOnTop = iniReader.ReadBoolean("ESP", "ShowInfoOnTop", pConfig->bEspShowInfoOnTop);
	pConfig->bEspOnlyShowEnemies = iniReader.ReadBoolean("ESP", "OnlyShowEnemies", pConfig->bEspOnlyShowEnemies);
	pConfig->bEspOnlyShowValidEms = iniReader.ReadBoolean("ESP", "OnlyShowValidEms", pConfig->bEspOnlyShowValidEms);
	pConfig->bEspOnlyShowESLSpawned = iniReader.ReadBoolean("ESP", "OnlyShowESLSpawned", pConfig->bEspOnlyShowESLSpawned);
	pConfig->bEspOnlyShowAlive = iniReader.ReadBoolean("ESP", "OnlyShowAlive", pConfig->bEspOnlyShowAlive);
	pConfig->fEspMaxEmDistance = iniReader.ReadFloat("ESP", "MaxEmDistance", pConfig->fEspMaxEmDistance);
	pConfig->bEspOnlyShowClosestEms = iniReader.ReadBoolean("ESP", "OnlyShowClosestEms", pConfig->bEspOnlyShowClosestEms);
	pConfig->iEspClosestEmsAmount = iniReader.ReadInteger("ESP", "ClosestEmsAmount", pConfig->iEspClosestEmsAmount);
	pConfig->bEspDrawLines = iniReader.ReadBoolean("ESP", "DrawLines", pConfig->bEspDrawLines);

	buf = iniReader.ReadString("ESP", "EmNameMode", "");
	if (!buf.empty())
	{
		if (buf == "DontShow") pConfig->iEspEmNameMode = 0;
		if (buf == "Normal") pConfig->iEspEmNameMode = 1;
		if (buf == "Simplified") pConfig->iEspEmNameMode = 2;
	}

	buf = iniReader.ReadString("ESP", "EmHPMode", "");
	if (!buf.empty())
	{
		if (buf == "DontShow") pConfig->iEspEmHPMode = 0;
		if (buf == "Bar") pConfig->iEspEmHPMode = 1;
		if (buf == "Text") pConfig->iEspEmHPMode = 2;
	}

	pConfig->bEspDrawDebugInfo = iniReader.ReadBoolean("ESP", "DrawDebugInfo", pConfig->bEspDrawDebugInfo);

	// SIDEINFO
	pConfig->bShowSideInfo = iniReader.ReadBoolean("SIDEINFO", "ShowSideInfo", pConfig->bShowSideInfo);
	pConfig->bSideShowEmCount = iniReader.ReadBoolean("SIDEINFO", "ShowEmCount", pConfig->bSideShowEmCount);
	pConfig->bSideShowEmList = iniReader.ReadBoolean("SIDEINFO", "ShowEmList", pConfig->bSideShowEmList);
	pConfig->bSideOnlyShowESLSpawned = iniReader.ReadBoolean("SIDEINFO", "OnlyShowESLSpawned", pConfig->bSideOnlyShowESLSpawned);
	pConfig->bSideShowSimpleNames = iniReader.ReadBoolean("SIDEINFO", "ShowSimpleNames", pConfig->bSideShowSimpleNames);
	pConfig->iSideClosestEmsAmount = iniReader.ReadInteger("SIDEINFO", "ClosestEmsAmount", pConfig->iSideClosestEmsAmount);
	pConfig->fSideMaxEmDistance = iniReader.ReadFloat("SIDEINFO", "MaxEmDistance", pConfig->fSideMaxEmDistance);

	buf = iniReader.ReadString("SIDEINFO", "EmHPMode", "");
	if (!buf.empty())
	{
		if (buf == "DontShow") pConfig->iSideEmHPMode = 0;
		if (buf == "Bar") pConfig->iSideEmHPMode = 1;
		if (buf == "Text") pConfig->iSideEmHPMode = 2;
	}

	// TRAINER HOTKEYS
	pConfig->sTrainerFocusUIKeyCombo = iniReader.ReadString("TRAINER_HOTKEYS", "FocusUI", pConfig->sTrainerFocusUIKeyCombo);
	pConfig->sTrainerNoclipKeyCombo = iniReader.ReadString("TRAINER_HOTKEYS", "NoclipToggle", pConfig->sTrainerNoclipKeyCombo);
	pConfig->sTrainerFreeCamKeyCombo = iniReader.ReadString("TRAINER_HOTKEYS", "FreeCamToggle", pConfig->sTrainerFreeCamKeyCombo);
	pConfig->sTrainerSpeedOverrideKeyCombo = iniReader.ReadString("TRAINER_HOTKEYS", "SpeedOverrideToggle", pConfig->sTrainerSpeedOverrideKeyCombo);
	pConfig->sTrainerMoveAshToPlayerKeyCombo = iniReader.ReadString("TRAINER_HOTKEYS", "MoveAshleyToPlayer", pConfig->sTrainerMoveAshToPlayerKeyCombo);

	// WEAPON HOTKEYS
	pConfig->bWeaponHotkeysEnable = iniReader.ReadBoolean("WEAPON_HOTKEYS", "Enable", pConfig->bWeaponHotkeysEnable);
	pConfig->sWeaponHotkeys[0] = iniReader.ReadString("WEAPON_HOTKEYS", "WeaponHotkeySlot1", pConfig->sWeaponHotkeys[0]);
	pConfig->sWeaponHotkeys[1] = iniReader.ReadString("WEAPON_HOTKEYS", "WeaponHotkeySlot2", pConfig->sWeaponHotkeys[1]);
	pConfig->sWeaponHotkeys[2] = iniReader.ReadString("WEAPON_HOTKEYS", "WeaponHotkeySlot3", pConfig->sWeaponHotkeys[2]);
	pConfig->sWeaponHotkeys[3] = iniReader.ReadString("WEAPON_HOTKEYS", "WeaponHotkeySlot4", pConfig->sWeaponHotkeys[3]);
	pConfig->sWeaponHotkeys[4] = iniReader.ReadString("WEAPON_HOTKEYS", "WeaponHotkeySlot5", pConfig->sWeaponHotkeys[4]);
	pConfig->sLastWeaponHotkey = iniReader.ReadString("WEAPON_HOTKEYS", "LastWeaponHotkey", pConfig->sLastWeaponHotkey);
	pConfig->iWeaponHotkeyWepIds[0] = iniReader.ReadInteger("WEAPON_HOTKEYS", "WeaponIdSlot1", pConfig->iWeaponHotkeyWepIds[0]);
	pConfig->iWeaponHotkeyWepIds[1] = iniReader.ReadInteger("WEAPON_HOTKEYS", "WeaponIdSlot2", pConfig->iWeaponHotkeyWepIds[1]);
	pConfig->iWeaponHotkeyWepIds[2] = iniReader.ReadInteger("WEAPON_HOTKEYS", "WeaponIdSlot3", pConfig->iWeaponHotkeyWepIds[2]);
	pConfig->iWeaponHotkeyWepIds[3] = iniReader.ReadInteger("WEAPON_HOTKEYS", "WeaponIdSlot4", pConfig->iWeaponHotkeyWepIds[3]);
	pConfig->iWeaponHotkeyWepIds[4] = iniReader.ReadInteger("WEAPON_HOTKEYS", "WeaponIdSlot5", pConfig->iWeaponHotkeyWepIds[4]);
	auto readIntVect = [&iniReader](std::string section, std::string key, std::string& default_value)
	{
		std::vector<int> ret;

		default_value = iniReader.ReadString(section, key, default_value);

		std::stringstream ss(default_value);

		for (int i; ss >> i;) {
			ret.push_back(i);
			int peeked = ss.peek();
			if (peeked == ',' || peeked == ' ')
				ss.ignore();
		}

		return ret;
	};
	pConfig->iWeaponHotkeyCycle[0] = readIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot1", pConfig->iWeaponHotkeyCycleString[0]);
	pConfig->iWeaponHotkeyCycle[1] = readIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot2", pConfig->iWeaponHotkeyCycleString[1]);
	pConfig->iWeaponHotkeyCycle[2] = readIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot3", pConfig->iWeaponHotkeyCycleString[2]);
	pConfig->iWeaponHotkeyCycle[3] = readIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot4", pConfig->iWeaponHotkeyCycleString[3]);
	pConfig->iWeaponHotkeyCycle[4] = readIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot5", pConfig->iWeaponHotkeyCycleString[4]);

	// Parse all hotkeys
	ParseHotkeys();

	// FPS WARNING
	pConfig->bIgnoreFPSWarning = iniReader.ReadBoolean("WARNING", "IgnoreFPSWarning", pConfig->bIgnoreFPSWarning);
	
	// IMGUI
	pConfig->fFontSizeScale = iniReader.ReadFloat("IMGUI", "FontSizeScale", pConfig->fFontSizeScale);
	pConfig->fFontSizeScale = fmin(fmax(pConfig->fFontSizeScale, 1.0f), 1.25f); // limit between 1.0 - 1.25

	pConfig->bEnableDPIScale = iniReader.ReadBoolean("IMGUI", "EnableDPIScale", pConfig->bEnableDPIScale);
	pConfig->bDisableMenuTip = iniReader.ReadBoolean("IMGUI", "DisableMenuTip", pConfig->bDisableMenuTip);

	// DEBUG
	pConfig->bVerboseLog = iniReader.ReadBoolean("DEBUG", "VerboseLog", pConfig->bVerboseLog);
	pConfig->bNeverHideCursor = iniReader.ReadBoolean("DEBUG", "NeverHideCursor", pConfig->bNeverHideCursor);
	pConfig->bUseDynamicFrametime = iniReader.ReadBoolean("DEBUG", "UseDynamicFrametime", pConfig->bUseDynamicFrametime);
	pConfig->bDisableFramelimiting = iniReader.ReadBoolean("DEBUG", "DisableFramelimiting", pConfig->bDisableFramelimiting);

	if (iniReader.ReadBoolean("DEBUG", "TweaksDevMode", TweaksDevMode))
		TweaksDevMode = true; // let the INI enable it if it's disabled, but not disable it
}

std::mutex settingsThreadRunningMutex;

void WriteSettings(std::string_view iniPath, bool trainerIni)
{
	std::lock_guard<std::mutex> guard(settingsThreadRunningMutex); // only allow single thread writing to INI at one time

	CIniReader iniReader("");

	#ifdef VERBOSE
	con.AddConcatLog("Writing settings to: ", iniPath.data());
	#endif

	// Copy the default .ini to folder if one doesn't exist, just so we can keep comments and descriptions intact.
	const char* filename = iniPath.data();
	if (!std::filesystem::exists(filename)) {
		#ifdef VERBOSE
		con.AddLogChar("ini file doesn't exist in folder. Creating new one.");
		#endif

		std::filesystem::create_directory(std::filesystem::path(iniPath).parent_path()); // Create the dir if it doesn't exist

		std::ofstream iniFile(iniPath.data());

		if(!trainerIni)
			iniFile << defaultSettings + 1; // +1 to skip the first new line
		else
			iniFile << defaultSettingsTrainer + 1; // +1 to skip the first new line

		iniFile.close();
	}

	// Try to remove read-only flag is it is set, for some reason.
	DWORD iniFile = GetFileAttributesA(iniPath.data());
	if (iniFile != INVALID_FILE_ATTRIBUTES) {
		bool isReadOnly = iniFile & FILE_ATTRIBUTE_READONLY;

		if (isReadOnly)
		{
			#ifdef VERBOSE
			con.AddLogChar("Read-only ini file detected. Attempting to remove flag");
			#endif

			spd::log()->info("{} -> Read-only ini file detected. Attempting to remove flag", __FUNCTION__);

			SetFileAttributesA(iniPath.data(), iniFile & ~FILE_ATTRIBUTE_READONLY);
		}
	}

	if (trainerIni)
	{
		// trainer.ini-only settings
		iniReader = CIniReader(iniPath);

		// TRAINER
		iniReader.WriteBoolean("TRAINER", "Enable", pConfig->bTrainerEnable);
		iniReader.WriteBoolean("TRAINER", "EnablePlayerSpeedOverride", pConfig->bTrainerPlayerSpeedOverride);
		iniReader.WriteFloat("TRAINER", "PlayerSpeedOverride", pConfig->fTrainerPlayerSpeedOverride);
		iniReader.WriteBoolean("TRAINER", "UseNumpadMovement", pConfig->bTrainerUseNumpadMovement);
		iniReader.WriteBoolean("TRAINER", "UseMouseWheelUpDown", pConfig->bTrainerUseMouseWheelUpDown);
		iniReader.WriteFloat("TRAINER", "NumpadMovementSpeed", pConfig->fTrainerNumMoveSpeed);
		iniReader.WriteBoolean("TRAINER", "EnableFreeCamera", pConfig->bTrainerEnableFreeCam);
		iniReader.WriteFloat("TRAINER", "FreeCamSpeed", pConfig->fTrainerFreeCamSpeed);
		iniReader.WriteBoolean("TRAINER", "EnableEnemyHPMultiplier", pConfig->bTrainerEnemyHPMultiplier);
		iniReader.WriteFloat("TRAINER", "EnemyHPMultiplier", pConfig->fTrainerEnemyHPMultiplier);
		iniReader.WriteBoolean("TRAINER", "UseRandomHPMultiplier", pConfig->bTrainerRandomHPMultiplier);
		iniReader.WriteFloat("TRAINER", "RandomHPMultiplierMin", pConfig->fTrainerRandomHPMultiMin);
		iniReader.WriteFloat("TRAINER", "RandomHPMultiplierMax", pConfig->fTrainerRandomHPMultiMax);
		iniReader.WriteBoolean("TRAINER", "DisableEnemySpawn", pConfig->bTrainerDisableEnemySpawn);
		iniReader.WriteBoolean("TRAINER", "DeadBodiesNeverDisappear", pConfig->bTrainerDeadBodiesNeverDisappear);
		iniReader.WriteBoolean("TRAINER", "AllowEnterDoorsWithoutAshley", pConfig->bTrainerAllowEnterDoorsWithoutAsh);

		// ESP
		iniReader.WriteBoolean("ESP", "ShowESP", pConfig->bShowESP);
		iniReader.WriteBoolean("ESP", "ShowInfoOnTop", pConfig->bEspShowInfoOnTop);
		iniReader.WriteBoolean("ESP", "OnlyShowEnemies", pConfig->bEspOnlyShowEnemies);
		iniReader.WriteBoolean("ESP", "OnlyShowValidEms", pConfig->bEspOnlyShowValidEms);
		iniReader.WriteBoolean("ESP", "OnlyShowESLSpawned", pConfig->bEspOnlyShowESLSpawned);
		iniReader.WriteBoolean("ESP", "OnlyShowAlive", pConfig->bEspOnlyShowAlive);
		iniReader.WriteFloat("ESP", "MaxEmDistance", pConfig->fEspMaxEmDistance);
		iniReader.WriteBoolean("ESP", "OnlyShowClosestEms", pConfig->bEspOnlyShowClosestEms);
		iniReader.WriteInteger("ESP", "ClosestEmsAmount", pConfig->iEspClosestEmsAmount);
		iniReader.WriteBoolean("ESP", "DrawLines", pConfig->bEspDrawLines);

		std::string buf;
		switch (pConfig->iEspEmNameMode) {
		case 0:
			buf = "DontShow";
			break;
		case 1:
			buf = "Normal";
			break;
		case 2:
			buf = "Simplified";
			break;
		} iniReader.WriteString("ESP", "EmNameMode", " " + buf);

		switch (pConfig->iEspEmHPMode) {
		case 0:
			buf = "DontShow";
			break;
		case 1:
			buf = "Bar";
			break;
		case 2:
			buf = "Text";
			break;
		} iniReader.WriteString("ESP", "EmHPMode", " " + buf);

		iniReader.WriteBoolean("ESP", "DrawDebugInfo", pConfig->bEspDrawDebugInfo);

		// SIDEINFO
		iniReader.WriteBoolean("SIDEINFO", "ShowSideInfo", pConfig->bShowSideInfo);
		iniReader.WriteBoolean("SIDEINFO", "ShowEmCount", pConfig->bSideShowEmCount);
		iniReader.WriteBoolean("SIDEINFO", "ShowEmList", pConfig->bSideShowEmList);
		iniReader.WriteBoolean("SIDEINFO", "OnlyShowESLSpawned", pConfig->bSideOnlyShowESLSpawned);
		iniReader.WriteBoolean("SIDEINFO", "ShowSimpleNames", pConfig->bSideShowSimpleNames);
		iniReader.WriteInteger("SIDEINFO", "ClosestEmsAmount", pConfig->iSideClosestEmsAmount);
		iniReader.WriteFloat("SIDEINFO", "MaxEmDistance", pConfig->fSideMaxEmDistance);

		switch (pConfig->iSideEmHPMode) {
		case 0:
			buf = "DontShow";
			break;
		case 1:
			buf = "Bar";
			break;
		case 2:
			buf = "Text";
			break;
		} iniReader.WriteString("SIDEINFO", "EmHPMode", " " + buf);

		// TRAINER_HOTKEYS
		iniReader.WriteString("TRAINER_HOTKEYS", "FocusUI", " " + pConfig->sTrainerFocusUIKeyCombo);
		iniReader.WriteString("TRAINER_HOTKEYS", "NoclipToggle", " " + pConfig->sTrainerNoclipKeyCombo);
		iniReader.WriteString("TRAINER_HOTKEYS", "FreeCamToggle", " " + pConfig->sTrainerFreeCamKeyCombo);
		iniReader.WriteString("TRAINER_HOTKEYS", "SpeedOverrideToggle", " " + pConfig->sTrainerSpeedOverrideKeyCombo);
		iniReader.WriteString("TRAINER_HOTKEYS", "MoveAshleyToPlayer", " " + pConfig->sTrainerMoveAshToPlayerKeyCombo);

		// WEAPON HOTKEYS
		iniReader.WriteBoolean("WEAPON_HOTKEYS", "Enable", pConfig->bWeaponHotkeysEnable);
		iniReader.WriteString("WEAPON_HOTKEYS", "WeaponHotkeySlot1", " " + pConfig->sWeaponHotkeys[0]);
		iniReader.WriteString("WEAPON_HOTKEYS", "WeaponHotkeySlot2", " " + pConfig->sWeaponHotkeys[1]);
		iniReader.WriteString("WEAPON_HOTKEYS", "WeaponHotkeySlot3", " " + pConfig->sWeaponHotkeys[2]);
		iniReader.WriteString("WEAPON_HOTKEYS", "WeaponHotkeySlot4", " " + pConfig->sWeaponHotkeys[3]);
		iniReader.WriteString("WEAPON_HOTKEYS", "WeaponHotkeySlot5", " " + pConfig->sWeaponHotkeys[4]);
		iniReader.WriteInteger("WEAPON_HOTKEYS", "WeaponIdSlot1", pConfig->iWeaponHotkeyWepIds[0]);
		iniReader.WriteInteger("WEAPON_HOTKEYS", "WeaponIdSlot2", pConfig->iWeaponHotkeyWepIds[1]);
		iniReader.WriteInteger("WEAPON_HOTKEYS", "WeaponIdSlot3", pConfig->iWeaponHotkeyWepIds[2]);
		iniReader.WriteInteger("WEAPON_HOTKEYS", "WeaponIdSlot4", pConfig->iWeaponHotkeyWepIds[3]);
		iniReader.WriteInteger("WEAPON_HOTKEYS", "WeaponIdSlot5", pConfig->iWeaponHotkeyWepIds[4]);

		auto writeIntVect = [&iniReader](std::string section, std::string key, std::vector<int>& vect) {
			std::string val = "";
			for (int num : vect)
				val += std::to_string(num) + ", ";
			if (!val.empty())
				val = val.substr(0, val.size() - 2);
			iniReader.WriteString(section, key, " " + val);
		};

		writeIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot1", pConfig->iWeaponHotkeyCycle[0]);
		writeIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot2", pConfig->iWeaponHotkeyCycle[1]);
		writeIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot3", pConfig->iWeaponHotkeyCycle[2]);
		writeIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot4", pConfig->iWeaponHotkeyCycle[3]);
		writeIntVect("WEAPON_HOTKEYS", "WeaponCycleSlot5", pConfig->iWeaponHotkeyCycle[4]);

		iniReader.WriteString("WEAPON_HOTKEYS", "LastWeaponHotkey", " " + pConfig->sLastWeaponHotkey);

		return;
	}

	// DISPLAY
	iniReader.WriteFloat("DISPLAY", "FOVAdditional", pConfig->fFOVAdditional);
	iniReader.WriteBoolean("DISPLAY", "DisableVsync", pConfig->bDisableVsync);

	iniReader.WriteBoolean("DISPLAY", "UltraWideAspectSupport", pConfig->bUltraWideAspectSupport);
	iniReader.WriteBoolean("DISPLAY", "SideAlignHUD", pConfig->bSideAlignHUD);
	iniReader.WriteBoolean("DISPLAY", "StretchFullscreenImages", pConfig->bStretchFullscreenImages);
	iniReader.WriteBoolean("DISPLAY", "StretchVideos", pConfig->bStretchVideos);
	iniReader.WriteBoolean("DISPLAY", "Remove16by10BlackBars", pConfig->bRemove16by10BlackBars);
	iniReader.WriteBoolean("DISPLAY", "ReplaceFramelimiter", pConfig->bReplaceFramelimiter);
	iniReader.WriteBoolean("DISPLAY", "FixDPIScale", pConfig->bFixDPIScale);
	iniReader.WriteBoolean("DISPLAY", "FixDisplayMode", pConfig->bFixDisplayMode);
	iniReader.WriteInteger("DISPLAY", "CustomRefreshRate", pConfig->iCustomRefreshRate);
	iniReader.WriteBoolean("DISPLAY", "OverrideLaserColor", pConfig->bOverrideLaserColor);
	iniReader.WriteBoolean("DISPLAY", "RainbowLaser", pConfig->bRainbowLaser);

	iniReader.WriteInteger("DISPLAY", "LaserR", pConfig->iLaserR);
	iniReader.WriteInteger("DISPLAY", "LaserG", pConfig->iLaserG);
	iniReader.WriteInteger("DISPLAY", "LaserB", pConfig->iLaserB);

	iniReader.WriteBoolean("DISPLAY", "RestorePickupTransparency", pConfig->bRestorePickupTransparency);
	iniReader.WriteBoolean("DISPLAY", "DisableBrokenFilter03", pConfig->bDisableBrokenFilter03);
	iniReader.WriteBoolean("DISPLAY", "FixBlurryImage", pConfig->bFixBlurryImage);
	iniReader.WriteBoolean("DISPLAY", "DisableFilmGrain", pConfig->bDisableFilmGrain);
	iniReader.WriteBoolean("DISPLAY", "EnableGCBlur", pConfig->bEnableGCBlur);

	if (pConfig->bUseEnhancedGCBlur)
		iniReader.WriteString("DISPLAY", "GCBlurType", "Enhanced");
	else
		iniReader.WriteString("DISPLAY", "GCBlurType", "Classic");

	iniReader.WriteBoolean("DISPLAY", "EnableGCScopeBlur", pConfig->bEnableGCScopeBlur);
	iniReader.WriteBoolean("DISPLAY", "WindowBorderless", pConfig->bWindowBorderless);
	iniReader.WriteInteger("DISPLAY", "WindowPositionX", pConfig->iWindowPositionX);
	iniReader.WriteInteger("DISPLAY", "WindowPositionY", pConfig->iWindowPositionY);
	iniReader.WriteBoolean("DISPLAY", "RememberWindowPos", pConfig->bRememberWindowPos);

	// AUDIO
	iniReader.WriteInteger("AUDIO", "VolumeMaster", pConfig->iVolumeMaster);
	iniReader.WriteInteger("AUDIO", "VolumeBGM", pConfig->iVolumeBGM);
	iniReader.WriteInteger("AUDIO", "VolumeSE", pConfig->iVolumeSE);
	iniReader.WriteInteger("AUDIO", "VolumeCutscene", pConfig->iVolumeCutscene);

	// MOUSE
	iniReader.WriteBoolean("MOUSE", "CameraImprovements", pConfig->bCameraImprovements);
	iniReader.WriteBoolean("MOUSE", "ResetCameraWhenRunning", pConfig->bResetCameraWhenRunning);
	iniReader.WriteFloat("MOUSE", "CameraSensitivity", pConfig->fCameraSensitivity);
	iniReader.WriteBoolean("MOUSE", "UseMouseTurning", pConfig->bUseMouseTurning);

	if (pConfig->iMouseTurnType == MouseTurnTypes::TypeA)
		iniReader.WriteString("MOUSE", "MouseTurnType", "TypeA");
	else
		iniReader.WriteString("MOUSE", "MouseTurnType", "TypeB");

	iniReader.WriteFloat("MOUSE", "TurnTypeBSensitivity", pConfig->fTurnTypeBSensitivity);
	iniReader.WriteBoolean("MOUSE", "UseRawMouseInput", pConfig->bUseRawMouseInput);
	iniReader.WriteBoolean("MOUSE", "DetachCameraFromAim", pConfig->bDetachCameraFromAim);
	iniReader.WriteBoolean("MOUSE", "FixSniperZoom", pConfig->bFixSniperZoom);
	iniReader.WriteBoolean("MOUSE", "FixSniperFocus", pConfig->bFixSniperFocus);
	iniReader.WriteBoolean("MOUSE", "FixRetryLoadMouseSelector", pConfig->bFixRetryLoadMouseSelector);

	// KEYBOARD
	iniReader.WriteBoolean("KEYBOARD", "FallbackToEnglishKeyIcons", pConfig->bFallbackToEnglishKeyIcons);
	iniReader.WriteBoolean("KEYBOARD", "AllowReloadWithoutAiming", pConfig->bAllowReloadWithoutAiming_kbm);
	iniReader.WriteBoolean("KEYBOARD", "ReloadWithoutZoom", pConfig->bReloadWithoutZoom_kbm);

	// CONTROLLER
	iniReader.WriteBoolean("CONTROLLER", "OverrideControllerSensitivity", pConfig->bOverrideControllerSensitivity);
	iniReader.WriteFloat("CONTROLLER", "ControllerSensitivity", pConfig->fControllerSensitivity);
	iniReader.WriteBoolean("CONTROLLER", "RemoveExtraXinputDeadzone", pConfig->bRemoveExtraXinputDeadzone);
	iniReader.WriteBoolean("CONTROLLER", "OverrideXinputDeadzone", pConfig->bOverrideXinputDeadzone);
	iniReader.WriteFloat("CONTROLLER", "XinputDeadzone", pConfig->fXinputDeadzone);
	iniReader.WriteBoolean("CONTROLLER", "AllowReloadWithoutAiming", pConfig->bAllowReloadWithoutAiming_controller);
	iniReader.WriteBoolean("CONTROLLER", "ReloadWithoutZoom", pConfig->bReloadWithoutZoom_controller);

	// FRAME RATE
	iniReader.WriteBoolean("FRAME RATE", "FixFallingItemsSpeed", pConfig->bFixFallingItemsSpeed);
	iniReader.WriteBoolean("FRAME RATE", "FixTurningSpeed", pConfig->bFixTurningSpeed);
	iniReader.WriteBoolean("FRAME RATE", "FixQTE", pConfig->bFixQTE);
	iniReader.WriteBoolean("FRAME RATE", "FixAshleyBustPhysics", pConfig->bFixAshleyBustPhysics);
	iniReader.WriteBoolean("FRAME RATE", "EnableFastMath", pConfig->bEnableFastMath);
	iniReader.WriteBoolean("FRAME RATE", "PrecacheModels", pConfig->bPrecacheModels);

	// MISC
	iniReader.WriteBoolean("MISC", "OverrideCostumes", pConfig->bOverrideCostumes);
	iniReader.WriteString("MISC", "LeonCostume", " " + std::string(sLeonCostumeNames[iCostumeComboLeon]));
	iniReader.WriteString("MISC", "AshleyCostume", " " + std::string(sAshleyCostumeNames[iCostumeComboAshley]));
	iniReader.WriteString("MISC", "AdaCostume", " " + std::string(sAdaCostumeNames[iCostumeComboAda]));
	iniReader.WriteBoolean("MISC", "AshleyJPCameraAngles", pConfig->bAshleyJPCameraAngles);
	iniReader.WriteInteger("MISC", "ViolenceLevelOverride", pConfig->iViolenceLevelOverride);
	iniReader.WriteBoolean("MISC", "AllowSellingHandgunSilencer", pConfig->bAllowSellingHandgunSilencer);
	iniReader.WriteBoolean("MISC", "AllowMafiaLeonCutscenes", pConfig->bAllowMafiaLeonCutscenes);
	iniReader.WriteBoolean("MISC", "SilenceArmoredAshley", pConfig->bSilenceArmoredAshley);
	iniReader.WriteBoolean("MISC", "AllowAshleySuplex", pConfig->bAllowAshleySuplex);
	iniReader.WriteBoolean("MISC", "AllowMatildaQuickturn", pConfig->bAllowMatildaQuickturn);
	iniReader.WriteBoolean("MISC", "FixDitmanGlitch", pConfig->bFixDitmanGlitch);
	iniReader.WriteBoolean("MISC", "UseSprintToggle", pConfig->bUseSprintToggle);
	iniReader.WriteBoolean("MISC", "DisableQTE", pConfig->bDisableQTE);
	iniReader.WriteBoolean("MISC", "AutomaticMashingQTE", pConfig->bAutomaticMashingQTE);
	iniReader.WriteBoolean("MISC", "SkipIntroLogos", pConfig->bSkipIntroLogos);
	iniReader.WriteBoolean("MISC", "SkipMenuLogo", pConfig->bSkipMenuLogo);
	iniReader.WriteBoolean("MISC", "EnableDebugMenu", pConfig->bEnableDebugMenu);
	iniReader.WriteBoolean("MISC", "EnableModExpansion", pConfig->bEnableModExpansion);
	iniReader.WriteBoolean("MISC", "DisableAdaKnifeGC", pConfig->bDisableAdaKnifeGC);
	iniReader.WriteBoolean("MISC", "DisableAdaKnifePS2", pConfig->bDisableAdaKnifePS2);

	// MEMORY
	iniReader.WriteBoolean("MEMORY", "AllowHighResolutionSFD", pConfig->bAllowHighResolutionSFD);
	iniReader.WriteBoolean("MEMORY", "RaiseVertexAlloc", pConfig->bRaiseVertexAlloc);
	iniReader.WriteBoolean("MEMORY", "RaiseInventoryAlloc", pConfig->bRaiseInventoryAlloc);

	// HOTKEYS
	iniReader.WriteString("HOTKEYS", "ConfigMenu", " " + pConfig->sConfigMenuKeyCombo);
	iniReader.WriteString("HOTKEYS", "Console", " " + pConfig->sConsoleKeyCombo);
	iniReader.WriteString("HOTKEYS", "FlipItemUp", " " + pConfig->sFlipItemUp);
	iniReader.WriteString("HOTKEYS", "FlipItemDown", " " + pConfig->sFlipItemDown);
	iniReader.WriteString("HOTKEYS", "FlipItemLeft", " " + pConfig->sFlipItemLeft);
	iniReader.WriteString("HOTKEYS", "FlipItemRight", " " + pConfig->sFlipItemRight);
	iniReader.WriteString("HOTKEYS", "QTE_key_1", " " + pConfig->sQTE_key_1);
	iniReader.WriteString("HOTKEYS", "QTE_key_2", " " + pConfig->sQTE_key_2);
	iniReader.WriteString("HOTKEYS", "DebugMenu", " " + pConfig->sDebugMenuKeyCombo);
	iniReader.WriteString("HOTKEYS", "MouseTurningModifier", " " + pConfig->sMouseTurnModifierKeyCombo);
	iniReader.WriteString("HOTKEYS", "JetSkiTricks", " " + pConfig->sJetSkiTrickCombo);

	// IMGUI
	iniReader.WriteFloat("IMGUI", "FontSizeScale", pConfig->fFontSizeScale);
}

DWORD WINAPI WriteSettingsThread(LPVOID lpParameter)
{
	std::string iniPathMain = rootPath + WrapperName.substr(0, WrapperName.find_last_of('.')) + ".ini";
	std::string iniPathTrainer = rootPath + "\\re4_tweaks\\trainer.ini";
	WriteSettings(iniPathMain, false);
	WriteSettings(iniPathTrainer, true);

	pConfig->HasUnsavedChanges = false;

	return 0;
}

void Config::WriteSettings()
{
	std::lock_guard<std::mutex> guard(settingsThreadRunningMutex); // if thread is already running, wait for it to finish

	// Spawn a new thread to handle writing settings, as INI writing funcs that get used are pretty slow
	CreateThreadAutoClose(NULL, 0, WriteSettingsThread, NULL, 0, NULL);
}

void Config::LogSettings()
{
	spd::log()->info("+--------------------------------+-----------------+");
	spd::log()->info("| Setting                        | Value           |");
	spd::log()->info("+--------------------------------+-----------------+");

	// DISPLAY
	spd::log()->info("+ DISPLAY------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "FOVAdditional", pConfig->fFOVAdditional);
	spd::log()->info("| {:<30} | {:>15} |", "DisableVsync", pConfig->bDisableVsync ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "UltraWideAspectSupport", pConfig->bUltraWideAspectSupport ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "SideAlignHUD", pConfig->bSideAlignHUD ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "StretchFullscreenImages", pConfig->bStretchFullscreenImages ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "StretchVideos", pConfig->bStretchVideos ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "Remove16by10BlackBars", pConfig->bRemove16by10BlackBars ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "ReplaceFramelimiter", pConfig->bReplaceFramelimiter ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixDPIScale", pConfig->bFixDPIScale ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixDisplayMode", pConfig->bFixDisplayMode ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "CustomRefreshRate", pConfig->iCustomRefreshRate);
	spd::log()->info("| {:<30} | {:>15} |", "OverrideLaserColor", pConfig->bOverrideLaserColor ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "RainbowLaser", pConfig->bRainbowLaser ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "LaserR", pConfig->iLaserR);
	spd::log()->info("| {:<30} | {:>15} |", "LaserG", pConfig->iLaserG);
	spd::log()->info("| {:<30} | {:>15} |", "LaserB", pConfig->iLaserB);
	spd::log()->info("| {:<30} | {:>15} |", "RestorePickupTransparency", pConfig->bRestorePickupTransparency ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DisableBrokenFilter03", pConfig->bDisableBrokenFilter03 ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixBlurryImage", pConfig->bFixBlurryImage ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DisableFilmGrain", pConfig->bDisableFilmGrain ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "EnableGCBlur", pConfig->bEnableGCBlur ? "true" : "false");
	
	if (pConfig->bUseEnhancedGCBlur)
		spd::log()->info("| {:<30} | {:>15} |", "GCBlurType", "Enhanced");
	else
		spd::log()->info("| {:<30} | {:>15} |", "GCBlurType", "Classic");

	spd::log()->info("| {:<30} | {:>15} |", "EnableGCScopeBlur", pConfig->bEnableGCScopeBlur ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "WindowBorderless", pConfig->bWindowBorderless ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "WindowPositionX", pConfig->iWindowPositionX);
	spd::log()->info("| {:<30} | {:>15} |", "WindowPositionY", pConfig->iWindowPositionY);
	spd::log()->info("| {:<30} | {:>15} |", "RememberWindowPos", pConfig->bRememberWindowPos ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// AUDIO
	spd::log()->info("+ AUDIO--------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "VolumeMaster", pConfig->iVolumeMaster);
	spd::log()->info("| {:<30} | {:>15} |", "VolumeBGM", pConfig->iVolumeBGM);
	spd::log()->info("| {:<30} | {:>15} |", "VolumeSE", pConfig->iVolumeSE);
	spd::log()->info("| {:<30} | {:>15} |", "VolumeCutscene", pConfig->iVolumeCutscene);
	spd::log()->info("+--------------------------------+-----------------+");

	// MOUSE
	spd::log()->info("+ MOUSE--------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "CameraImprovements", pConfig->bCameraImprovements ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "ResetCameraWhenRunning", pConfig->bResetCameraWhenRunning ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "CameraSensitivity", pConfig->fCameraSensitivity);
	spd::log()->info("| {:<30} | {:>15} |", "UseMouseTurning", pConfig->bUseMouseTurning ? "true" : "false");

	if (pConfig->iMouseTurnType == MouseTurnTypes::TypeA)
		spd::log()->info("| {:<30} | {:>15} |", "MouseTurnType", "TypeA");
	else
		spd::log()->info("| {:<30} | {:>15} |", "MouseTurnType", "TypeB");

	spd::log()->info("| {:<30} | {:>15} |", "TurnTypeBSensitivity", pConfig->fTurnTypeBSensitivity);
	spd::log()->info("| {:<30} | {:>15} |", "UseRawMouseInput", pConfig->bUseRawMouseInput ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DetachCameraFromAim", pConfig->bDetachCameraFromAim ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixSniperZoom", pConfig->bFixSniperZoom ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixSniperFocus", pConfig->bFixSniperFocus ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixRetryLoadMouseSelector", pConfig->bFixRetryLoadMouseSelector ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// KEYBOARD
	spd::log()->info("+ KEYBOARD-----------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "FallbackToEnglishKeyIcons", pConfig->bFallbackToEnglishKeyIcons ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "AllowReloadWithoutAiming", pConfig->bAllowReloadWithoutAiming_kbm ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "ReloadWithoutZoom", pConfig->bReloadWithoutZoom_kbm ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// CONTROLLER
	spd::log()->info("+ CONTROLLER---------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "OverrideControllerSensitivity", pConfig->bOverrideControllerSensitivity ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "ControllerSensitivity", pConfig->fControllerSensitivity);
	spd::log()->info("| {:<30} | {:>15} |", "RemoveExtraXinputDeadzone", pConfig->bRemoveExtraXinputDeadzone ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "OverrideXinputDeadzone", pConfig->bOverrideXinputDeadzone ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "XinputDeadzone", pConfig->fXinputDeadzone);
	spd::log()->info("| {:<30} | {:>15} |", "AllowReloadWithoutAiming", pConfig->bAllowReloadWithoutAiming_controller ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "ReloadWithoutZoom", pConfig->bReloadWithoutZoom_controller ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// FRAME RATE
	spd::log()->info("+ FRAME RATE---------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "FixFallingItemsSpeed", pConfig->bFixFallingItemsSpeed ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixTurningSpeed", pConfig->bFixTurningSpeed ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixQTE", pConfig->bFixQTE ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixAshleyBustPhysics", pConfig->bFixAshleyBustPhysics ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "EnableFastMath", pConfig->bEnableFastMath ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "PrecacheModels", pConfig->bPrecacheModels ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// MISC
	spd::log()->info("+ MISC---------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "WrappedDllPath", pConfig->sWrappedDllPath.data());
	spd::log()->info("| {:<30} | {:>15} |", "OverrideCostumes", pConfig->bOverrideCostumes ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "LeonCostume", sLeonCostumeNames[iCostumeComboLeon]);
	spd::log()->info("| {:<30} | {:>15} |", "AshleyCostume", sAshleyCostumeNames[iCostumeComboAshley]);
	spd::log()->info("| {:<30} | {:>15} |", "AdaCostume", sAdaCostumeNames[iCostumeComboAda]);
	spd::log()->info("| {:<30} | {:>15} |", "AshleyJPCameraAngles", pConfig->bAshleyJPCameraAngles ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "ViolenceLevelOverride", pConfig->iViolenceLevelOverride);
	spd::log()->info("| {:<30} | {:>15} |", "AllowSellingHandgunSilencer", pConfig->bAllowSellingHandgunSilencer ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "AllowMafiaLeonCutscenes", pConfig->bAllowMafiaLeonCutscenes ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "SilenceArmoredAshley", pConfig->bSilenceArmoredAshley ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "AllowAshleySuplex", pConfig->bAllowAshleySuplex ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "AllowMatildaQuickturn", pConfig->bAllowMatildaQuickturn ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "FixDitmanGlitch", pConfig->bFixDitmanGlitch ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "UseSprintToggle", pConfig->bUseSprintToggle ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DisableQTE", pConfig->bDisableQTE ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "AutomaticMashingQTE", pConfig->bAutomaticMashingQTE ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "SkipIntroLogos", pConfig->bSkipIntroLogos ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "SkipMenuLogo", pConfig->bSkipMenuLogo ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "EnableDebugMenu", pConfig->bEnableDebugMenu ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "EnableModExpansion", pConfig->bEnableModExpansion ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DisableAdaKnifeGC", pConfig->bDisableAdaKnifeGC ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DisableAdaKnifePS2", pConfig->bDisableAdaKnifePS2 ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// MEMORY
	spd::log()->info("+ MEMORY-------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "AllowHighResolutionSFD", pConfig->bAllowHighResolutionSFD ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "RaiseVertexAlloc", pConfig->bRaiseVertexAlloc ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "RaiseInventoryAlloc", pConfig->bRaiseInventoryAlloc ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// HOTKEYS
	spd::log()->info("+ HOTKEYS------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "ConfigMenu", pConfig->sConfigMenuKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "Console", pConfig->sConsoleKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "FlipItemUp", pConfig->sFlipItemUp.data());
	spd::log()->info("| {:<30} | {:>15} |", "FlipItemDown", pConfig->sFlipItemDown.data());
	spd::log()->info("| {:<30} | {:>15} |", "FlipItemLeft", pConfig->sFlipItemLeft.data());
	spd::log()->info("| {:<30} | {:>15} |", "FlipItemRight", pConfig->sFlipItemRight.data());
	spd::log()->info("| {:<30} | {:>15} |", "QTE_key_1", pConfig->sQTE_key_1.data());
	spd::log()->info("| {:<30} | {:>15} |", "QTE_key_2", pConfig->sQTE_key_2.data());
	spd::log()->info("| {:<30} | {:>15} |", "DebugMenu", pConfig->sDebugMenuKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "MouseTurningModifier", pConfig->sMouseTurnModifierKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "JetSkiTricks", pConfig->sJetSkiTrickCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "FocusUI", pConfig->sTrainerFocusUIKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "NoclipToggle", pConfig->sTrainerNoclipKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "SpeedOverrideToggle", pConfig->sTrainerSpeedOverrideKeyCombo.data());
	spd::log()->info("| {:<30} | {:>15} |", "MoveAshleyToPlayer", pConfig->sTrainerMoveAshToPlayerKeyCombo.data());
	spd::log()->info("+--------------------------------+-----------------+");

	// FPS WARNING
	spd::log()->info("+ WARNING------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "IgnoreFPSWarning", pConfig->bIgnoreFPSWarning ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// IMGUI
	spd::log()->info("+ IMGUI--------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "FontSizeScale", pConfig->fFontSizeScale);
	spd::log()->info("| {:<30} | {:>15} |", "DisableMenuTip", pConfig->bDisableMenuTip ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");

	// DEBUG
	spd::log()->info("+ DEBUG--------------------------+-----------------+");
	spd::log()->info("| {:<30} | {:>15} |", "VerboseLog", pConfig->bVerboseLog ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "NeverHideCursor", pConfig->bNeverHideCursor ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "UseDynamicFrametime", pConfig->bUseDynamicFrametime ? "true" : "false");
	spd::log()->info("| {:<30} | {:>15} |", "DisableFramelimiting", pConfig->bDisableFramelimiting ? "true" : "false");
	spd::log()->info("+--------------------------------+-----------------+");
}