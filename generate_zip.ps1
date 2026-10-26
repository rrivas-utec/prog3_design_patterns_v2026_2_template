[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectName = 'prog3_design_patterns_v2026_2'
$sourceFiles = @(
    'include/cafeteria.h',
    'include/gastos_personales.h',
    'include/sensores_inteligentes.h',
    'src/cafeteria.cpp',
    'src/gastos_personales.cpp',
    'src/sensores_inteligentes.cpp'
)
$projectRoot = Split-Path -Parent $PSCommandPath
$outputPath = Join-Path $projectRoot "$projectName.zip"

foreach ($sourceFile in $sourceFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot $sourceFile) -PathType Leaf)) {
        throw "No se encontro el archivo requerido: $sourceFile"
    }
}
if (Test-Path -LiteralPath $outputPath) { Remove-Item -LiteralPath $outputPath -Force }
# Preserve the exact include/ and src/ paths without adding unrelated files.
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($outputPath, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($sourceFile in $sourceFiles) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive,
            (Join-Path $projectRoot $sourceFile),
            $sourceFile,
            [System.IO.Compression.CompressionLevel]::Optimal
        ) | Out-Null
    }
}
finally {
    $archive.Dispose()
}
Write-Host "Archivo generado: $outputPath"
