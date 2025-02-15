#include <iostream>
#include "dllmain.h"
#include "Patches.h"
#include <d3d9.h>
#include "Settings.h"
#include "imgui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx9.h"
#include "input.hpp"
#include "Utils.h"

ConsoleOutput con;

bool bConsoleOpen;
bool AutoScroll = true;  // Keep scrolling if already at the bottom.

ImGuiTextBuffer Buf;
ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.

std::string TitleCombo;

std::vector<uint32_t> ConsoleCombo;

bool ParseConsoleKeyCombo(std::string_view in_combo)
{
    if (in_combo.empty())
        return false;

    ConsoleCombo.clear();
    ConsoleCombo = ParseKeyCombo(in_combo);

    pInput->RegisterHotkey({ []() {
        bConsoleOpen = !bConsoleOpen;
    }, &ConsoleCombo });

    return ConsoleCombo.size() > 0;
}

void ConsoleOutput::Clear()
{
    Buf.clear();
    LineOffsets.clear();
    LineOffsets.push_back(0);
}

void ConsoleOutput::AddLogChar(const char* fmt, ...)
{
    int old_size = Buf.size();
    va_list args;
    va_start(args, fmt);
    Buf.appendfv(fmt, args);
    va_end(args);
    Buf.append("\n");
    for (int new_size = Buf.size(); old_size < new_size; old_size++)
        if (Buf[old_size] == '\n')
            LineOffsets.push_back(old_size + 1);
}

void ConsoleOutput::AddLogHex(int fmt, ...)
{
    int old_size = Buf.size();
    va_list args;

    char fmt_hex[50];
    sprintf_s(fmt_hex, "%02x", fmt);
    va_start(args, fmt_hex);
    Buf.appendfv(fmt_hex, args);

    va_end(args);
    Buf.append("\n");
    for (int new_size = Buf.size(); old_size < new_size; old_size++)
        if (Buf[old_size] == '\n')
            LineOffsets.push_back(old_size + 1);
}

void ConsoleOutput::AddLogInt(int fmt, ...)
{
    int old_size = Buf.size();
    va_list args;

    char fmt_int[50];
    sprintf_s(fmt_int, "%d", fmt);
    va_start(args, fmt_int);
    Buf.appendfv(fmt_int, args);

    va_end(args);
    Buf.append("\n");
    for (int new_size = Buf.size(); old_size < new_size; old_size++)
        if (Buf[old_size] == '\n')
            LineOffsets.push_back(old_size + 1);
}

void ConsoleOutput::AddLogFloat(float fmt, ...)
{
    int old_size = Buf.size();
    va_list args;

    char fmt_int[50];
    sprintf_s(fmt_int, "%f", fmt);
    va_start(args, fmt_int);
    Buf.appendfv(fmt_int, args);

    va_end(args);
    Buf.append("\n");
    for (int new_size = Buf.size(); old_size < new_size; old_size++)
        if (Buf[old_size] == '\n')
            LineOffsets.push_back(old_size + 1);
}

void ConsoleOutput::AddConcatLog(const char* fmt, int value, ...)
{
    char logBuffer[200];
    strcpy_s(logBuffer, fmt);
    strcat_s(logBuffer, std::to_string(value).data());
    con.AddLogChar(logBuffer);
    memset(logBuffer, 0, sizeof(logBuffer));
}

void ConsoleOutput::AddConcatLog(const char* fmt, float value, ...)
{
    char logBuffer[200];
    strcpy_s(logBuffer, fmt);
    strcat_s(logBuffer, std::to_string(value).data());
    con.AddLogChar(logBuffer);
    memset(logBuffer, 0, sizeof(logBuffer));
}

void ConsoleOutput::AddConcatLog(const char* fmt, double value, ...)
{
    char logBuffer[200];
    strcpy_s(logBuffer, fmt);
    strcat_s(logBuffer, std::to_string(value).data());
    con.AddLogChar(logBuffer);
    memset(logBuffer, 0, sizeof(logBuffer));
}

void ConsoleOutput::AddConcatLog(const char* fmt, const char* value, ...)
{
    char logBuffer[200];
    strcpy_s(logBuffer, fmt);
    strcat_s(logBuffer, value);
    con.AddLogChar(logBuffer);
    memset(logBuffer, 0, sizeof(logBuffer));
}

void Draw(const char* title, bool* p_open = NULL)
{
    if (!ImGui::Begin(title, p_open))
    {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Debug"))
    {
        ImGui::SliderFloat("fdbg1", &pConfig->fdbg1, -200.0f, 200.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("fdbg2", &pConfig->fdbg2, -200.0f, 200.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("fdbg3", &pConfig->fdbg3, -200.0f, 200.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("fdbg4", &pConfig->fdbg4, -200.0f, 200.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("fdbg5", &pConfig->fdbg5, -200.0f, 200.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("fdbg6", &pConfig->fdbg6, -200.0f, 200.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();

        ImGui::Checkbox("bdbg1", &pConfig->bdbg1);
        ImGui::Checkbox("bdbg2", &pConfig->bdbg2);
        ImGui::Checkbox("bdbg3", &pConfig->bdbg3);
    }

    // Options menu
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    // Main window
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    bool clear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");

    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (clear)
        con.Clear();
    if (copy)
        ImGui::LogToClipboard();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    const char* buf = Buf.begin();
    const char* buf_end = Buf.end();
    
    ImGuiListClipper clipper;
    clipper.Begin(LineOffsets.Size);
    while (clipper.Step())
    {
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
        {
            const char* line_start = buf + LineOffsets[line_no];
            const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
            ImGui::TextUnformatted(line_start, line_end);
        }
    }
    clipper.End();
    
    ImGui::PopStyleVar();

    if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();

    ImGui::End();
}

// Simple log window.
void ConsoleOutput::ShowConsoleOutput()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - (550 * esHook._cur_monitor_dpi), viewport->Pos.y + (20 * esHook._cur_monitor_dpi)));
    ImGui::SetNextWindowSize(ImVec2(500 * esHook._cur_monitor_dpi, 400 * esHook._cur_monitor_dpi), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(!bConsoleOpen);

    std::string name = std::string("Console Output - ") + con.TitleKeyCombo + std::string(" to Show/Hide") + " - " + pConfig->sTrainerFocusUIKeyCombo + " to Focus/Unfocus";
    ImGui::Begin(name.data());
    ImGui::End();
    Draw(name.data());
}