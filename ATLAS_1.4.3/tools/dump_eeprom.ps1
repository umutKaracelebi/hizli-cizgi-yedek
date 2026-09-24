<#
  dump_eeprom.ps1 - ATLAS_1.4.3 EEPROM debug izini okur ve cozumler.
  USB-TTL / serial adaptor GEREKMEZ: kodu yuklediginiz programlayici
  (USBasp, USBtinyISP, Arduino as ISP, USBasp klonu, ...) yeterlidir.

  Ornekler:
    # 1) USBasp ile oku (varsayilan)
    powershell -File .\tools\dump_eeprom.ps1

    # 2) Baska programlayici / port
    powershell -File .\tools\dump_eeprom.ps1 -Programmer arduinoasisp -Port COM7

    # 3) Sadece daha once alinmis bir .bin dosyasini cozumle (donanim gerekmez)
    powershell -File .\tools\dump_eeprom.ps1 -DecodeOnly .\tools\logs\atlas_log.bin

    # 4) Logu temizle (0xFF ile doldur)
    powershell -File .\tools\dump_eeprom.ps1 -Clear

  Log bicimi icin TraceMode.ino basligina bakin.
#>
param(
  [string]$Programmer = 'usbasp',
  [string]$Mcu = 'm328p',
  [string]$Port,
  [string]$OutFile,
  [string]$DecodeOnly,
  [string]$Avrdude,
  [switch]$Clear
)

$ErrorActionPreference = 'Stop'

$ModeNames = @{
  0 = 'RAW SENSORS'
  1 = 'CALIBRATED SENSORS'
  2 = 'CALIBRATION (MAX/MIN/TH)'
  3 = 'LINE POSITION'
  4 = 'IMPELLER / POWER (tribun)'
}
$LogVersion = 1
$LogOffData = 12

function Resolve-AvrdudeExe {
  param([string]$Explicit)
  if ($Explicit) { return $Explicit }
  if (Get-Command avrdude.exe -ErrorAction SilentlyContinue) { return 'avrdude.exe' }
  $root = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\arduino\tools\avrdude'
  $found = Get-ChildItem $root -Recurse -Filter 'avrdude.exe' -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty FullName
  if (-not $found) { throw 'avrdude.exe bulunamadi. -Avrdude "<tam yol>\avrdude.exe" ile verin.' }
  return $found
}

function Resolve-AvrdudeConf {
  param([string]$Exe)
  $candidate = Join-Path (Split-Path (Split-Path $Exe -Parent) -Parent) 'etc\avrdude.conf'
  if (Test-Path $candidate) { return $candidate }
  return $null
}

