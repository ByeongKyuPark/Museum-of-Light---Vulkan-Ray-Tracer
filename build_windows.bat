cd build || goto :error
if not exist windows mkdir windows
cd windows || goto :error
cmake -D VCPKG_TARGET_TRIPLET=x64-windows-static -D CMAKE_TOOLCHAIN_FILE=../vcpkg.windows/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A "x64" ../.. || goto :error
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" RayTracingInVulkan.sln /t:Rebuild /p:Configuration=Release || goto :error
cd ..


cd ..d || goto :error
if not exist windows mkdir windows || goto :error
cd windows || goto :error
cmake -D VCPKG_TARGET_TRIPLET=x64-windows-static -D CMAKE_TOOLCHAIN_FILE=../vcpkg.windows/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A "x64" ../.. || goto :error
msbuild RayTracingInVulkan.sln /t:Rebuild /p:Configuration=Release || goto :error
cd ..
cd ..

exit /b


:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%