$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build --config Release --parallel
if ($LASTEXITCODE) { exit $LASTEXITCODE }
ctest --test-dir build -C Release --output-on-failure
if ($LASTEXITCODE) { exit $LASTEXITCODE }
$plugin = Join-Path $PWD 'build/RangeMasterLab_artefacts/Release/VST3/c.d.r.c plugins - RangeMaster.vst3'
& 'build/RangeMasterHostCheck_artefacts/Release/RangeMasterHostCheck.exe' $plugin
exit $LASTEXITCODE
