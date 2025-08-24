param (
    [Parameter(Mandatory = $true)]
    [ValidateSet("add","remove")]
    [string]$Action
)

# Constants
$optionID = "MyCustomOption"
$optionName = "My Custom Option"
$regPath = "Registry::HKEY_CLASSES_ROOT\DesktopBackground\Shell\$optionID"
$commandPath = "$regPath\command"
$firstPath = ($env:PATH -split ";")[0]
$batFile = Join-Path $firstPath "myscript.bat"

# Function: Show a message box to the user
function Show-MessageBox {
    param([string]$Message)
    Add-Type -AssemblyName PresentationFramework
    [System.Windows.MessageBox]::Show($Message, $optionName, 'OK', 'Information') | Out-Null
}

# Function: Add registry keys for context menu
function Add-RegistryKeys {
    Write-Host "Adding registry keys..."
    if (-not (Test-Path $regPath)) {
        New-Item -Path $regPath -Force | Out-Null
    }
    Set-ItemProperty -Path $regPath -Name "MUIVerb" -Value $optionName
    Set-ItemProperty -Path $regPath -Name "Icon" -Value "shell32.dll,1"  # First icon in shell32.dll

    if (-not (Test-Path $commandPath)) {
        New-Item -Path $commandPath -Force | Out-Null
    }
    Set-ItemProperty -Path $commandPath -Name "(Default)" -Value "`"$batFile`""
}

# Function: Remove registry keys for context menu
function Remove-RegistryKeys {
    Write-Host "Removing registry keys..."
    if (Test-Path $regPath) {
        Remove-Item -Path $regPath -Recurse -Force
    } else {
        Write-Host "Registry key not found: $regPath"
    }
}

# Function: Create the batch script
function Create-BatchScript {
    Write-Host "Creating batch file at $batFile..."
    if (-not (Test-Path $firstPath)) {
        throw "The first directory in PATH does not exist: $firstPath"
    }

    $batContent = "@echo off
:: List all items in current directory and save to output.txt
dir > output.txt
pause"

    Set-Content -Path $batFile -Value $batContent -Encoding ASCII
}

# Function: Delete the batch script
function Remove-BatchScript {
    Write-Host "Removing batch file..."
    if (Test-Path $batFile) {
        Remove-Item -Path $batFile -Force
    } else {
        Write-Host "Batch file not found: $batFile"
    }
}

# =====================
# MAIN LOGIC
# =====================
try {
    if ($Action -eq "add") {
        Add-RegistryKeys
        Create-BatchScript
        Show-MessageBox "The custom context menu $optionName has been added successfully."
    }
    elseif ($Action -eq "remove") {
        Remove-RegistryKeys
        Remove-BatchScript
        Show-MessageBox "The custom context menu $optionName has been removed successfully."
    }
}
catch {
    Write-Error "An error occurred: $_"
    Show-MessageBox "An error occurred: $_"
}
