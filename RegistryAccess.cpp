#include <windows.h>
#include <atlstr.h>
#include <iostream>

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

int main()
{
    HKEY hRootKey = HKEY_CURRENT_USER;
    LPCTSTR subKey = _T("Software\\MyApp\\Colors");
    CString value;

    if (RegistryKeyExists(hRootKey, subKey))
    {
        subKey = _T("Software\\MyApp\\Colors");
        if (ReadStringValue(hRootKey, subKey, _T("BackgroundBookmarked"), value))
        {
            std::wcout << value.GetString() << std::endl;
        }
    }
    /*
    if (ReadStringValue(hRootKey, subKey, _T("Hello"), value))
    {
        std::wcout << value.GetString() << std::endl;
    }

    DWORD dwValue;
    if (ReadDWORDValue(hRootKey, subKey, _T("Number"), dwValue))
    {
        std::wcout << dwValue << std::endl;
    }

    if (WriteStringValue(hRootKey, subKey, _T("Test"), _T("Hello, World!")))
    {
        std::wcout << _T("Value written successfully") << std::endl;
    }

    if (WriteDWORDValue(hRootKey, subKey, _T("TestDWORD"), 1234))
    {
        std::wcout << _T("DWORD value written successfully") << std::endl;
    }*/
    return 0;
}