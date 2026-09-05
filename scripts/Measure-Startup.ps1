param(
    [string]$PackageDirectory = "$PSScriptRoot/../Builds/Windows_M0_Current",
    [int]$Seconds = 60,
    [int]$Frames = 300
)

$ErrorActionPreference = 'Stop'
$packageRoot = (Resolve-Path -LiteralPath $PackageDirectory).Path
$resultDirectory = Join-Path $PSScriptRoot "../Saved/Verification/Startup-$(Get-Date -Format yyyyMMdd-HHmmss)"
New-Item -ItemType Directory -Path $resultDirectory | Out-Null
$resultDirectory = (Resolve-Path -LiteralPath $resultDirectory).Path

# 保留 CSV 采集，使用引擎支持的同步处理，避免 UE 5.8 CSV 线程退出时清理 TLS 崩溃。
# 同步处理会增加游戏线程开销，后续对比必须沿用相同采集方式。
$arguments = '-windowed -ResX=1280 -ResY=720 -unattended -nosplash ' +
    "-seconds=$Seconds -csvCaptureFrames=$Frames -csvCompression=0 -csvNoProcessingThread " +
    "-abslog=`"$resultDirectory/Runtime.log`" " +
    '-ExecCmds="obj list class=StrategyUnit,sg.ViewDistanceQuality,sg.ShadowQuality,sg.GlobalIlluminationQuality,sg.ReflectionQuality,sg.PostProcessQuality"'
$process = Start-Process -FilePath "$packageRoot/RTS/Binaries/Win64/RTS.exe" `
    -ArgumentList $arguments -WorkingDirectory $packageRoot -WindowStyle Hidden -PassThru
$process.WaitForExit()
$runtimeLog = Get-Content -LiteralPath "$resultDirectory/Runtime.log"
$result = [ordered]@{
    Package = $packageRoot
    ExitCode = $process.ExitCode
    RequestedSeconds = $Seconds
    RequestedFrames = $Frames
    CsvCompleted = [bool]($runtimeLog -match 'Capture Ended. Writing CSV')
    RuntimeErrors = @($runtimeLog | Select-String 'Error:|Fatal error|Assertion failed').Count
    CaptureMode = 'csvNoProcessingThread; hidden window; no player input'
    Resolution = '1280x720'
    FullMatchAccepted = $false
}
$result | ConvertTo-Json | Set-Content -LiteralPath "$resultDirectory/Result.json" -Encoding UTF8
$result | ConvertTo-Json
Write-Output "Evidence: $resultDirectory"
if ($process.ExitCode -ne 0 -or -not $result.CsvCompleted -or $result.RuntimeErrors -ne 0) {
    throw "启动采集未通过，参见 $resultDirectory"
}
