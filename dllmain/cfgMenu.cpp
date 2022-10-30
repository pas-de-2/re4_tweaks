﻿#define NOMINMAX
#include <iostream>
#include "dllmain.h"
#include "Patches.h"
#include "Settings.h"
#include "imgui.h"
#include "imgui\misc\cpp\imgui_stdlib.h"
#include "input.hpp"
#include "Patches.h"
#include <FAhashes.h>
#include "Utils.h"
#include "UI_DebugWindows.h"
#include "UI_Utility.h"
#include "Trainer.h"

bool bCfgMenuOpen;
bool NeedsToRestart;
bool bWaitingForHotkey;

int iGCBlurMode = 0;
int iCostumeComboLeon;
int iCostumeComboAshley;
int iCostumeComboAda;

float fLaserColorPicker[3]{ 0.0f, 0.0f, 0.0f };

ImVec2 PrevRightColumnPos;
ImVec2 PrevLeftColumnPos;

int MenuTipTimer = 500; // cfgMenu tooltip timer

std::string cfgMenuTitle = "re4_tweaks";

std::vector<uint32_t> cfgMenuCombo;

bool ParseConfigMenuKeyCombo(std::string_view in_combo)
{
	if (in_combo.empty())
		return false;

	cfgMenuCombo.clear();
	cfgMenuCombo = ParseKeyCombo(in_combo);

	pInput->RegisterHotkey({ []() {
		bCfgMenuOpen = !bCfgMenuOpen;
	}, &cfgMenuCombo });

	return cfgMenuCombo.size() > 0;
}

