[CmdletBinding()]
param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.6",
    [string]$ProjectPath = "",
    [string]$Target = "CombatLearningAgentEditor",
    [string]$Platform = "Win64",
    [string]$Configuration = "Development"
)

if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $scriptRoot = if ([string]::IsNullOrWhiteSpace($PSScriptRoot)) {
        Split-Path -Parent $MyInvocation.MyCommand.Path
    }
    else {
        $PSScriptRoot
    }

    $ProjectPath = Join-Path $scriptRoot "CombatLearningAgent.uproject"
}

$buildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path $buildBat)) {
    throw "Build script not found: $buildBat"
}

if (-not (Test-Path $ProjectPath)) {
    throw "Project file not found: $ProjectPath"
}

& "$buildBat" $Target $Platform $Configuration "$ProjectPath" -WaitMutex -FromMsBuild
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "Build failed with exit code $exitCode"
}

Write-Host "Build succeeded for $Target ($Platform $Configuration)."
