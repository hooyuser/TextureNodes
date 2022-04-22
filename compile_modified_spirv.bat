@echo off

IF NOT EXIST "assets\shaders" mkdir "assets\shaders"

ECHO [GLSLC] SPIRV compilation starts!

for /f %%f in ('git diff --name-only --diff-filter=M') do (
    if %%~xf == .vert (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo   "%%~xnf" is compiled ...
    )
    if %%~xf == .frag (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo   "%%~xnf" is compiled ...
    )
    if %%~xf == .comp (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo   "%%~xnf" is compiled ...
    )
)
for /f %%f in ('git ls-files --others --exclude-standard') do (
    if %%~xf == .vert (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo   "%%~xnf" is compiled ...
    )
    if %%~xf == .frag (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo   "%%~xnf" is compiled ...
    )
    if %%~xf == .comp (
        .\extern\vulkan\Bin\glslc.exe %~dp0assets\glsl_shaders\%%~xnf -o %~dp0assets\shaders\%%~xnf.spv
        echo   "%%~xnf" is compiled ...
    )
)

echo [GLSLC] SPIR-V compilation completes!

pause