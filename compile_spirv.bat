@echo off

IF NOT EXIST "assets\shaders" mkdir "assets\shaders"

SET asset_path=%~dp0assets
SET glsl_shader_path=%asset_path%\glsl_shaders

ECHO [GLSLC] SPIRV compilation starts!

FOR %%f IN (%glsl_shader_path%\*.vert, %glsl_shader_path%\*.frag, %glsl_shader_path%\*.comp) DO (
    .\extern\vulkan\Bin\glslc.exe %%f -o %asset_path%\shaders\%%~xnf.spv
    ECHO   "%%~xnf" is compiled ...
)

ECHO [GLSLC] SPIRV compilation completes!

PAUSE