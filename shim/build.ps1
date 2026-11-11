# Rebuild recshim artifacts into assets/ (same flags as the CI workflow).
# Usage: pwsh shim/build.ps1   (zig must be on PATH)
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$assets = Join-Path $root 'assets'
New-Item -ItemType Directory -Force $assets | Out-Null

$targets = @(
    @{ triple = 'x86_64-windows-gnu';   arch = 'amd64' },
    @{ triple = 'aarch64-windows-gnu';  arch = 'arm64' }
)

foreach ($t in $targets) {
    $out = Join-Path $assets "recshim-$($t.arch).exe"
    zig cc -target $t.triple -Os -s -lkernel32 -lshell32 `
        -o $out (Join-Path $PSScriptRoot 'recshim.c')
    if ($LASTEXITCODE -ne 0) { throw "zig cc failed for $($t.triple)" }
    Write-Host "built $out"
}

Get-ChildItem (Join-Path $assets 'recshim-*.exe') | Select-Object Name, Length
