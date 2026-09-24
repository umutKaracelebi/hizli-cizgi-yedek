<#
  build_check.ps1 - ATLAS_1.4.3 sketch'ini Arduino IDE olmadan derleyip
  flash / RAM kullanimini raporlar.

  Kullanim (PowerShell):
      powershell -ExecutionPolicy Bypass -File .\tools\build_check.ps1

  Gereksinim: Arduino IDE'nin indirdigi avr-gcc + ctags paketleri
  (%LOCALAPPDATA%\Arduino15\packages\...)
#>
param(
  [string]$SketchDir = (Split-Path -Parent $PSScriptRoot),
  [string]$BuildDir = (Join-Path $env:TEMP 'atlas_build'),
  [switch]$NoSerialDebug
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------- araclar
$Arduino15 = Join-Path $env:LOCALAPPDATA 'Arduino15'
$CoreVer = '1.8.8'
$CoreRoot = Join-Path $Arduino15 "packages\arduino\hardware\avr\$CoreVer"
$GccDir = (Get-ChildItem (Join-Path $Arduino15 'packages\arduino\tools\avr-gcc') -Directory |
  Sort-Object Name -Descending | Select-Object -First 1).FullName
$GccBin = Join-Path $GccDir 'bin'
$Gpp = Join-Path $GccBin 'avr-g++.exe'
$Gcc = Join-Path $GccBin 'avr-gcc.exe'
$SizeExe = Join-Path $GccBin 'avr-size.exe'
$ObjCopy = Join-Path $GccBin 'avr-objcopy.exe'
$Ctags = (Get-ChildItem (Join-Path $Arduino15 'packages\builtin\tools\ctags') -Recurse -Filter ctags.exe |
  Select-Object -First 1).FullName

foreach ($t in @($Gpp, $Gcc, $SizeExe, $ObjCopy, $Ctags)) {
  if (-not (Test-Path $t)) { throw "Bulunamadi: $t" }
}

$Mcpu = 'atmega328p'
$Fcpu = '16000000L'
$Defines = @(
  '-DARDUINO=10819',
  '-DARDUINO_AVR_UNO',
  '-DARDUINO_ARCH_AVR',
  "-DF_CPU=$Fcpu"
)
$Includes = @(
  (Join-Path $CoreRoot 'cores\arduino'),
  (Join-Path $CoreRoot 'variants\standard'),
  $SketchDir
) | ForEach-Object { "-I$_" }

$BaseFlags = @('-c', '-g', '-Os', '-w', "-mmcu=$Mcpu", '-ffunction-sections', '-fdata-sections', '-MMD', '-flto') +
  $Defines + $Includes
$CFlags = $BaseFlags + @('-std=gnu11')
$CppFlags = $BaseFlags + @('-std=gnu++11', '-fpermissive', '-fno-exceptions', '-fno-threadsafe-statics')
$LdFlags = @('-Os', "-mmcu=$Mcpu", '-Wl,--gc-sections', '-flto', '-fuse-linker-plugin')

# ------------------------------------------------------------- temiz kopya
# -NoSerialDebug: sketch'in kopyasi uzerinde DEBUG_MODE_USE_SERIAL = 0 yapip
# EEPROM (USB-TTL'siz) debug yolunun da derlendigini dogrular
if ($NoSerialDebug) {
  $origDir = $SketchDir
  $mainInoName = (Split-Path $origDir -Leaf) + '.ino'
  $copyDir = Join-Path $env:TEMP 'atlas_sketch_noserial'
  if (Test-Path $copyDir) { Remove-Item $copyDir -Recurse -Force }
  New-Item -ItemType Directory -Path $copyDir | Out-Null
  Get-ChildItem $origDir -File -Filter '*.ino' | Copy-Item -Destination $copyDir -Force
  $mainCopy = Join-Path $copyDir $mainInoName
  $src = Get-Content -Raw $mainCopy
  if ($src -notmatch '#define DEBUG_MODE_USE_SERIAL\s+1') { throw 'DEBUG_MODE_USE_SERIAL satiri bulunamadi.' }
  $src = $src -replace '#define DEBUG_MODE_USE_SERIAL\s+1', '#define DEBUG_MODE_USE_SERIAL 0'
  [System.IO.File]::WriteAllText($mainCopy, $src)
  $SketchDir = $copyDir
  $BuildDir = Join-Path $env:TEMP 'atlas_build_noserial'
  Write-Host 'Mod: DEBUG_MODE_USE_SERIAL = 0 (EEPROM trace, USB-TTL yok)' -ForegroundColor Yellow
}

if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force }
New-Item -ItemType Directory -Path $BuildDir | Out-Null
$objDir = Join-Path $BuildDir 'obj'
New-Item -ItemType Directory -Path $objDir | Out-Null

