#include <windows.h>
#include <atlstr.h>
#include <iostream>
#include <vector>
#include <strsafe.h>
#include <map>

HKEY OpenRegistryKey(HKEY hRootKey, LPCTSTR subKey, REGSAM access = KEY_READ)
{
    HKEY hKey;
    if (RegOpenKeyEx(hRootKey, subKey, 0, access, &hKey) == ERROR_SUCCESS)
    {
        return hKey;
    }
    return nullptr;  // Key does not exist or failed to open
}

bool RegistryKeyExists(HKEY hRootKey, LPCTSTR subKey)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(hRootKey, subKey, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

bool ReadStringValue(HKEY hRootKey, LPCTSTR subKey, LPCTSTR valueName, CString& outValue)
{
    HKEY hKey = OpenRegistryKey(hRootKey, subKey, KEY_READ);
    if (!hKey) return false;

    DWORD dataSize = 0;
    LONG result = RegQueryValueEx(hKey, valueName, nullptr, nullptr, nullptr, &dataSize);
    if (result != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return false;
    }

    CString value;
    value.Preallocate(dataSize / sizeof(TCHAR));

    result = RegQueryValueEx(hKey, valueName, nullptr, nullptr, (LPBYTE)value.GetString(), &dataSize);
    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS)
    {
        outValue = value;
        return true;
    }
    return false;
}

bool ReadDWORDValue(HKEY hRootKey, LPCTSTR subKey, LPCTSTR valueName, DWORD& outValue)
{
    HKEY hKey = OpenRegistryKey(hRootKey, subKey, KEY_READ);
    if (!hKey) return false;

    DWORD dataSize = sizeof(DWORD);
    LONG result = RegQueryValueEx(hKey, valueName, nullptr, nullptr, (LPBYTE)&outValue, &dataSize);
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

bool WriteStringValue(HKEY hRootKey, LPCTSTR subKey, LPCTSTR valueName, CString const& value)
{
    HKEY hKey;
    LONG result = RegCreateKeyEx(hRootKey, subKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;

    result = RegSetValueEx(hKey, valueName, 0, REG_SZ, (const BYTE*)value.GetString(), (value.GetLength() + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

bool WriteDWORDValue(HKEY hRootKey, LPCTSTR subKey, LPCTSTR valueName, DWORD value)
{
    HKEY hKey;
    LONG result = RegCreateKeyEx(hRootKey, subKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;

    result = RegSetValueEx(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

struct RegistryValueInfo
{
    CString name;
    DWORD type;
};

bool GetRegistryValues(HKEY hRootKey, LPCTSTR subKey, std::vector<RegistryValueInfo>& values)
{
    HKEY hKey = OpenRegistryKey(hRootKey, subKey, KEY_READ);
    if (!hKey) return false;

    DWORD index = 0;
    TCHAR valueName[256];
    DWORD valueNameSize;
    DWORD type;
    LONG result;

    values.clear();

    while (true)
    {
        valueNameSize = _countof(valueName);
        result = RegEnumValue(hKey, index, valueName, &valueNameSize, nullptr, &type, nullptr, nullptr);
        if (result == ERROR_NO_MORE_ITEMS) break;
        if (result != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return false;
        }

        values.push_back({valueName, type});
        index++;
    }

    RegCloseKey(hKey);
    return true;
}

bool GetRegistrySubkeys(HKEY hRootKey, LPCWSTR subKey, std::vector<CString>& subkeys)
{
    HKEY hKey = OpenRegistryKey(hRootKey, subKey, KEY_READ);
    if (!hKey) return false;

    DWORD index = 0;
    TCHAR subKeyName[256];
    DWORD subKeyNameSize;
    LONG result;

    subkeys.clear();

    while (true)
    {
        subKeyNameSize = _countof(subKeyName);
        result = RegEnumKeyEx(hKey, index, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr);
        if (result == ERROR_NO_MORE_ITEMS) break;
        if (result != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return false;
        }

        subkeys.push_back(subKeyName);
        index++;
    }

    RegCloseKey(hKey);
    return true;
}

int main()
{
    
    return 0;
}