# ------------------------------------------------------------ cozumleyici
function Show-AtlasEepromLog {
  param([string]$Path)

  $bytes = [System.IO.File]::ReadAllBytes($Path)
  if ($bytes.Length -lt $LogOffData) { Write-Host "Dosya cok kucuk: $Path" -ForegroundColor Red; return }

  $magic = [string][char]$bytes[0] + [char]$bytes[1] + [char]$bytes[2] + [char]$bytes[3]
  Write-Host ''
  Write-Host "EEPROM izi : $Path" -ForegroundColor Cyan

  if ($magic -ne 'ATLS') {
    Write-Host "Gecerli ATLAS logu bulunamadi (magic='$magic')." -ForegroundColor Yellow
    Write-Host 'Once debugModeNoSerial() ile bir test calistirin, sonra tekrar okuyun.'
    return
  }

  $version = $bytes[4]
  $mode = $bytes[5]
  $count = $bytes[6]
  $recordLength = $bytes[7]
  $flags = $bytes[8]
  $period = [int]$bytes[9] + (([int]$bytes[10]) -shl 8)

  if ($version -ne $LogVersion) { Write-Host "Log surumu uyumsuz ($version != $LogVersion)." -ForegroundColor Yellow }

  $modeName = $ModeNames[[int]$mode]
  if (-not $modeName) { $modeName = "UNKNOWN ($mode)" }

  Write-Host ("Mode       : {0}" -f $modeName)
  Write-Host ("Kayit      : {0} adet x {1} byte, periyot {2} ms" -f $count, $recordLength, $period)
  Write-Host ("Flags      : 0x{0:X2} (invertSensorReads={1})" -f $flags, ($flags -band 1))

  if ($count -eq 0 -or $recordLength -eq 0) { Write-Host 'Log bos.' -ForegroundColor Yellow; return }

  $maxRecords = [math]::Floor(($bytes.Length - $LogOffData) / $recordLength)
  if ($count -gt $maxRecords) { $count = $maxRecords }

  $csvRows = New-Object System.Collections.Generic.List[string]
  $sensorHeader = '#;t_ms' + (0..15 | ForEach-Object { ";S$_" }) -join ''
  $sensorTitles = ' #  t(ms)   ' + ((0..15 | ForEach-Object { '{0,4}' -f "S$_" }) -join '')

  switch ([int]$mode) {
    0 {
      $csvRows.Add($sensorHeader)
      $sensorMin = 0..15 | ForEach-Object { 255 }
      $sensorMax = 0..15 | ForEach-Object { 0 }
      Write-Host $sensorTitles
      for ($i = 0; $i -lt $count; $i++) {
        $vals = @()
        for ($s = 0; $s -lt 16; $s++) { $vals += $bytes[$LogOffData + $i * $recordLength + $s] }
        for ($s = 0; $s -lt 16; $s++) {
          if ($vals[$s] -lt $sensorMin[$s]) { $sensorMin[$s] = $vals[$s] }
          if ($vals[$s] -gt $sensorMax[$s]) { $sensorMax[$s] = $vals[$s] }
        }
        Write-Host (('{0,3}{1,7}   ' -f $i, ($i * $period)) + (($vals | ForEach-Object { '{0,4}' -f $_ }) -join ''))
        $csvRows.Add(('{0};{1};{2}' -f $i, ($i * $period), ($vals -join ';')))
      }
      Write-Host ''
      Write-Host ('MIN      ' + (($sensorMin | ForEach-Object { '{0,4}' -f $_ }) -join '')) -ForegroundColor DarkGray
      Write-Host ('MAX      ' + (($sensorMax | ForEach-Object { '{0,4}' -f $_ }) -join '')) -ForegroundColor DarkGray
    }

    1 {
      $csvRows.Add($sensorHeader)
      Write-Host $sensorTitles
      for ($i = 0; $i -lt $count; $i++) {
        $vals = @()
        for ($s = 0; $s -lt 16; $s++) { $vals += $bytes[$LogOffData + $i * $recordLength + $s] }
        $mark = ''
        for ($s = 0; $s -lt 16; $s++) { if ($vals[$s] -gt 0) { $mark += '^' } else { $mark += '.' } }
        Write-Host (('{0,3}{1,7}   ' -f $i, ($i * $period)) + (($vals | ForEach-Object { '{0,4}' -f $_ }) -join '') + "  $mark")
        $csvRows.Add(('{0};{1};{2}' -f $i, ($i * $period), ($vals -join ';')))
      }
      Write-Host ''
      Write-Host '^ = o anda cizgiyi goren sensor' -ForegroundColor DarkGray
    }

    2 {
      $maxValues = @(); $minValues = @(); $thValues = @()
      for ($s = 0; $s -lt 16; $s++) {
        $maxValues += $bytes[$LogOffData + $s]
        $minValues += $bytes[$LogOffData + 16 + $s]
        $thValues += $bytes[$LogOffData + 32 + $s]
      }
      Write-Host ''
      Write-Host ('MAX      ' + (($maxValues | ForEach-Object { '{0,4}' -f $_ }) -join '')) -ForegroundColor Green
      Write-Host ('MIN      ' + (($minValues | ForEach-Object { '{0,4}' -f $_ }) -join '')) -ForegroundColor Green
      Write-Host ('TH       ' + (($thValues | ForEach-Object { '{0,4}' -f $_ }) -join '')) -ForegroundColor Green
      Write-Host ''
      Write-Host 'Kontrol: her sensor icin MAX > MIN olmali (kontrast), TH ikisinin arasinda olmali.' -ForegroundColor DarkGray
      $csvRows.Add(';' + ((0..15 | ForEach-Object { "S$_" }) -join ';'))
      $csvRows.Add('MAX;' + ($maxValues -join ';'))
      $csvRows.Add('MIN;' + ($minValues -join ';'))
      $csvRows.Add('TH;' + ($thValues -join ';'))
    }

    3 {
      $csvRows.Add('#;t_ms;position;position_1000;on_line')
      Write-Host ' #  t(ms)   position  pos/1000  online'
      for ($i = 0; $i -lt $count; $i++) {
        $base = $LogOffData + $i * $recordLength
        $position = [int]$bytes[$base] + (([int]$bytes[$base + 1]) -shl 8)
        $online = $bytes[$base + 2] -band 1
        Write-Host ('{0,3}{1,7}{2,11}{3,10}{4,8}' -f $i, ($i * $period), $position, ([math]::Round($position / 1000.0, 2)), $online)
        $csvRows.Add(('{0};{1};{2};{3};{4}' -f $i, ($i * $period), $position, ([math]::Round($position / 1000.0, 2)), $online))
      }
      Write-Host ''
      Write-Host 'position: 0 = en sol sensor, 15000 = en sag sensor (1000 x sensor index)' -ForegroundColor DarkGray
    }

    4 {
      $csvRows.Add('#;t_ms;impeller_pwm;adc6;adc7')
      Write-Host ' #  t(ms)   pwm  ADC6  ADC7   (pwm 0 = fan kapali)'

      $onSamples = 0
      $offSamples = 0
      $a6OnMin = 255; $a6OnMax = 0; $a7OnMin = 255; $a7OnMax = 0
      $a6OffMin = 255; $a6OffMax = 0; $a7OffMin = 255; $a7OffMax = 0

      for ($i = 0; $i -lt $count; $i++) {
        $base = $LogOffData + $i * $recordLength
        $pwm = $bytes[$base]
        $a6 = $bytes[$base + 1]
        $a7 = $bytes[$base + 2]

        if ($pwm -gt 0) {
          $onSamples++
          if ($a6 -lt $a6OnMin) { $a6OnMin = $a6 }
          if ($a6 -gt $a6OnMax) { $a6OnMax = $a6 }
          if ($a7 -lt $a7OnMin) { $a7OnMin = $a7 }
          if ($a7 -gt $a7OnMax) { $a7OnMax = $a7 }
        }
        else {
          $offSamples++
          if ($a6 -lt $a6OffMin) { $a6OffMin = $a6 }
          if ($a6 -gt $a6OffMax) { $a6OffMax = $a6 }
          if ($a7 -lt $a7OffMin) { $a7OffMin = $a7 }
          if ($a7 -gt $a7OffMax) { $a7OffMax = $a7 }
        }

        Write-Host ('{0,3}{1,7}{2,6}{3,6}{4,6}' -f $i, ($i * $period), $pwm, $a6, $a7)
        $csvRows.Add(('{0};{1};{2};{3};{4}' -f $i, ($i * $period), $pwm, $a6, $a7))
      }

      Write-Host ''
      Write-Host ("Fan acik ornek : {0}   fan kapali ornek: {1}" -f $onSamples, $offSamples)
      if ($onSamples -gt 0) {
        Write-Host ("ADC6 (fan acik) : min {0}  max {1}" -f $a6OnMin, $a6OnMax) -ForegroundColor Green
        Write-Host ("ADC7 (fan acik) : min {0}  max {1}" -f $a7OnMin, $a7OnMax) -ForegroundColor Green
      }
      if ($offSamples -gt 0) {
        Write-Host ("ADC6 (fan kapali): min {0}  max {1}" -f $a6OffMin, $a6OffMax) -ForegroundColor DarkGray
        Write-Host ("ADC7 (fan kapali): min {0}  max {1}" -f $a7OffMin, $a7OffMax) -ForegroundColor DarkGray
      }
      Write-Host ''
      Write-Host 'Yorum: fan acikken ADC6/ADC7 degerinin dusmesi (sag) pil / konnektor /' -ForegroundColor DarkGray
      Write-Host 'kablo direncinin zayif oldugunu gosterir. PWM artarken sag da artmali.' -ForegroundColor DarkGray
    }

    default {
      Write-Host 'Bilinmeyen mod, ham kayitlar dokulur.' -ForegroundColor Yellow
      for ($i = 0; $i -lt $count; $i++) {
        $vals = @()
        for ($s = 0; $s -lt $recordLength; $s++) { $vals += $bytes[$LogOffData + $i * $recordLength + $s] }
        Write-Host (('{0,3}  ' -f $i) + ($vals -join ' '))
      }
    }
  }

  $csvPath = [System.IO.Path]::ChangeExtension($Path, '.csv')
  [System.IO.File]::WriteAllLines($csvPath, $csvRows)
  Write-Host ''
  Write-Host "CSV: $csvPath" -ForegroundColor Cyan
}

