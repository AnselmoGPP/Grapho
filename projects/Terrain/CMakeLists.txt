CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

if( UNIX )
	#set(CMAKE_CXX_COMPILER "/usr/bin/c++")
	#set(CMAKE_C_COMPILER "gcc-9")
	#set(CMAKE_CXX_COMPILER "/usr/bin/g++-9")
endif()

PROJECT(Terrain
	VERSION 1.0
	DESCRIPTION "Procedural terrain generation"
	LANGUAGES CXX
	)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_GENERATOR_PLATFORM x64)
FIND_PACKAGE(Vulkan REQUIRED)

MESSAGE(STATUS "Project: " ${PROJECT_NAME})

ADD_DEFINITIONS(
	-std=c++17
	-O2		# O2 optimization is incompatible with Debug mode
	#-O3
)
#ADD_COMPILE_DEFINITIONS( IMGUI_IMPL_OPENGL_LOADER_GLEW=1 )
#ADD_COMPILE_DEFINITIONS( IMGUI_IMPL_OPENGL_LOADER_GLAD=1 )

ADD_EXECUTABLE( ${PROJECT_NAME}
	src/main.cpp

	src/noise.cpp
	src/terrain.cpp
	src/common.cpp
	src/entities.cpp
	src/components.cpp
	src/systems.cpp

	include/noise.hpp
	include/terrain.hpp
	include/common.hpp
	include/entities.hpp
	include/components.hpp
	include/systems.hpp

	../../Readme.md
	TODO.txt
	CMakeLists.txt
)

TARGET_SOURCES(${PROJECT_NAME} PRIVATE
	#../../extern/imgui/imgui-1.72b/imgui.cpp
	#../../extern/imgui/imgui-1.72b/imgui_demo.cpp
	#../../extern/imgui/imgui-1.72b/imgui_draw.cpp
	#../../extern/imgui/imgui-1.72b/imgui_widgets.cpp
	#../../extern/imgui/imgui-1.72b/examples/imgui_impl_glfw.cpp
	#../../extern/imgui/imgui-1.72b/examples/imgui_impl_opengl3.cpp
)

TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME} PUBLIC
	include
	C:/VulkanSDK/1.3.275.0/Include
	../Renderer/include
	../../extern/glfw/glfw-3.3.2/include
	../../extern/glm/glm-0.9.9.5
	../../extern/stb
	../../extern/tinyobjloader
	../../extern/assimp/include
	../../extern/assimp/contrib
	../../_BUILD/extern/assimp/include/
	../../extern/FastNoiseLite
	../../extern/FastNoise2/include
	#../../extern/BulletPhysics/bullet3/src/
	#../../extern/vulkansdk-linux-x86_64-1.2.170.0/1.2.170.0/x86_64/include
	#../../extern/imgui/imgui-1.72b
	#../../extern/eigen-3.3.8
)

if( UNIX )
	#TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME} PUBLIC /usr/include/c++/9 )

	TARGET_LINK_LIBRARIES( ${PROJECT_NAME} 
		${PROJECT_SOURCE_DIR}/../../_BUILD/extern/glfw/glfw-3.3.2/src/libglfw3.a
		${PROJECT_SOURCE_DIR}/../../_BUILD/extern/glm/glm-0.9.9.5/glm/libglm_static.a
		C:/VulkanSDK/1.3.275.0/Lib/shaderc_combined.lib
		-lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi #-lglfw 
	)
elseif( WIN32 )
	TARGET_LINK_LIBRARIES( ${PROJECT_NAME} 
		#Vulkan::Vulkan
		optimized	C:/VulkanSDK/1.3.275.0/Lib/vulkan-1.lib
		optimized	C:/VulkanSDK/1.3.275.0/Lib/shaderc_combined.lib
		optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/projects/Renderer/Release/Vk_renderer.lib
		optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/glfw/glfw-3.3.2/src/Release/glfw3.lib
		optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/glm/glm-0.9.9.5/glm/Release/glm_static.lib
		
		optimized   ${PROJECT_SOURCE_DIR}/../../_BUILD/extern/assimp/lib/Release/assimp-vc143-mt.lib
		optimized   ${PROJECT_SOURCE_DIR}/../../_BUILD/extern/assimp/contrib/zlib/Release/zlibstatic.lib
		#optimized  ${PROJECT_SOURCE_DIR}/../../_BUILD/extern/assimp/bin/Release/assimp-vc143-mt.dll

		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/Bullet3Collision.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/Bullet3Dynamics.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/Bullet3Common.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletCollision.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletDynamics.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletSoftBody.lib

		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/Bullet2FileLoader.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/Bullet3Geometry.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/Bullet3OpenCL_clew.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletExampleBrowserLib.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletFileLoader.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletInverseDynamics.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletInverseDynamicsUtils.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletRobotics.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletRoboticsGUI.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletSoftBody.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletWorldImporter.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BulletXmlWorldImporter.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/BussIK.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/clsocket.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/ConvexDecomposition.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/GIMPACTUtils.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/gtest.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/gwen.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/HACD.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/LinearMath.lib
		#optimized	${PROJECT_SOURCE_DIR}/../../_BUILD/extern/lib/Release/OpenGLWindow.lib
	)
endif()