# ------------------------------------------------- .ino dosyalarini birlestir
$sketchName = Split-Path $SketchDir -Leaf
if (-not $mainInoName) { $mainInoName = $sketchName + '.ino' }
$mainIno = Join-Path $SketchDir $mainInoName
$inos = @($mainIno) + (Get-ChildItem $SketchDir -Filter '*.ino' |
  Where-Object { $_.FullName -ne $mainIno } | Sort-Object Name | Select-Object -ExpandProperty FullName)

Write-Host "Sketch dosyalari:" -ForegroundColor Cyan
$inos | ForEach-Object { Write-Host "  $(Split-Path $_ -Leaf)" }

# Arduino IDE gibi fonksiyon prototiplerini (ctags ile) uret
$protoLines = & $Ctags -u --language-force=c++ -f - --c++-kinds=pf --fields=+iaS @inos
$protos = New-Object System.Collections.Generic.List[string]
foreach ($line in $protoLines) {
  $f = $line -split "`t"
  if ($f.Count -lt 4) { continue }
  $pattern = $f[2]
  if ($pattern -notmatch '^/\^(.*)\$/;"$') { continue }
  $decl = $Matches[1] -replace '\\', ''
  $decl = ($decl -split '{')[0].TrimEnd()
  if ($decl.Length -eq 0) { continue }
  $protos.Add("$decl;")
}
$protoBlock = ($protos | Select-Object -Unique) -join "`r`n"

$sketchCpp = Join-Path $BuildDir 'sketch.ino.cpp'
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('#include <Arduino.h>')
[void]$sb.AppendLine($protoBlock)
foreach ($ino in $inos) {
  [void]$sb.AppendLine((Get-Content -Raw $ino))
}
[System.IO.File]::WriteAllText($sketchCpp, $sb.ToString())

# --------------------------------------------------------------- derleme
$objs = New-Object System.Collections.Generic.List[string]
$failed = $false

function Build-One([string]$src, [string[]]$flags) {
  $rel = $src.Substring(3) -replace '[\\/:]', '_'
  $out = Join-Path $objDir ($rel + '.o')
  $compiler = if ($src -match '\.(c|S)$') { $script:Gcc } else { $script:Gpp }
  & $compiler @flags -o $out $src 2>&1 | ForEach-Object { Write-Host $_ }
  if ($LASTEXITCODE -ne 0) { $script:failed = $true; Write-Host "HATA: $src" -ForegroundColor Red }
  else { $script:objs.Add($out) }
}

Build-One $sketchCpp $CppFlags

$coreSrcDirs = @((Join-Path $CoreRoot 'cores\arduino'), (Join-Path $CoreRoot 'variants\standard'))
foreach ($dir in $coreSrcDirs) {
  foreach ($src in (Get-ChildItem $dir -File | Where-Object { $_.Extension -in '.c', '.cpp', '.S' })) {
    Build-One $src.FullName $(if ($src.Extension -eq '.cpp') { $CppFlags } else { $CFlags })
  }
}

if ($failed) { Write-Host "DERLEME BASARISIZ" -ForegroundColor Red; exit 1 }

# ------------------------------------------------------------------- link
$elf = Join-Path $BuildDir 'ATLAS_1.4.3.elf'
& $Gpp @LdFlags -o $elf @objs "-Wl,-Map=$BuildDir\ATLAS_1.4.3.map" -lm 2>&1 | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0) { Write-Host "LINK BASARISIZ" -ForegroundColor Red; exit 1 }

$hex = Join-Path $BuildDir 'ATLAS_1.4.3.hex'
& $ObjCopy -O ihex -R .eeprom $elf $hex

# ------------------------------------------------------------------- sonuc
Write-Host ''
Write-Host '=== DERLEME BASARILI ===' -ForegroundColor Green
$sizeTxt = (& $SizeExe --format=avr --mcu=$Mcpu $elf) -join "`r`n"
Write-Host $sizeTxt
$elfBytes = (Get-Item $elf).Length
Write-Host "ELF: $elf"
Write-Host "HEX: $hex ($((Get-Item $hex).Length) byte)"
