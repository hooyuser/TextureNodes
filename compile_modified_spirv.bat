@echo off
cd assets
IF NOT EXIST "shaders" mkdir "shaders"
cd ..
for /f %%f in ('git ls-files -m') do (
    if %%~xf == .vert goto compile_spirv
    if %%~xf == .frag (
        :compile_spirv
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo %%~xnf.spv is compiled ...
    )
)
echo SPIR-V compilation completes!
pause