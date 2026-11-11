#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
mkdir -p dist
ditto 'build/RangeMasterLab_artefacts/Release/AU/c.d.r.c plugins - RangeMaster.component' 'dist/c.d.r.c plugins - RangeMaster.component'
ditto 'build/RangeMasterLab_artefacts/Release/VST3/c.d.r.c plugins - RangeMaster.vst3' 'dist/c.d.r.c plugins - RangeMaster.vst3'
codesign --force --deep --sign - 'dist/c.d.r.c plugins - RangeMaster.component'
codesign --force --deep --sign - 'dist/c.d.r.c plugins - RangeMaster.vst3'
build/RangeMasterHostCheck_artefacts/Release/RangeMasterHostCheck "$(pwd)/dist/c.d.r.c plugins - RangeMaster.vst3"
echo 'Built universal macOS AU + VST3 in dist/. Install the AU and run auval -v aufx RmLb CdrC.'
