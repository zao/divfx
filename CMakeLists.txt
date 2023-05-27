cmake_minimum_required(VERSION 3.25)
project(divfx)

find_package(directxtex CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_executable(divfx
    src/Cards.cpp
    src/Cards.hpp
    src/D3D.cpp
    src/D3D.hpp
    src/DivFxMain.cpp
    src/Util.cpp
    src/Util.hpp
)

target_link_libraries(divfx PRIVATE
	"d3d11.lib"
	"d3dcompiler.lib"
	"dxgi.lib"
    "shcore.lib"
    Microsoft::DirectXTex
    Microsoft::DirectXTK
	fmt::fmt-header-only
	glfw
    glm::glm
)