# ------------------------------------------------------------------ main
if ($DecodeOnly) {
  if (-not (Test-Path $DecodeOnly)) { throw "Dosya yok: $DecodeOnly" }
  Show-AtlasEepromLog -Path (Resolve-Path $DecodeOnly).Path
  return
}

$avrdudeExe = Resolve-AvrdudeExe -Explicit $Avrdude
$conf = Resolve-AvrdudeConf -Exe $avrdudeExe
$avrArgs = @()
if ($conf) { $avrArgs += @('-C', $conf) }
$avrArgs += @('-c', $Programmer, '-p', $Mcu)
if ($Port) { $avrArgs += @('-P', $Port) }

if (-not $OutFile) {
  $logDir = Join-Path $PSScriptRoot 'logs'
  New-Item -ItemType Directory -Force -Path $logDir | Out-Null
  $OutFile = Join-Path $logDir ('atlas_log_{0}.bin' -f (Get-Date -Format 'yyyyMMdd_HHmmss'))
}
$OutFile = [System.IO.Path]::GetFullPath($OutFile)
New-Item -ItemType Directory -Force -Path (Split-Path $OutFile -Parent) | Out-Null

if ($Clear) {
  $blank = Join-Path $env:TEMP 'atlas_eeprom_blank.bin'
  $buf = New-Object byte[] 1024
  for ($i = 0; $i -lt 1024; $i++) { $buf[$i] = 0xFF }
  [System.IO.File]::WriteAllBytes($blank, $buf)
  $avrArgs += @('-U', "eeprom:w:${blank}:r")
  Write-Host "EEPROM temizleniyor ($avrdudeExe)" -ForegroundColor Yellow
}
else {
  $avrArgs += @('-U', "eeprom:r:${OutFile}:r")
  Write-Host "EEPROM okunuyor -> $OutFile" -ForegroundColor Yellow
}

Write-Host "Komut: $avrdudeExe $($avrArgs -join ' ')" -ForegroundColor DarkGray
& $avrdudeExe @avrArgs
if ($LASTEXITCODE -ne 0) { throw "avrdude hata kodu: $LASTEXITCODE" }

if (-not $Clear) { Show-AtlasEepromLog -Path $OutFile }
