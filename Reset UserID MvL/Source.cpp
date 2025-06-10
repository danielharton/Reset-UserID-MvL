// NSMBPrefsLauncher.cpp
// Compile with:
//   g++ Source.cpp -o NSMBPrefsLauncher.exe -municode -static -mwindows
// Or in Visual Studio: set Linker -> System -> SubSystem = Windows, then build.

#include <windows.h>
#include <sddl.h>      // for ConvertSidToStringSidW
#include <string>
#include <vector>
#include <random>
#include <memory>

// Generate a random lowercase hex string of given length.
static std::string GenerateHex(int length) {
    static const char hexChars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result.push_back(hexChars[dis(gen)]);
    }
    return result;
}

// Generate a GUID-like string: 8-4-4-4-12 hex characters
static std::string GenerateGuid() {
    return GenerateHex(8) + "-" + GenerateHex(4) + "-" + GenerateHex(4) +
        "-" + GenerateHex(4) + "-" + GenerateHex(12);
}

// Convert an ASCII string into a binary blob (ASCII bytes + null terminator)
static std::vector<BYTE> ToBinary(const std::string& str) {
    std::vector<BYTE> data(str.begin(), str.end());
    data.push_back(0);
    return data;
}

// Retrieve the current user's SID as a string.
static std::wstring GetCurrentUserSid() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return L"";
    }
    DWORD len = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &len);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return L"";
    }
    std::unique_ptr<BYTE[]> buffer(new BYTE[len]);
    PTOKEN_USER pUser = reinterpret_cast<PTOKEN_USER>(buffer.get());
    if (!GetTokenInformation(hToken, TokenUser, pUser, len, &len)) {
        CloseHandle(hToken);
        return L"";
    }
    LPWSTR sidString = NULL;
    if (!ConvertSidToStringSidW(pUser->User.Sid, &sidString)) {
        CloseHandle(hToken);
        return L"";
    }
    std::wstring sid(sidString);
    LocalFree(sidString);
    CloseHandle(hToken);
    return sid;
}

// Write a REG_BINARY value under specified root/subkey.
static bool SetBinaryRegValue(HKEY root, const std::wstring& subKey,
    const std::wstring& valueName,
    const std::vector<BYTE>& data) {
    HKEY hKey = NULL;
    LONG rc = RegCreateKeyExW(root, subKey.c_str(), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
        &hKey, NULL);
    if (rc != ERROR_SUCCESS) return false;
    rc = RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY,
        data.data(), static_cast<DWORD>(data.size()));
    RegCloseKey(hKey);
    return (rc == ERROR_SUCCESS);
}

// Standard Windows entry point for GUI apps (no console)
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    const std::wstring baseHKCU = L"Software\\ipodtouch0218\\NSMB-MarioVsLuigi";
    std::wstring sid = GetCurrentUserSid();
    if (sid.empty()) return 1;
    const std::wstring baseHKU = sid + L"\\Software\\ipodtouch0218\\NSMB-MarioVsLuigi";

    // Generate random registry values
    std::string tokenVal = GenerateHex(22);
    std::string cloudVal = GenerateHex(32);
    std::string guidVal = GenerateGuid();

    // Prepare binary blobs
    auto tokenData = ToBinary(tokenVal);
    auto cloudData = ToBinary(cloudVal);
    auto guidData = ToBinary(guidVal);

    // Write to both hives
    SetBinaryRegValue(HKEY_CURRENT_USER, baseHKCU, L"token_h183304158", tokenData);
    SetBinaryRegValue(HKEY_CURRENT_USER, baseHKCU, L"unity.cloud_userid_h2665564582", cloudData);
    SetBinaryRegValue(HKEY_CURRENT_USER, baseHKCU, L"id_h5861160", guidData);

    SetBinaryRegValue(HKEY_USERS, baseHKU, L"token_h183304158", tokenData);
    SetBinaryRegValue(HKEY_USERS, baseHKU, L"unity.cloud_userid_h2665564582", cloudData);
    SetBinaryRegValue(HKEY_USERS, baseHKU, L"id_h5861160", guidData);

    // Launch NSMB-MarioVsLuigi.exe from current directory
    wchar_t pathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, pathBuf, MAX_PATH);
    std::wstring path(pathBuf);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) path.resize(pos);
    path += L"\\NSMB-MarioVsLuigi.exe";

    STARTUPINFOW si{ sizeof(si) };
    PROCESS_INFORMATION pi;
    CreateProcessW(path.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread)  CloseHandle(pi.hThread);

    return 0;
}

// Provide a stub main() so linker in console subsystem still resolves main
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_HIDE);
}
