
// NSMBPrefsLauncher.cpp
// Compile with:
//   g++ NSMBPrefsLauncher.cpp -o NSMBPrefsLauncher.exe -municode -static -mwindows
// Or in Visual Studio: set Linker -> System -> SubSystem = Windows, then build.

#include <windows.h>
#include <sddl.h>      // for ConvertSidToStringSidW
#include <string>
#include <vector>
#include <random>
#include <memory>
#include <iomanip>
#include <sstream>
#include <fstream>
using namespace std;
// Convert binary to hex string (comma-separated bytes)
static std::string BinaryToRegHex(const std::vector<BYTE>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < data.size(); ++i) {
        oss << std::setw(2) << (int)data[i];
        if (i + 1 < data.size()) oss << ",";
    }
    return oss.str();
}

// Read REG_BINARY
static bool GetBinaryRegValue(HKEY root, const std::wstring& subKey,
    const std::wstring& valueName,
    std::vector<BYTE>& outData) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) return false;
    DWORD type = 0, size = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_BINARY) {
        RegCloseKey(hKey);
        return false;
    }
    outData.resize(size);
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, outData.data(), &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    RegCloseKey(hKey);
    return true;
}

// Write a REG_BINARY
static void SetBinaryRegValue(HKEY root, const std::wstring& subKey,
    const std::wstring& valueName,
    const std::vector<BYTE>& data) {
    HKEY hKey;
    RegCreateKeyExW(root, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY,
        data.data(), (DWORD)data.size());
    RegCloseKey(hKey);
}

// Write a REG_DWORD
static void SetDwordRegValue(HKEY root, const std::wstring& subKey,
    const std::wstring& valueName,
    DWORD value) {
    HKEY hKey;
    RegCreateKeyExW(root, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(hKey);
}

// Delete a registry value
static void DeleteRegValue(HKEY root, const std::wstring& subKey,
    const std::wstring& valueName) {
    HKEY hKey;
    if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueW(hKey, valueName.c_str());
        RegCloseKey(hKey);
    }
}

// Retrieve current user SID
static std::wstring GetCurrentUserSid() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return L"";
    DWORD len = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &len);
    std::unique_ptr<BYTE[]> buffer(new BYTE[len]);
    PTOKEN_USER pUser = reinterpret_cast<PTOKEN_USER>(buffer.get());
    if (!GetTokenInformation(hToken, TokenUser, pUser, len, &len)) { CloseHandle(hToken); return L""; }
    LPWSTR sidString = NULL;
    ConvertSidToStringSidW(pUser->User.Sid, &sidString);
    std::wstring sid(sidString);
    LocalFree(sidString);
    CloseHandle(hToken);
    return sid;
}

