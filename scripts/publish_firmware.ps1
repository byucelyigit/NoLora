param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9A-Za-z.\-]+$')]
    [string]$Version
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe'
if (-not (Test-Path $pio)) { $pio = 'pio' }

# Surum numarasi derleme bayragi ile gomulur (src\main.cpp -> FW_VERSION).
$env:PLATFORMIO_BUILD_FLAGS = "-DFW_VERSION=\`"$Version\`""

Push-Location $root
try {
    & $pio run
    if ($LASTEXITCODE -ne 0) { throw "Derleme basarisiz." }
}
finally {
    Pop-Location
    Remove-Item Env:\PLATFORMIO_BUILD_FLAGS -ErrorAction SilentlyContinue
}

$built = Join-Path $root '.pio\build\esp32dev\firmware.bin'
if (-not (Test-Path $built)) { throw "firmware.bin bulunamadi: $built" }

$outDir = Join-Path $root 'web\firmware'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Get-ChildItem $outDir -Filter 'firmware-*.bin' | Remove-Item -Force

$fileName = "firmware-$Version.bin"
$target = Join-Path $outDir $fileName
Copy-Item $built $target -Force

$size = (Get-Item $target).Length
$sha256 = (Get-FileHash $target -Algorithm SHA256).Hash.ToLower()

$manifest = [ordered]@{
    version = $Version
    file    = $fileName
    size    = $size
    sha256  = $sha256
} | ConvertTo-Json

[IO.File]::WriteAllText((Join-Path $outDir 'version.json'), $manifest, (New-Object Text.UTF8Encoding($false)))

Write-Host ""
Write-Host "Hazir: web\firmware\$fileName ($size bayt)"
Write-Host "SHA-256: $sha256"
Write-Host "Yayinlamak icin: git add web/firmware; git commit; git push (main)"
