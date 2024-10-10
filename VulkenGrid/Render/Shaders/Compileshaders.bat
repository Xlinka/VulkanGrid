@echo off
set VULKAN_SDK=C:\VulkanSDK\1.3.290.0
set GLSLC="%VULKAN_SDK%\Bin\glslc.exe"

if not exist compiled_shaders (
    mkdir compiled_shaders
)

%GLSLC% triangle.vert -o compiled_shaders\vert.spv
%GLSLC% triangle.frag -o compiled_shaders\frag.spv

echo Shader compilation completed.
