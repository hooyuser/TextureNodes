@echo off
cd assets
IF NOT EXIST "shaders" mkdir "shaders"
cd ..
for /f %%f in ('dir /b %~dp0assets\glsl_shaders') do (
    .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%f -o %~dp0assets\shaders\%%f.spv
    echo %%f.spv is compiled ...
)
echo SPIRV compilation completes!
pause