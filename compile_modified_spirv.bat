@echo off
cd assets
IF NOT EXIST "shaders" mkdir "shaders"
cd ..
for /f %%f in ('git ls-files -m') do (
    if %%~xf == .vert (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv --target-env=vulkan1.3
        echo %%~xnf.spv is compiled ...
    )
    if %%~xf == .frag (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv --target-env=vulkan1.3
        echo %%~xnf.spv is compiled ...
    )
    if %%~xf == .comp (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv --target-env=vulkan1.3
        echo %%~xnf.spv is compiled ...
    )
)
for /f %%f in ('git ls-files --others --exclude-standard') do (
    if %%~xf == .vert (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv --target-env=vulkan1.3
        echo %%~xnf.spv is compiled ...
    )
    if %%~xf == .frag (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv --target-env=vulkan1.3
        echo %%~xnf.spv is compiled ...
    )
    if %%~xf == .comp (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv --target-env=vulkan1.3
        echo %%~xnf.spv is compiled ...
    )
)
echo SPIR-V compilation completes!
pause