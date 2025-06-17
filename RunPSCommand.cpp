#include <windows.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

std::string EscapeCommandForPowerShell(const std::string& raw)
{
    std::ostringstream oss;
    for (char ch : raw)
    {
        if (ch == '"') oss << '\\';  // Escape quote for cmd line
        oss << ch;
    }
    return oss.str();
}

std::string ExecuteCommand(const std::string& command, int timeout)
{
    HANDLE hRead = NULL, hWrite = NULL;
    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hRead, &hWrite, &saAttr, 0))
        return "ERROR: Cannot create pipe.";

    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    std::string escapedCommand = EscapeCommandForPowerShell(command);
    std::string cmdLineStr = "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"" + escapedCommand + "\"";
    char* cmdLine = _strdup(cmdLineStr.c_str());

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.hStdInput = NULL;

    PROCESS_INFORMATION pi = {};
    BOOL success = CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    free(cmdLine);
    CloseHandle(hWrite);

    if (!success)
    {
        CloseHandle(hRead);
        return "ERROR: Cannot create process.";
    }

    std::string output;
    const DWORD bufSize = 4096;
    char buffer[bufSize];
    DWORD bytesRead = 0;
    DWORD totalWait = 0;
    const DWORD sleepStep = 50;

    while (true)
    {
        DWORD bytesAvailable = 0;
        if (PeekNamedPipe(hRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0)
        {
            if (ReadFile(hRead, buffer, bufSize - 1, &bytesRead, NULL) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                output += buffer;
            }
        }

        DWORD waitResult = WaitForSingleObject(pi.hProcess, 0);
        if (waitResult == WAIT_OBJECT_0)
            break;

        if (timeout >= 0 && totalWait >= (DWORD)timeout)
        {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            CloseHandle(hRead);
            return "TIMEOUT!";
        }

        Sleep(sleepStep);
        totalWait += sleepStep;
    }

    // Final flush
    while (ReadFile(hRead, buffer, bufSize - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hRead);

    return output;
}

std::string RunMultipleCommands(const std::vector<std::string>& commands, int timeoutPerCmd, int totalTimeout)
{
    DWORD startTime = GetTickCount();
    std::ostringstream combinedOutput;

    for (size_t i = 0; i < commands.size(); ++i)
    {
        DWORD elapsed = GetTickCount() - startTime;
        if (totalTimeout >= 0 && elapsed >= (DWORD)totalTimeout)
        {
            combinedOutput << ">> [Command #" << (i + 1) << "] Skipped due to total timeout.\n";
            break;
        }

        int remainingTotal = totalTimeout >= 0 ? (int)(totalTimeout - elapsed) : -1;
        if (remainingTotal <= 0 && totalTimeout >= 0)
        {
            combinedOutput << ">> [Command #" << (i + 1) << "] Skipped due to total timeout.\n";
            break;
        }

        int effectiveTimeout = timeoutPerCmd;
        if (timeoutPerCmd < 0 && remainingTotal >= 0)
            effectiveTimeout = remainingTotal;
        else if (timeoutPerCmd >= 0 && remainingTotal >= 0)
            effectiveTimeout = min(timeoutPerCmd, remainingTotal);

        const std::string& cmd = commands[i];
        std::string result = ExecuteCommand(cmd, effectiveTimeout);

        combinedOutput << ">> [Command #" << (i + 1) << "]: " << cmd << "\n" << result << "\n";
    }

    return combinedOutput.str();
}


int main()
{
    std::vector<std::string> commands = {
        R"(Write-Output 'Simple test')",
        R"(Write-Output "PowerShell says: `"Quoted Text`"")",
        R"(Start-Sleep -Seconds 4; Write-Output 'Done')",
        R"(Write-Output 'Unclosed string)",
        R"(Get-Item 'Z:\NoSuchPath')"
    };

    std::string result = RunMultipleCommands(commands, 3000, 8000);
    std::cout << result << std::endl;
}

