#pragma once
#include <string>
#include "imgui.h"
#include <random>
#include <DirectXMath.h>
#include "SDK/basic_types.h"
#include "IniReader.h"

template <typename I> std::string IntToHexStr(I w, size_t hex_len = sizeof(I) << 1) {
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[((int)w >> j) & 0x0f];
	return std::string("0x") + rc;
}

std::string WstrToStr(const std::wstring& wstr);
std::wstring StrToWstr(const std::string& str);
std::string StrToUpper(std::string str);
float GetRandomFloat(float min, float max);
float get_distance(float x1, float y1, float z1, float x2, float y2, float z2);
bool WorldToScreen(Vec pos, Vec& screen, Mtx44 matrix, float fov, float windowWidth, float windowHeight);
ImU32 getHealthColor(float health);

void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);

// Extended version of CIniReader which also checks command-line params for values to override with
class CmdIniReader : CIniReader
{
private:
	LPWSTR* argv = nullptr;
	int argc = 0;

	std::string SearchCommandLineParamValue(std::string_view param_name, bool is_boolean = false);

public:
	CmdIniReader() : CIniReader() {}
	CmdIniReader(std::string_view szFileName) : CIniReader(szFileName) {}
	CmdIniReader(std::stringstream& ini_mem) : CIniReader(ini_mem) {}
	~CmdIniReader();

	bool ReadBoolean(std::string_view szSection, std::string_view szKey, bool bolDefaultValue);

	float ReadFloat(std::string_view szSection, std::string_view szKey, float fltDefaultValue);

	int ReadInteger(std::string_view szSection, std::string_view szKey, int iDefaultValue);

	std::string ReadString(std::string_view szSection, std::string_view szKey, std::string_view szDefaultValue);
};