void cfgMenuRender()
{
	// Min/Max window sizes
	const float min_x = 940.0f * esHook._cur_monitor_dpi;
	const float min_y = 640.0f * esHook._cur_monitor_dpi;

	const float max_x = 1920.0f * esHook._cur_monitor_dpi;
	const float max_y = 1080.0f * esHook._cur_monitor_dpi;

	ImGui::SetNextWindowSizeConstraints(ImVec2(min_x, min_y), ImVec2(max_x, max_y));

	ImGui::Begin("cfgMenu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
	{
		auto pos = ImGui::GetWindowPos();
		auto draw = ImGui::GetWindowDrawList();

		float leftside_size_x = 200; // Base
		leftside_size_x *= esHook._cur_monitor_dpi;

		float topright_size_y = 71; // Base
		topright_size_y *= pConfig->fFontSizeScale;
		topright_size_y *= esHook._cur_monitor_dpi;

		// Left side BG
		draw->AddRectFilled(ImVec2(pos.x + 0, pos.y + 0), ImVec2(pos.x + leftside_size_x + 1, pos.y + ImGui::GetWindowHeight()), ImColor(8, 8, 8, 230), 10.f, 5);

		// Right side BG
		draw->AddRectFilled(ImVec2(pos.x + leftside_size_x, pos.y + 0), ImVec2(pos.x + ImGui::GetWindowWidth(), pos.y + ImGui::GetWindowHeight()), ImColor(8, 8, 8, 250), 8.f, 10);

		// Top right separator
		draw->AddLine(ImVec2(pos.x + leftside_size_x + 10, round(pos.y + topright_size_y)), ImVec2(pos.x + ImGui::GetItemRectSize().x - 15, round(pos.y + topright_size_y)), ImColor(150, 10, 40));

		// Setup tab buttons
		{
			ImGui::BeginChild("left side", ImVec2(leftside_size_x, 0));

			ImColor icn_color = ImColor(230, 15, 95);

			ImVec4 active = ImVec4(150.0f / 255.0f, 10.0f / 255.0f, 40.0f / 255.0f, 255.0f / 255.0f);
			ImVec4 inactive = ImVec4(31.0f / 255.0f, 30.0f / 255.0f, 31.0f / 255.0f, 0.0f / 255.0f);

			ImVec2 btn_size = ImVec2(leftside_size_x - 28, 31 * pConfig->fFontSizeScale * esHook._cur_monitor_dpi);

			// cfgMenu title
			ImGui::SetCursorPos(ImVec2(12, 25));

			ImGui::Text(cfgMenuTitle.data());

			ImGui::Dummy(ImVec2(30, 30 * esHook._cur_monitor_dpi));

			// Display
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##display", "Display", active, inactive, MenuTab::Display, ICON_FA_DESKTOP, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Audio
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##audio", "Audio", active, inactive, MenuTab::Audio, ICON_FA_HEADPHONES, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Mouse
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##mouse", "Mouse", active, inactive, MenuTab::Mouse, ICON_FA_LOCATION_ARROW, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Keyboard
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##keyboard", "Keyboard", active, inactive, MenuTab::Keyboard, ICON_FA_KEYBOARD, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Controller
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##controller", "Controller", active, inactive, MenuTab::Controller, ICON_FA_JOYSTICK, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Frame rate
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##framerate", "Frame rate", active, inactive, MenuTab::Framerate, ICON_FA_CHESS_CLOCK, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Misc
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##misc", "Misc", active, inactive, MenuTab::Misc, ICON_FA_COG, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Memory
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##memory", "Memory", active, inactive, MenuTab::Memory, ICON_FA_CALCULATOR, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Hotkeys
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##hotkeys", "Hotkeys", active, inactive, MenuTab::Hotkeys, ICON_FA_LAMBDA, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			if (!pConfig->bTrainerEnable)
			{
				if (ImGui_TabButton("##trainer", "Trainer", active, inactive, MenuTab::Trainer, ICON_FA_SHOE_PRINTS, ImColor(255, 255, 255, 60), ImColor(255, 255, 255, 60), btn_size))
					ImGui::OpenPopup("Trainer menu");

				// Always center this window when appearing
				ImVec2 center = ImGui::GetMainViewport()->GetCenter();
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

				if (ImGui::BeginPopupModal("Trainer menu", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Warning: this section contains cheats & patches that may ruin your game experience,\nor break things in the game.\n\nAre you sure you want to use the trainer menu?\n\n");
					ImGui::Separator();

					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 265, ImGui::GetCursorPos().y));

					if (ImGui::Button("Yes", ImVec2(120, 0)))
					{
						pConfig->bTrainerEnable = true;
						pConfig->WriteSettings();
						ImGui::CloseCurrentPopup();
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();

					if (ImGui::Button("Cancel", ImVec2(120, 0)))
					{
						Tab = MenuTab::Display;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

			}
			else
				ImGui_TabButton("##trainer", "Trainer", active, inactive, MenuTab::Trainer, ICON_FA_SHOE_PRINTS, icn_color, IM_COL32_WHITE, btn_size);

			ImGui::Dummy(ImVec2(0.0f, 12.0f * esHook._cur_monitor_dpi));

			// Save/Load
			float sl_btn_size_x = (leftside_size_x / 2) - 18;
			float sl_btn_size_y = 35.0f;
			sl_btn_size_y *= pConfig->fFontSizeScale;
			sl_btn_size_y *= esHook._cur_monitor_dpi;

			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 10 - sl_btn_size_x, ImGui::GetContentRegionAvail().y - 10 - sl_btn_size_y));

			// Separator
			draw->AddLine(ImVec2(ImGui::GetCursorScreenPos().x + 10, ImGui::GetCursorScreenPos().y - 10), ImVec2(ImGui::GetCursorScreenPos().x + leftside_size_x - 20, ImGui::GetCursorScreenPos().y - 10), ImColor(150, 10, 40));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.51f, 0.00f, 0.14f, 0.00f));
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			if (ImGui::Button(ICON_FA_ERASER" Discard", ImVec2(sl_btn_size_x, sl_btn_size_y)))
			{
				float oldSize = pConfig->fFontSizeScale;

				pConfig->ReadSettings();

				if (oldSize != pConfig->fFontSizeScale)
					bRebuildFont = true;
			}

			ImGui::SameLine();

			if (ImGui::Button(ICON_FA_CODE" Save", ImVec2(sl_btn_size_x, sl_btn_size_y)))
			{
				// Parse key combos on save
				pConfig->ParseHotkeys();

				// Update console title
				con.TitleKeyCombo = pConfig->sConsoleKeyCombo;

				pConfig->WriteSettings();
			}

			ImGui::PopStyleColor();

			ImGui::EndChild();
		}

		ImGui::SameLine();

		// Setup top right
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.51f, 0.00f, 0.14f, 0.00f));

			ImGui::BeginChild("top right", ImVec2(0, topright_size_y));

			// Config menu font size
			if (ImGui::Button("-"))
			{
				if (pConfig->fFontSizeScale > 1.0f)
				{
					pConfig->fFontSizeScale -= 0.05f;

					bRebuildFont = true;
					pConfig->HasUnsavedChanges = true;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("+"))
			{
				if (pConfig->fFontSizeScale < 1.20f)
				{
					pConfig->fFontSizeScale += 0.05f;
					bRebuildFont = true;
					pConfig->HasUnsavedChanges = true;
				}
			}

			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Font Size");

			ImGui::SameLine();

			// Tips
			float tip_offset = 45.0f;
			tip_offset *= pConfig->fFontSizeScale;
			tip_offset *= esHook._cur_monitor_dpi;

			if (pConfig->HasUnsavedChanges)
			{
				const char *txt = "You have unsaved changes!";

				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(txt).x - tip_offset, ImGui::GetContentRegionAvail().y / 2));
				ImGui::SameLine();
				ImGui::BulletText(txt);
			}

			if (NeedsToRestart)
			{
				const char* txt = "Changes that have been made require the game to be restarted!";

				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(txt).x - tip_offset, ImGui::GetContentRegionAvail().y / 6));
				ImGui::SameLine();
				ImGui::BulletText(txt);
			}

			ImGui::EndChild();
		}

		// Setup item view
		{
			ImGui::SetCursorPos(ImVec2(leftside_size_x + 16, topright_size_y + 10));

			ImGui::BeginChild("right side", ImVec2(0, 0));

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(16.f, 16.f));

			int columnCount = 1 + ((ImGui::GetWindowWidth() / (716.0f * esHook._cur_monitor_dpi)) * 1.5f);

			ImColor itmbgColor = ImColor(25, 20, 20, 166);

			if (Tab == MenuTab::Display)
			{
				if (ImGui::BeginTable("Display", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// FOVAdditional
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("FOVAdditional", &pConfig->bEnableFOV))
						{
							pConfig->HasUnsavedChanges = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Additional FOV value.");
						ImGui::TextWrapped("20 seems good for most cases.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("FOV Slider").x);
						ImGui::BeginDisabled(!pConfig->bEnableFOV);
						ImGui::SliderFloat("FOV Slider", &pConfig->fFOVAdditional, 0.0f, 50.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (!pConfig->bEnableFOV)
							pConfig->fFOVAdditional = 0.0f;
					}

					// DisableVsync
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("DisableVsync", &pConfig->bDisableVsync))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Force V-Sync to be disabled. For some reason the vanilla game doesn't provide a functional way to do this.");
					}

					// Aspect ratio tweaks
					{
						ImGui_ColumnSwitch();

						ImGui::Spacing();
						ImGui::TextWrapped("Aspect ratio tweaks");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("UltraWideAspectSupport", &pConfig->bUltraWideAspectSupport);
						ImGui::TextWrapped("Fixes the incorrect aspect ratio when playing in ultrawide resolutions, preventing the image from being cut off and the HUD appearing off-screen.");
						ImGui::TextWrapped("(Change the resolution for this setting to take effect)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::BeginDisabled(!pConfig->bUltraWideAspectSupport);
						pConfig->HasUnsavedChanges |= ImGui::Checkbox("SideAlignHUD", &pConfig->bSideAlignHUD);
						ImGui::TextWrapped("Moves the HUD to the right side of the screen.");

						ImGui::Spacing();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("StretchFullscreenImages", &pConfig->bStretchFullscreenImages);
						ImGui::TextWrapped("Streches some images to fit the screen, such as the images shown when reading \"Files\".");

						ImGui::Spacing();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("StretchVideos", &pConfig->bStretchVideos);
						ImGui::TextWrapped("Streches pre-rendered videos to fit the screen.");
						ImGui::EndDisabled();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("Remove16by10BlackBars", &pConfig->bRemove16by10BlackBars);
						ImGui::TextWrapped("Removes top and bottom black bars that are present when playing in 16:10. Will crop a few pixels from each side of the screen.");
						ImGui::TextWrapped("(Change the resolution for this setting to take effect)");
					}

					// ReplaceFramelimiter
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("ReplaceFramelimiter", &pConfig->bReplaceFramelimiter))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Replaces the games 60/30FPS framelimiter with our own version, which reduces CPU usage quite a lot.");
						ImGui::TextWrapped("(experimental, not known if the new framelimiter performs the same as the old one yet)");
					}

					// FixDPIScale
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("FixDPIScale", &pConfig->bFixDPIScale))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Forces game to run at normal 100%% DPI scaling, fixes resolution issues for players that have above 100%% DPI scaling set.");
					}

					// FixDisplayMode
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("FixDisplayMode", &pConfig->bFixDisplayMode))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows game to use non - 60Hz refresh rates in fullscreen, fixing the black screen issue people have when starting the game.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::Text("CustomRefreshRate");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(100 * pConfig->fFontSizeScale * esHook._cur_monitor_dpi);
						ImGui::BeginDisabled(!pConfig->bFixDisplayMode);
						ImGui::InputInt("Hz", &pConfig->iCustomRefreshRate);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (ImGui::IsItemEdited())
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui::Spacing();

						ImGui::TextWrapped("Determines a custom refresh rate for the game to use.");
						ImGui::TextWrapped("Requires FixDisplayMode to be enabled.");
						ImGui::TextWrapped("-1 will make it try to use the current refresh rate as reported by Windows.");
					}

					// OverrideLaserColor
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("OverrideLaserColor", &pConfig->bOverrideLaserColor);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Overrides the color of the laser sights.");
						ImGui::TextWrapped("Note: The game's vanilla config.ini contains a setting to change the");
						ImGui::TextWrapped("alpha/opacity of the laser, but since it doesn't work, we don't include it here.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(!pConfig->bOverrideLaserColor || pConfig->bRainbowLaser);
						ImGui::ColorEdit4("Laser color picker", fLaserColorPicker, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
						if (ImGui::IsItemEdited())
						{
							pConfig->iLaserR = (int)(fLaserColorPicker[0] * 255.0f);
							pConfig->iLaserG = (int)(fLaserColorPicker[1] * 255.0f);
							pConfig->iLaserB = (int)(fLaserColorPicker[2] * 255.0f);
						}
						ImGui::EndDisabled();

						ImGui::Spacing();

						ImGui::BeginDisabled(!pConfig->bOverrideLaserColor);
						pConfig->HasUnsavedChanges |= ImGui::Checkbox("RainbowLaser", &pConfig->bRainbowLaser);
						ImGui::EndDisabled();

						if (!pConfig->bOverrideLaserColor)
							pConfig->bRainbowLaser = false;
					}

					// RestorePickupTransparency
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("RestorePickupTransparency", &pConfig->bRestorePickupTransparency);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Restores transparency on the item pickup screeen.");
					}

					// DisableBrokenFilter03
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("DisableBrokenFilter03", &pConfig->bDisableBrokenFilter03);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("This filter was originally meant to add an extra glow effect on certain fire sources, but it was broken when the game was ported to the Xbox 360, making the entire image have an orange tint overlay applied to it.");
						ImGui::TextWrapped("(if you're using the HD Project, you should disable this option)");
					}

					// FixBlurryImage
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixBlurryImage", &pConfig->bFixBlurryImage);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Fixes a problem related to a vertex buffer that caused the image to be slightly blurred, making the image much sharper and clearer.");
					}

					// DisableFilmGrain
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("DisableFilmGrain", &pConfig->bDisableFilmGrain);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Disables the film grain overlay that is present in most sections of the game.");
					}

					// EnableGCBlur
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("EnableGCBlur", &pConfig->bEnableGCBlur))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Restores DoF blurring from the GC version, which was removed/unimplemented in later ports.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						if (ImGui::RadioButton("Enhanced", &iGCBlurMode, 0))
							pConfig->bUseEnhancedGCBlur = true;
						ImGui::Indent(29.0);
						ImGui::TextWrapped("Slightly improved implementation to look better on modern high-definition displays.");
						ImGui::Unindent(29.0);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						if (ImGui::RadioButton("Classic", &iGCBlurMode, 1))
							pConfig->bUseEnhancedGCBlur = false;
						ImGui::Indent(29.0);
						ImGui::TextWrapped("The original GC implementation.");
						ImGui::Unindent(29.0);

					}

					// EnableGCScopeBlur
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("EnableGCScopeBlur", &pConfig->bEnableGCScopeBlur))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Restores outer-scope blurring when using a scope, which was removed/unimplemented in later ports.");
					}

					// WindowBorderless
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("WindowBorderless", &pConfig->bWindowBorderless))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Whether to use a borderless-window when using windowed-mode.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Position to draw the game window when using windowed mode.");
						ImGui::TextWrapped("-1 will use the games default (usually places it at 0,0)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(pConfig->bRememberWindowPos);
						ImGui::PushItemWidth(150 * pConfig->fFontSizeScale * esHook._cur_monitor_dpi);
						ImGui::InputInt("X Pos", &pConfig->iWindowPositionX);
						ImGui::InputInt("Y Pos", &pConfig->iWindowPositionY);
						ImGui::PopItemWidth();
						ImGui::EndDisabled();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						pConfig->HasUnsavedChanges |= ImGui::Checkbox("RememberWindowPos", &pConfig->bRememberWindowPos);
						ImGui::TextWrapped("Remember the last window position. This automatically updates the \"X Pos\" and \"Y Pos\" values above.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}
			
			if (Tab == MenuTab::Audio)
			{
				if (ImGui::BeginTable("Display", 2, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// Audio volume
					{
						ImGui_ColumnSwitch();

						ImGui::Spacing();
						ImGui::TextWrapped("Adjust the volume of the game");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Cutscene Volume will also affect volume of merchant screen dialogue, as that seems to be handled the same way as cutscenes.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Cutscene Volume").x);
						bool changed = ImGui::SliderInt("Master Volume", &pConfig->iVolumeMaster, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						changed |= ImGui::SliderInt("Music Volume", &pConfig->iVolumeBGM, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						changed |= ImGui::SliderInt("Effect Volume", &pConfig->iVolumeSE, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						changed |= ImGui::SliderInt("Cutscene Volume", &pConfig->iVolumeCutscene, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						ImGui::PopItemWidth();

						if (changed)
						{
							pConfig->HasUnsavedChanges = true;
							AudioTweaks_UpdateVolume();
						}
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}
						
			if (Tab == MenuTab::Mouse)
			{
				if (ImGui::BeginTable("Mouse", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// CameraImprovements
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("CameraImprovements", &pConfig->bCameraImprovements);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Provides a small set of camera tweaks:");

						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("Aim weapons in the direction the camera is pointing to");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("Stop camera from being randomly reset");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("Allow full 360 degree movement");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("Smoother and more responsive movement");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::BeginDisabled(!pConfig->bCameraImprovements);
						pConfig->HasUnsavedChanges |= ImGui::Checkbox("ResetCameraWhenRunning", &pConfig->bResetCameraWhenRunning);
						ImGui::EndDisabled();
						ImGui::TextWrapped("Center the camera when the run key is pressed.");
						ImGui::TextWrapped("Only used if MouseTurning isn't enabled.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Camera sensitivity").x);
						ImGui::BeginDisabled(!pConfig->bCameraImprovements);
						ImGui::SliderFloat("Camera sensitivity", &pConfig->fCameraSensitivity, 0.50f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();
					}

					// UseMouseTurning
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("UseMouseTurning", &pConfig->bUseMouseTurning);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Provides alternative ways to turn the character using the mouse.");
						ImGui::TextWrapped("\"Modern\" aiming mode in the game's settings is recommended.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::RadioButton("Type A", &pConfig->iMouseTurnType, MouseTurnTypes::TypeA);
						ImGui::Indent(29.0);
						ImGui::TextWrapped("The character's rotation is influenced by the camera's position, similar to Resident Evil 6.");
						ImGui::TextWrapped("Using CameraImprovements is recommended.");
						ImGui::Unindent(29.0);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::RadioButton("Type B", &pConfig->iMouseTurnType, MouseTurnTypes::TypeB);
						ImGui::Indent(29.0);
						ImGui::TextWrapped("The character's rotation is directly changed by the mouse, similar to Resident Evil 5.");
						ImGui::Unindent(29.0);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Type B sensitivity").x - 60);
						ImGui::BeginDisabled(!(pConfig->bUseMouseTurning && (pConfig->iMouseTurnType == MouseTurnTypes::TypeB)));
						ImGui::Indent(29.0);
						ImGui::SliderFloat("Type B sensitivity", &pConfig->fTurnTypeBSensitivity, 0.50f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();
						ImGui::Unindent(29.0);
					}

					// UseRawMouseInput
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("UseRawMouseInput", &pConfig->bUseRawMouseInput);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Makes the game use Raw Input for aiming and turning (if MouseTurning is enabled).");
						ImGui::TextWrapped("Greatly improves mouse input by removing negative/positive accelerations that were being applied both by the game and by Direct Input.");
						ImGui::TextWrapped("This option automatically enables the game's \"Modern\" aiming mode, and is incompatible with the \"Classic\" mode.");
					}

					// DetachCameraFromAim
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("DetachCameraFromAim", &pConfig->bDetachCameraFromAim);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("When using the \"Modern\" mouse setting, the game locks the camera position to the aiming position, making both move together.");
						ImGui::TextWrapped("Although this is the expected behavior in most games, some people might prefer to keep the original camera behavior while also having the benefits from \"Modern\" aiming.");
						ImGui::TextWrapped("Enabling this will also restore the horizontal aiming sway that was lost when the devs implemented \"Modern\" aiming.");
					}

					// FixSniperZoom
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixSniperZoom", &pConfig->bFixSniperZoom);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Prevents the camera from being randomly displaced after you zoom with a sniper rifle when using keyboard and mouse.");
					}

					// FixSniperFocus
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("FixSniperFocus", &pConfig->bFixSniperFocus))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("When zooming in and out with the sniper rifle, this option makes the \"focus\" animation look similar to how it looks like with a controller.");
						ImGui::TextWrapped("Requires EnableGCBlur to be enabled.");
					}

					// FixRetryLoadMouseSelector
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixRetryLoadMouseSelector", &pConfig->bFixRetryLoadMouseSelector);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Prevents the game from overriding your selection in the \"Retry / Load\" screen when moving the mouse before confirming an action.");
						ImGui::TextWrapped("This bug usually causes people to return to the main menu by mistake, when they actually wanted to just restart from the last checkpoint.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Keyboard)
			{
				if (ImGui::BeginTable("Keyboard", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// FallbackToEnglishKeyIcons
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("FallbackToEnglishKeyIcons", &pConfig->bFallbackToEnglishKeyIcons))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Game will turn keys invisible for certain unsupported keyboard languages, enabling this should make game use English keys for unsupported ones instead (requires game restart)");
					}

					// AllowReloadWithoutAiming_kbm
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("AllowReloadWithoutAiming", &pConfig->bAllowReloadWithoutAiming_kbm))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Removes the need to be aiming the weapon before you can reload it.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("ReloadWithoutZoom", &pConfig->bReloadWithoutZoom_kbm);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Don't zoom in when reloading without aiming.");
						ImGui::TextWrapped("Not usually recomended, since the zooming in and out masks some animation quirks when the reload ends.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Controller)
			{
				if (ImGui::BeginTable("Controller", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// OverrideControllerSensitivity
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("OverrideControllerSensitivity", &pConfig->bOverrideControllerSensitivity);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Change the controller sensitivity. For some reason the vanilla game doesn't have an option to change it for controllers, only for the mouse.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Sensitivity Slider").x);
						ImGui::BeginDisabled(!pConfig->bOverrideControllerSensitivity);
						ImGui::SliderFloat("Sensitivity Slider", &pConfig->fControllerSensitivity, 0.50f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (!pConfig->bOverrideControllerSensitivity)
							pConfig->fControllerSensitivity = 1.0f;
					}

					// RemoveExtraXinputDeadzone
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("RemoveExtraXinputDeadzone", &pConfig->bRemoveExtraXinputDeadzone);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Removes unnecessary deadzones that were added for Xinput controllers.");
					}

					// EnableDeadzoneOverride
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("EnableDeadzoneOverride", &pConfig->bOverrideXinputDeadzone);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Change the Xinput controller deadzone.");
						ImGui::TextWrapped("(Unrelated to the previous option)");
						ImGui::TextWrapped("The game's default is 1, but that seems unnecessarily large, so we default to 0.4 instead.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Deadzone Slider").x);
						ImGui::BeginDisabled(!pConfig->bOverrideXinputDeadzone);
						ImGui::SliderFloat("Deadzone Slider", &pConfig->fXinputDeadzone, 0.0f, 3.5f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (ImGui::IsItemEdited())
							pConfig->HasUnsavedChanges = true;

						if (!pConfig->bOverrideXinputDeadzone)
							pConfig->fXinputDeadzone = 1.0f;
					}

					// AllowReloadWithoutAiming_controller
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("AllowReloadWithoutAiming", &pConfig->bAllowReloadWithoutAiming_controller);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Removes the need to be aiming the weapon before you can reload it.");
						ImGui::TextWrapped("Warning: this will also change the reload button based on your current controller config type, since the original button is used for sprinting when not aiming.");
						
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("New reload button (Xinput):");
						ImGui::BulletText("Config Type I: B");
						ImGui::BulletText("Config Type II & III: X");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("ReloadWithoutZoom", &pConfig->bReloadWithoutZoom_controller);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Don't zoom in when reloading without aiming.");
						ImGui::TextWrapped("Not usually recomended, since the zooming in and out masks some animation quirks when the reload ends.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Framerate)
			{
				if (ImGui::BeginTable("Framerate", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// FixFallingItemsSpeed
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixFallingItemsSpeed", &pConfig->bFixFallingItemsSpeed);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Fixes the speed of falling items in 60 FPS, making them not fall at double speed.");
					}

					// FixTurningSpeed
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixTurningSpeed", &pConfig->bFixTurningSpeed);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Fixes speed of backwards turning when using framerates other than 30FPS.");
					}

					// FixQTE
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixQTE", &pConfig->bFixQTE);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("When running in 60 FPS, some QTEs require extremely fast button presses to work. This gets even worse in Professional difficulty, making it seem almost impossible to survive the minecart and the statue bridge QTEs.");
						ImGui::TextWrapped("This fix makes QTEs that involve rapid button presses much more forgiving.");
					}

					// FixAshleyBustPhysics
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixAshleyBustPhysics", &pConfig->bFixAshleyBustPhysics);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Fixes difference between 30/60FPS on physics applied to Ashley.");
					}

					// EnableFastMath
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("EnableFastMath", &pConfig->bEnableFastMath))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Replaces older math functions in the game with much more optimized equivalents.");
						ImGui::TextWrapped("Experimental, can hopefully improve framerate in some areas that had dips.");
					}

					// PrecacheModels
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("PrecacheModels", &pConfig->bPrecacheModels);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Forces game to fully cache all models in the level after loading in.");
						ImGui::TextWrapped("May help with framerate drops when viewing a model for the first time.");
						ImGui::TextWrapped("(not fully tested, could cause issues if level has many models to load!)");
						ImGui::TextWrapped("Changes take effect on next level load.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Misc)
			{
				if (ImGui::BeginTable("Misc", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// OverrideCostumes
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("OverrideCostumes", &pConfig->bOverrideCostumes);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows overriding the costumes, making it possible to combine Normal/Special 1/Special 2 costumes.");
						ImGui::TextWrapped("May cause weird visuals in cutscenes.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::BeginDisabled(!pConfig->bOverrideCostumes);
						ImGui::PushItemWidth(150 * pConfig->fFontSizeScale * esHook._cur_monitor_dpi);
						ImGui::Combo("Leon", &iCostumeComboLeon, sLeonCostumeNames, IM_ARRAYSIZE(sLeonCostumeNames));
						if (ImGui::IsItemEdited())
						{
							pConfig->HasUnsavedChanges = true;
							pConfig->CostumeOverride.Leon = (LeonCostume)iCostumeComboLeon;
						}

						ImGui::Combo("Ashley", &iCostumeComboAshley, sAshleyCostumeNames, IM_ARRAYSIZE(sAshleyCostumeNames));
						if (ImGui::IsItemEdited())
						{
							pConfig->HasUnsavedChanges = true;
							pConfig->CostumeOverride.Ashley = (AshleyCostume)iCostumeComboAshley;
						}

						ImGui::Combo("Ada", &iCostumeComboAda, sAdaCostumeNames, IM_ARRAYSIZE(sAdaCostumeNames));
						if (ImGui::IsItemEdited())
						{
							pConfig->HasUnsavedChanges = true;

							// ID number 2 seems to be the exact same outfit as ID number 0, for some reason, so we increase the ID here to use the actual next costume
							if (iCostumeComboAda == 2)
								pConfig->CostumeOverride.Ada = (AdaCostume)(iCostumeComboAda + 1);
							else
								pConfig->CostumeOverride.Ada = (AdaCostume)iCostumeComboAda;
						}
						ImGui::PopItemWidth();
						ImGui::EndDisabled();
					}

					// AshleyJPCameraAngles
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("AshleyJPCameraAngles", &pConfig->bAshleyJPCameraAngles);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Unlocks the JP-only classic camera angles during Ashley segment.");
					}

					// ViolenceLevelOverride
					{
						ImGui_ColumnSwitch();

						ImGui::Text("ViolenceLevelOverride");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows overriding the level of violence in the game.");
						ImGui::TextWrapped("Use -1 to leave as games default, 0 for low-violence mode (as used in JP/GER version), or 2 for full violence.");
						
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::PushItemWidth(100 * pConfig->fFontSizeScale * esHook._cur_monitor_dpi);
						if (ImGui::InputInt("Violence Level Override", &pConfig->iViolenceLevelOverride))
						{
							if (pConfig->iViolenceLevelOverride < -1)
								pConfig->iViolenceLevelOverride = -1;

							if (pConfig->iViolenceLevelOverride > 2)
								pConfig->iViolenceLevelOverride = 2;

							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true; // unfortunately required as game only reads pSys from savegame during first loading screen...
						}
						ImGui::PopItemWidth();
					}

					// AllowSellingHandgunSilencer
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("AllowSellingHandgunSilencer", &pConfig->bAllowSellingHandgunSilencer))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows selling the (normally unused) handgun silencer to the merchant.");
					}

					// AllowMafiaLeonCutscenes
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("AllowMafiaLeonCutscenes", &pConfig->bAllowMafiaLeonCutscenes))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows the game to display Leon's mafia outfit (\"Special 2\") on cutscenes.");
					}

					// SilenceArmoredAshley
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("SilenceArmoredAshley", &pConfig->bSilenceArmoredAshley);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Silence Ashley's armored outfit (\"Special 2\").");
						ImGui::TextWrapped("For those who also hate the constant \"Clank Clank Clank\".");
					}

					// AllowAshleySuplex
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("AllowAshleySuplex", &pConfig->bAllowAshleySuplex);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows Ashley to Suplex enemies in very specific situations.");
						ImGui::TextWrapped("(previously was only possible in the initial NTSC GameCube ver., was patched out in all later ports.)");
					}

					// AllowMatildaQuickturn
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("AllowMatildaQuickturn", &pConfig->bAllowMatildaQuickturn);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allows quickturning character to camera direction when wielding Matilda.");
						ImGui::TextWrapped("(only effective if MouseTurning is disabled)");
					}

					// FixDitmanGlitch
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("FixDitmanGlitch", &pConfig->bFixDitmanGlitch);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Fixes the Ditman glitch, which would allow players to increase their walk speed.");
					}

					// UseSprintToggle
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("UseSprintToggle", &pConfig->bUseSprintToggle);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Changes sprint key to act like a toggle instead of needing to be held.");
					}

					// DisableQTE
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("DisableQTE", &pConfig->bDisableQTE);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Disables most of the QTEs, making them pass automatically.");
					}

					// AutomaticMashingQTE
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("AutomaticMashingQTE", &pConfig->bAutomaticMashingQTE);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Unlike the previous option, this only automates the \"mashing\" QTEs, making them pass automatically. Prompts are still shown!");
					}

					// SkipIntroLogos
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("SkipIntroLogos", &pConfig->bSkipIntroLogos);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Whether to skip the Capcom etc intro logos when starting the game.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(!pConfig->bSkipIntroLogos);
						pConfig->HasUnsavedChanges |= ImGui::Checkbox("SkipMenuFades", &pConfig->bSkipMenuFades);
						ImGui::EndDisabled();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Reduces the length of menu fades/messages (eg. 'Load successful!') and skips the initial \"Resident Evil 4\" logo / \"PRESS ANY KEY\" screen.");
						ImGui::TextWrapped("(will only take effect when SkipIntroLogos is also set to true)");
					}

					// EnableDebugMenu
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("EnableDebugMenu", &pConfig->bEnableDebugMenu))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Enables the \"tool menu\" debug menu, present inside the game but unused, and adds a few custom menu entries (\"SAVE GAME\", \"DOF / BLUR MENU\", etc).");
						ImGui::TextWrapped("Can be opened with the LT+LS button combination (or CTRL+F3 on keyboard by default).");
						ImGui::TextWrapped("If enabled on the 1.0.6 debug build it'll apply some fixes to the existing debug menu, fixing AREA JUMP etc, but won't add our custom entries due to lack of space.");
						ImGui::TextWrapped("(may have a rare chance to cause a heap corruption crash when loading a save, but if the game loads fine then there shouldn't be any chance of crashing)");
					}

					// EnableModExpansion
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("EnableModExpansion", &pConfig->bEnableModExpansion))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Enables patches/hooks for expanded modding capabilities, such as allowing enemy speed & scale to be defined when spawning.");
						ImGui::TextWrapped("Only needed when using mods that specifically require it, otherwise should be left disabled.");
					}

					// DisableAdaKnife
					{
						ImGui_ColumnSwitch();

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("DisableAdaKnifeGC", &pConfig->bDisableAdaKnifeGC);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Disable's Ada's knife in Assignment Ada and The Mercenaries.");
						ImGui::TextWrapped("(Ada didn't have a knife in these modes until the Wii port of the game)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						pConfig->HasUnsavedChanges |= ImGui::Checkbox("DisableAdaKnifePS2", &pConfig->bDisableAdaKnifePS2);
						ImGui::TextWrapped("Disable's Ada's knife in Separate Ways.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Memory)
			{
				if (ImGui::BeginTable("Memory", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// AllowHighResolutionSFD
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("AllowHighResolutionSFD", &pConfig->bAllowHighResolutionSFD))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allocate more memory for SFD movie files, and properly scale its resolution display above 512x336.");
						ImGui::TextWrapped("Not tested beyond 1920x1080.");
					}

					// RaiseVertexAlloc
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("RaiseVertexAlloc", &pConfig->bRaiseVertexAlloc))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allocate more memory for some vertex buffers.");
						ImGui::TextWrapped("This prevents a crash that can happen when playing with a high FOV.");
					}

					// RaiseInventoryAlloc
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("RaiseInventoryAlloc", &pConfig->bRaiseInventoryAlloc))
						{
							pConfig->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("Allocate more memory for the inventory screen, preventing crashes with high-poly models inside ss_pzzl.dat.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Hotkeys)
			{
				float btn_size_x = 150 * esHook._cur_monitor_dpi;

				if (ImGui::BeginTable("HotkeysTop", 1, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui::TableNextColumn();

					static float bgHeight = 0;
					ImGui_ItemBG(bgHeight, itmbgColor);

					ImGui::TextWrapped("Key combinations for various re4_tweaks features.");
					ImGui::TextWrapped("Most keys can be combined (requiring multiple to be pressed at the same time). To combine, hold one key and press another at the same time.");
					ImGui::TextWrapped("(Press \"Save\" for changes to take effect.)");

					bgHeight = ImGui::GetCursorPos().y;

					ImGui::EndTable();
				}

				ImGui::Dummy(ImVec2(0.0f, 10.0f));

				if (ImGui::BeginTable("Hotkeys", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// cfgMenu 
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("Key combination to open the re4_tweaks config menu");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(1);
						if (ImGui::Button(pConfig->sConfigMenuKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							pConfig->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyComboThread, &pConfig->sConfigMenuKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("Open config menu");
					}

					// Console
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("Key combination to open the re4_tweaks debug console (only in certain re4_tweaks builds)");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(2);
						if (ImGui::Button(pConfig->sConsoleKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							pConfig->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyComboThread, &pConfig->sConsoleKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("Open console");
					}

					// Inv flip 
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("Key bindings for flipping items in the inventory screen when using keyboard and mouse.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Normally, you can only rotate them with the keyboard, not flip them. Flipping was possible in the old PC port and is possible using a controller.");
						ImGui::Spacing();
						ImGui::BulletText("Key combinations not supported");
						ImGui::Spacing();
						
						if (ImGui::BeginTable("inv flip", 2))
						{
							ImGui::TableNextColumn();
							ImGui::Spacing();

							// UP
							ImGui::TextWrapped("Flip UP");
							ImGui::PushID(3);
							if (ImGui::Button(pConfig->sFlipItemUp.c_str(), ImVec2(btn_size_x, 0)))
							{
								pConfig->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyThread, &pConfig->sFlipItemUp, 0, NULL);
							}
							ImGui::PopID();

							ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

							// DOWN
							ImGui::TextWrapped("Flip DOWN");
							ImGui::PushID(4);
							if (ImGui::Button(pConfig->sFlipItemDown.c_str(), ImVec2(btn_size_x, 0)))
							{
								pConfig->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyThread, &pConfig->sFlipItemDown, 0, NULL);
							}
							ImGui::PopID();

							ImGui::TableNextColumn();

							// LEFT
							ImGui::TextWrapped("Flip LEFT");
							ImGui::PushID(5);
							if (ImGui::Button(pConfig->sFlipItemLeft.c_str(), ImVec2(btn_size_x, 0)))
							{
								pConfig->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyThread, &pConfig->sFlipItemLeft, 0, NULL);
							}
							ImGui::PopID();

							ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

							// RIGHT
							ImGui::TextWrapped("Flip RIGHT");
							ImGui::PushID(6);
							if (ImGui::Button(pConfig->sFlipItemRight.c_str(), ImVec2(btn_size_x, 0)))
							{
								pConfig->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyThread, &pConfig->sFlipItemRight, 0, NULL);
							}
							ImGui::PopID();

							ImGui::EndTable();
						}
					}

					// QTE 
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("Key bindings for QTE keys when playing with keyboard and mouse.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Unlike the \"official\" way of rebinding keys through usrpInput.ini, this option also changes the on-screen prompt to properly match the selected key.");
						ImGui::BulletText("Key combinations not supported");
						ImGui::Spacing();

						if (ImGui::BeginTable("qtekey", 2))
						{
							ImGui::TableNextColumn();
							ImGui::Spacing();

							// QTE1
							ImGui::TextWrapped("QTE key 1");
							ImGui::PushID(7);
							if (ImGui::Button(pConfig->sQTE_key_1.c_str(), ImVec2(btn_size_x, 0)))
							{
								pConfig->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyThread, &pConfig->sQTE_key_1, 0, NULL);
							}
							ImGui::PopID();

							ImGui::TableNextColumn();

							// QTE2
							ImGui::TextWrapped("QTE key 2");
							ImGui::PushID(8);
							if (ImGui::Button(pConfig->sQTE_key_2.c_str(), ImVec2(btn_size_x, 0)))
							{
								pConfig->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyThread, &pConfig->sQTE_key_2, 0, NULL);
							}
							ImGui::PopID();

							ImGui::EndTable();
						}
					}

					// Debug menu
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("Key combination to make the \"tool menu\" debug menu appear.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("Requires EnableDebugMenu to be enabled.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(9);
						if (ImGui::Button(pConfig->sDebugMenuKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							pConfig->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyComboThread, &pConfig->sDebugMenuKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("Open debug menu");
					}

					// MouseTurningModifier
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("MouseTurning camera modifier.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("When holding this key combination, MouseTurning is temporarily activated/deactivated.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(10);
						if (ImGui::Button(pConfig->sMouseTurnModifierKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							pConfig->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyComboThread, &pConfig->sMouseTurnModifierKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("MouseTurning modifier");
					}

					// JetSkiTricks 
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("Key combination for jump tricks during Jet-ski sequence (normally RT+LT on controller)");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(11);
						if (ImGui::Button(pConfig->sJetSkiTrickCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							pConfig->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&SetHotkeyComboThread, &pConfig->sJetSkiTrickCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("Trick while riding Jet-ski");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Trainer)
			{
				Trainer_RenderUI(columnCount);
			}

			ImGui::PopStyleVar();

			ImGui::EndChild();
		}

		//
		ImGui::End();
	}
}

void ShowCfgMenuTip()
{
	if (!bCfgMenuOpen)
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(20, viewport->Pos.y + 20));
		ImGui::SetNextWindowBgAlpha(0.5);

		ImGui::Begin("cfgMenuTip", nullptr, ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

		std::string tooltip = std::string("re4_tweaks: Press ") + pConfig->sConfigMenuKeyCombo.c_str() + std::string(" to open the configuration menu");

		ImGui::TextUnformatted(tooltip.data());

		ImGui::End();
	}
}
