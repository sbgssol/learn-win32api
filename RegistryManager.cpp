#include <windows.h>
#include <string>
#include <iostream>

#define LY_MSB(_msg_, ...) {\
DWORD __lasterr = GetLastError();\
char __msg[1000];\
sprintf_s(__msg, 1000, "%s::%d Error Code = %d", __FUNCTION__, __LINE__, __lasterr);\
char __msg2[1000];\
sprintf_s(__msg2, 1000, _msg_, ##__VA_ARGS__);\
MessageBox(0, __msg2, __msg, MB_OK);\
SetLastError(__lasterr);\
}

#define LY_INF(_msg_, ...) {\
char __msg[1000];\
sprintf_s(__msg, 1000, "%s::%d", __FUNCTION__, __LINE__);\
char __msg2[1000];\
sprintf_s(__msg2, 1000, _msg_, ##__VA_ARGS__);\
std::cout << "INFO: " << __msg << ": " << __msg2 << "\n";\
}

#define LY_ERR(_msg_, ...) {\
char __msg[1000];\
sprintf_s(__msg, 1000, "%s::%d Error Code = %d", __FUNCTION__, __LINE__, GetLastError());\
char __msg2[1000];\
sprintf_s(__msg2, 1000, _msg_, ##__VA_ARGS__);\
std::cerr << "ERROR at " << __msg << "\n" << __msg2 << "\n";\
exit(1);\
}

#define LY_TEST(expr, msg, ...) if (!(expr)) LY_ERR(msg, __VA_ARGS__)

class RegistryManager
{
public:
    RegistryManager(HKEY rootKey);
    ~RegistryManager();

    bool CreateKey(const std::string& subKey, std::string& error);
    bool DeleteKey(const std::string& subKey, std::string& error);

    bool WriteStringValue(const std::string& subKey, const std::string& valueName, const std::string& data, std::string& error);
    bool WriteDWORDValue(const std::string& subKey, const std::string& valueName, DWORD data, std::string& error);

    bool ReadStringValue(const std::string& subKey, const std::string& valueName, std::string& dataOut, std::string& error);
    bool ReadDWORDValue(const std::string& subKey, const std::string& valueName, DWORD& dataOut, std::string& error);

    bool DeleteValue(const std::string& subKey, const std::string& valueName, std::string& error);

private:
    HKEY m_rootKey;

    std::string GetLastErrorAsString(DWORD errorCode = GetLastError()) const;
};


RegistryManager::RegistryManager(HKEY rootKey) : m_rootKey(rootKey)
{}
RegistryManager::~RegistryManager()
{}

bool RegistryManager::CreateKey(const std::string& subKey, std::string& error)
{
    HKEY hKey;
    DWORD disposition;
    LONG result = RegCreateKeyEx(m_rootKey, subKey.c_str(), 0, nullptr, 0,
                                  KEY_WRITE, nullptr, &hKey, &disposition);
    if (result != ERROR_SUCCESS)
    {
        error = "Failed to create/open key: " + GetLastErrorAsString(result);
        return false;
    }

    RegCloseKey(hKey);
    return true;
}

bool RegistryManager::DeleteKey(const std::string& subKey, std::string& error)
{
    LONG result = RegDeleteKey(m_rootKey, subKey.c_str());
    if (result != ERROR_SUCCESS)
    {
        error = "Failed to delete key: " + GetLastErrorAsString(result);
        return false;
    }
    return true;
}

bool RegistryManager::WriteStringValue(const std::string& subKey, const std::string& valueName, const std::string& data, std::string& error)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(m_rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        error = "Failed to open key for writing: " + GetLastErrorAsString(result);
        return false;
    }

    result = RegSetValueEx(hKey, valueName.c_str(), 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(data.c_str()),
                            static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
    {
        error = "Failed to write string value: " + GetLastErrorAsString(result);
        return false;
    }

    return true;
}

bool RegistryManager::WriteDWORDValue(const std::string& subKey, const std::string& valueName, DWORD data, std::string& error)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(m_rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        error = "Failed to open key for writing: " + GetLastErrorAsString(result);
        return false;
    }

    result = RegSetValueEx(hKey, valueName.c_str(), 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&data), sizeof(DWORD));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
    {
        error = "Failed to write DWORD value: " + GetLastErrorAsString(result);
        return false;
    }

    return true;
}

bool RegistryManager::ReadStringValue(const std::string& subKey, const std::string& valueName, std::string& dataOut, std::string& error)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(m_rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        //error = "Failed to open key for reading: " + GetLastErrorAsString(result);
        LY_MSB("Failed to open key for reading");
        return false;
    }

    DWORD type = 0;
    DWORD size = 0;
    result = RegQueryValueEx(hKey, valueName.c_str(), nullptr, &type, nullptr, &size);
    if (result != ERROR_SUCCESS || type != REG_SZ)
    {
        //error = "Failed to query string value: " + GetLastErrorAsString(result);
        LY_MSB("Failed to query string value");
        RegCloseKey(hKey);
        return false;
    }

    char buffer[255];
    result = RegQueryValueEx(hKey, valueName.c_str(), nullptr, nullptr,
                              (LPBYTE)buffer, &size);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
    {
        //error = "Failed to read string value: " + GetLastErrorAsString(result);
        LY_MSB("Failed to read string value");
        return false;
    }

    buffer[254] = '\0';  // remove null terminator
    dataOut = buffer;
    return true;
}

bool RegistryManager::ReadDWORDValue(const std::string& subKey, const std::string& valueName, DWORD& dataOut, std::string& error)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(m_rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        error = "Failed to open key for reading: " + GetLastErrorAsString(result);
        return false;
    }

    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    result = RegQueryValueEx(hKey, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&dataOut), &size);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || type != REG_DWORD)
    {
        error = "Failed to read DWORD value: " + GetLastErrorAsString(result);
        return false;
    }

    return true;
}

bool RegistryManager::DeleteValue(const std::string& subKey, const std::string& valueName, std::string& error)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(m_rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        error = "Failed to open key for deleting value: " + GetLastErrorAsString(result);
        return false;
    }

    result = RegDeleteValue(hKey, valueName.c_str());
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
    {
        error = "Failed to delete value: " + GetLastErrorAsString(result);
        return false;
    }

    return true;
}

std::string RegistryManager::GetLastErrorAsString(DWORD errorCode) const
{
    LPSTR msgBuffer = nullptr;
    DWORD size = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msgBuffer, 0, nullptr);

    std::string message(msgBuffer, size);
    LocalFree(msgBuffer);
    return message;
}

/// Main
/// 



int main()
{
    RegistryManager reg(HKEY_CURRENT_USER);
    std::string error;

    if (!reg.CreateKey("Software\\MyTestApp", error))
    {
        
        LY_MSB("Code=%d\nError: %s", GetLastError(), error.c_str());
        return 1;
    }

    if (!reg.WriteStringValue("Software\\MyTestApp", "Username", "admin", error))
    {
        LY_MSB("Error: %s", error.c_str());
        return 1;
    }

    std::string value;
    if (!reg.ReadStringValue("Software\\MyTestApp", "Username", value, error))
    {
        LY_MSB("Error: %s", error.c_str());
        return 1;
    }

    LY_INF("Data=%s", value.c_str());

    RegistryManager rm2{ HKEY_LOCAL_MACHINE };
    std::string output;
    std::string err;
    if (rm2.ReadStringValue("software\\RegisteredApplications", "File Explorer", output, err))
    {
        LY_INF("%s", output.c_str());
    }

    return 0;
}
