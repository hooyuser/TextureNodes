@echo off
cd assets
IF NOT EXIST "shaders" mkdir "shaders"
cd ..
for /f %%f in ('git ls-files -m') do (
    if %%~xf == .vert (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo %%~xnf.spv is compiled ...
    )
    if %%~xf == .frag (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo %%~xnf.spv is compiled ...
    )
)
echo SPIR-V compilation completes!
pause