// Get timestamp in YYYYMMDD_HH_MM_SS_MS format
static std::string GetTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << st.wYear
        << std::setw(2) << st.wMonth
        << std::setw(2) << st.wDay << '_'
        << std::setw(2) << st.wHour << '_'
        << std::setw(2) << st.wMinute << '_'
        << std::setw(2) << st.wSecond << '_'
        << std::setw(3) << st.wMilliseconds;
    return oss.str();
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    const std::wstring baseSubKey = L"Software\\ipodtouch0218\\NSMB-MarioVsLuigi";
    std::wstring sid = GetCurrentUserSid(); if (sid.empty()) return 1;
    const std::wstring hkuSubKey = sid + L"\\" + baseSubKey;

    // Create folder "previousids"
    wchar_t exePath[MAX_PATH]; GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring folder(exePath);
    folder.resize(folder.find_last_of(L"\\/"));
    folder += L"\\previousids";
    CreateDirectoryW(folder.c_str(), NULL);

    const std::vector<std::wstring> binKeys = { L"token_h183304158", L"id_h5861160", L"unity.cloud_userid_h2665564582" };
    std::string ts = GetTimestamp();
    std::wstring beforePath = folder + L"\\" + std::wstring(ts.begin(), ts.end()) + L"_before.reg";
    std::ofstream beforeFile(beforePath);
    beforeFile << "Windows Registry Editor Version 5.00\r\n\r\n";
    beforeFile << "[HKEY_CURRENT_USER\\" << std::string(baseSubKey.begin(), baseSubKey.end()) << "]\r\n";
    for (auto& key : binKeys) {
        std::vector<BYTE> data;
        if (GetBinaryRegValue(HKEY_CURRENT_USER, baseSubKey, key, data)) {
            beforeFile << "\"" << std::string(key.begin(), key.end())
                << "\"=hex:" << BinaryToRegHex(data) << "\r\n";
        }
    }
    beforeFile << "\r\n[HKEY_USERS\\" << std::string(hkuSubKey.begin(), hkuSubKey.end()) << "]\r\n";
    for (auto& key : binKeys) {
        std::vector<BYTE> data;
        if (GetBinaryRegValue(HKEY_USERS, hkuSubKey, key, data)) {
            beforeFile << "\"" << std::string(key.begin(), key.end())
                << "\"=hex:" << BinaryToRegHex(data) << "\r\n";
        }
    }
    beforeFile.close();

    // Delete selected session keys before launch
    const std::vector<std::wstring> delKeys = { L"unity.player_session_count_h922449978", L"unity.player_sessionid_h1351336811" };
    for (auto& k : delKeys) {
        DeleteRegValue(HKEY_CURRENT_USER, baseSubKey, k);
        DeleteRegValue(HKEY_USERS, hkuSubKey, k);
    }

    // Modify regs (randomized as before)
    std::random_device rd; std::mt19937 gen(rd());
    auto randHex = [&](int len) {std::string s; s.reserve(len); for (int i = 0; i < len; ++i) s += "0123456789abcdef"[gen() % 16]; return s; };
    auto ToBin = [&](const std::string& str) {std::vector<BYTE>d(str.begin(), str.end()); d.push_back(0); return d; };
    std::string token = randHex(22), cloud = randHex(32), guid = randHex(8) + "-" + randHex(4) + "-" + randHex(4) + "-" + randHex(4) + "-" + randHex(12);
    std::uniform_int_distribution<> ds(0, 57); DWORD skin;
    do { skin = ds(gen); } while (skin == 24 || skin == 52);
    SetBinaryRegValue(HKEY_CURRENT_USER, baseSubKey, L"token_h183304158", ToBin(token));
    SetBinaryRegValue(HKEY_CURRENT_USER, baseSubKey, L"id_h5861160", ToBin(guid));
    SetBinaryRegValue(HKEY_CURRENT_USER, baseSubKey, L"unity.cloud_userid_h2665564582", ToBin(cloud));
    SetDwordRegValue(HKEY_CURRENT_USER, baseSubKey, L"Skin_h2089423610", skin);
    SetBinaryRegValue(HKEY_USERS, hkuSubKey, L"token_h183304158", ToBin(token));
    SetBinaryRegValue(HKEY_USERS, hkuSubKey, L"id_h5861160", ToBin(guid));
    SetBinaryRegValue(HKEY_USERS, hkuSubKey, L"unity.cloud_userid_h2665564582", ToBin(cloud));
    SetDwordRegValue(HKEY_USERS, hkuSubKey, L"Skin_h2089423610", skin);

    // Launch game
    std::wstring gameExe = folder.substr(0, folder.find(L"\\previousids")) + L"\\NSMB-MarioVsLuigi.exe";
    STARTUPINFOW si{ sizeof(si) }; PROCESS_INFORMATION pi;
    CreateProcessW(gameExe.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (pi.hProcess) CloseHandle(pi.hProcess); if (pi.hThread) CloseHandle(pi.hThread);

    // Wait and backup AFTER
    Sleep(15000);
    std::string ts2 = GetTimestamp();
    std::wstring afterPath = folder + L"\\" + std::wstring(ts2.begin(), ts2.end()) + L"_after.reg";
    std::ofstream afterFile(afterPath);
    afterFile << "Windows Registry Editor Version 5.00\r\n\r\n";
    afterFile << "[HKEY_CURRENT_USER\\" << std::string(baseSubKey.begin(), baseSubKey.end()) << "]\r\n";
    for (auto& key : binKeys) {
        std::vector<BYTE> data;
        if (GetBinaryRegValue(HKEY_CURRENT_USER, baseSubKey, key, data)) {
            afterFile << "\"" << std::string(key.begin(), key.end())
                << "\"=hex:" << BinaryToRegHex(data) << "\r\n";
        }
    }
    afterFile << "\r\n[HKEY_USERS\\" << std::string(hkuSubKey.begin(), hkuSubKey.end()) << "]\r\n";
    for (auto& key : binKeys) {
        std::vector<BYTE> data;
        if (GetBinaryRegValue(HKEY_USERS, hkuSubKey, key, data)) {
            afterFile << "\"" << std::string(key.begin(), key.end())
                << "\"=hex:" << BinaryToRegHex(data) << "\r\n";
        }
    }
    afterFile.close();

    return 0;
}

// stub main
int main() { return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_HIDE); }
