:: this script configures the environemnt variables required for emscripten
call C:\work\Nebula-Viewer\Code\Libs\SDKs\emsdk\emsdk_env.bat
:: add ninja to the PATH
set PATH=%PATH%;C:\work\Nebula-Viewer\_Output\tools\bin\ninja
:: BuildType can be Debug or Release
set BuildType=Debug
:: generate project files using cmake
for /l %%x in (1, 1, 2) do (
	:: We run the command twice because it fails the first time, and I don't know how to fix it (https://github.com/emscripten-core/emscripten/issues/14762)
	call emcmake.bat C:\work\Nebula-Viewer\_Output\tools\bin\CMake\win32\bin\cmake -S . -B build_web -DBUILD_SHARED_LIBS=OFF -DCMAKE_RULE_MESSAGES:BOOL=OFF -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF -GNinja -DCMAKE_BUILD_TYPE=%BuildType%
)
:: build
C:\work\Nebula-Viewer\_Output\tools\bin\CMake\win32\bin\cmake --build build_web