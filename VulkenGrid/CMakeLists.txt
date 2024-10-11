cmake_minimum_required(VERSION 3.10)
project(VulkanGridEngine)

# Use C++17
set(CMAKE_CXX_STANDARD 17)

# Specify source files
set(SOURCES
    main.cpp
    Engine/VulkanInstance.cpp
    Engine/VulkanDevice.cpp
    Engine/VulkanSwapChain.cpp
    Engine/VulkanBuffer.cpp
    Engine/VulkanCommandBuffer.cpp
    Logger/Logger.cpp
    Logger/SystemInfo.cpp
    Render/Pipeline.cpp
    Render/ShaderModule.cpp
    Render/RenderPass.cpp
    Utils/FilesUtils.cpp
    Utils/VulkanUtils.cpp
    Utils/LoggerUtils.cpp
)

# Check if we're compiling on Linux
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
    message("Compiling for Linux...")
else()
    message("Compiling for Windows...")
endif()

if (NOT LINUX)
    # Set the path to your Vulkan SDK installation
    set(VULKAN_SDK "C:/VulkanSDK/1.3.290.0")

    # Add Vulkan include and library directories
    include_directories("${VULKAN_SDK}/Include")
    link_directories("${VULKAN_SDK}/Lib")

    # Set the path to your GLFW installation
    set(GLFW_DIR "C:/Libraries/GLFW")

    # Add GLFW include directory
    include_directories("${GLFW_DIR}/include")

    # Add GLFW library directory based on your compiler version (e.g., Visual Studio 2019)
    link_directories("${GLFW_DIR}/lib-vc2019")
else()
    # Find the Vulkan SDK package
    find_package(Vulkan REQUIRED)
endif()

# Include directories for source files
include_directories(Engine Logger Render)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

if (NOT LINUX)
    # Link GLFW and Vulkan libraries
    target_link_libraries(${PROJECT_NAME} glfw3 "${VULKAN_SDK}/Lib/vulkan-1.lib")
else()
    # Link libraries
    target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)
endif()

# Define the shader compilation command
function(compile_shader shader_file)
    get_filename_component(shader_name ${shader_file} NAME)
    # Use $<CONFIG> instead of CMAKE_BUILD_TYPE for multi-configuration generators
    set(output_file ${CMAKE_BINARY_DIR}/$<CONFIG>/shaders/${shader_name}.spv)

    # Add logging to verify the shader compilation step
    message(STATUS "Compiling shader: ${shader_file}")
    message(STATUS "Shader output will be: ${output_file}")

    add_custom_command(
        OUTPUT ${output_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>/shaders
        COMMAND glslc ${CMAKE_SOURCE_DIR}/${shader_file} -o ${output_file}
        DEPENDS ${CMAKE_SOURCE_DIR}/${shader_file}
        COMMENT "Compiling shader ${shader_file}"
        VERBATIM
    )
endfunction()

# Compile the vertex and fragment shaders
compile_shader("Render/Shaders/triangle.vert")
compile_shader("Render/Shaders/triangle.frag")

# Create a custom target to ensure shaders are compiled
add_custom_target(
    CompileShaders ALL
    DEPENDS ${CMAKE_BINARY_DIR}/$<CONFIG>/shaders/triangle.vert.spv
            ${CMAKE_BINARY_DIR}/$<CONFIG>/shaders/triangle.frag.spv
    COMMENT "Compiling all shaders"
)

# Ensure shaders are compiled before building the executable
add_dependencies(${PROJECT_NAME} CompileShaders)
