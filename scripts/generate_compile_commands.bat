pushd "%~dp0.."

cmake^
 -B build-analysis^
 -G Ninja^
 -S .^
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

popd
