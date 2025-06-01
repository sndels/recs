if not exist "%~dp0..\build" mkdir %~dp0..\build
pushd "%~dp0..\build"

call ..\scripts\cmake.bat

msbuild^
 /property:GenerateFullPaths=true^
 /t:build^
 /consoleloggerparameters:NoSummary^
 /property:Configuration=%1^
 %2.vcxproj || goto :error

.\Debug\%2.exe || goto :error

popd
goto :EOF

:error
popd
exit /b
