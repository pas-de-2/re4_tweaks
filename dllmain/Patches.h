#pragma once
#include <d3d9.h>
#include <imgui.h>

extern std::string WrapperName;
extern std::string rootPath;
extern std::string logPath;
extern HWND hWindow;

// Init functions
void Init_60fpsFixes();
void Init_AspectRatioTweaks();
void Init_AudioTweaks();
void Init_CameraTweaks();
void Init_ControllerTweaks();
void Init_CommandLine();
void Init_DebugDisplay();
void Init_D3D9Hook();
void Init_DisplayTweaks();
void Init_ExceptionHandler();
void Init_FilterXXFixes();
bool Init_Game();
void Init_HandleLimits();
void Init_HDProject();
void Init_KeyboardMouseTweaks();
void Init_MouseTurning();
void Init_MathReimpl();
void Init_Misc();
void Init_ModExpansion();
void Init_QTEfixes();
void Init_sofdec();
void Init_ToolMenu();
void Init_ToolMenuDebug();
void Init_WndProcHook();

// Hotkey bindings and parsing
bool ParseConsoleKeyCombo(std::string_view in_combo);
bool ParseConfigMenuKeyCombo(std::string_view in_combo);
bool ParseMouseTurnModifierCombo(std::string_view in_combo);
bool ParseToolMenuKeyCombo(std::string_view in_combo);
bool ParseJetSkiTrickCombo(std::string_view in_combo);

// Audio Tweaks
void AudioTweaks_UpdateVolume();

// EndSceneHook
struct EndSceneHook
{
	void EndScene_hook(LPDIRECT3DDEVICE9 pDevice);

	std::chrono::high_resolution_clock::duration _last_frame_duration;
	std::chrono::high_resolution_clock::time_point _start_time;
	std::chrono::high_resolution_clock::time_point _last_present_time;

	float _cur_monitor_dpi = 1.0f;

	ImFont* ESP_font;

	ImGuiContext* _imgui_context = nullptr;
};

extern EndSceneHook esHook;
extern bool bRebuildFont;
bool ParseImGuiUIFocusCombo(std::string_view in_combo);

// cfgMenu
void cfgMenuRender();
void ShowCfgMenuTip();

extern bool bCfgMenuOpen;
extern bool bWaitingForHotkey;

extern float fLaserColorPicker[3];

extern int iGCBlurMode;
extern int iCostumeComboLeon;
extern int iCostumeComboAshley;
extern int iCostumeComboAda;

// ConsoleWnd
struct ConsoleOutput
{
	std::string TitleKeyCombo;
	void ShowConsoleOutput();
	void AddLogChar(const char* fmt, ...);
	void AddLogHex(int fmt, ...);
	void AddLogInt(int fmt, ...);
	void AddLogFloat(float fmt, ...);
	void AddConcatLog(const char* fmt, int value, ...);
	void AddConcatLog(const char* fmt, float value, ...);
	void AddConcatLog(const char* fmt, double value, ...);
	void AddConcatLog(const char* fmt, const char* value, ...);
	void Clear();
};

extern ConsoleOutput con;
extern bool bConsoleOpen;

// LAApatch.cpp
void LAACheck();

// Controller vars
extern int* g_XInputDeadzone_LS;
extern int* g_XInputDeadzone_RS;

extern int8_t* AnalogRX_8;
extern int8_t* AnalogRY_9;

// Mouse vars
extern float* fMousePosX;
extern float* fMousePosY;
extern int32_t* MouseDeltaX;
extern int32_t* MouseDeltaY;

enum MouseTurnTypes
{
	TypeA,
	TypeB
};

// Camera vars
extern float* fCameraPosX;
extern float* fCameraPosY;

extern float* wepPitch;