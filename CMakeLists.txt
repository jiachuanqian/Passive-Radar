cmake_minimum_required(VERSION 3.15) 
project(PassiveRadar LANGUAGES CXX CUDA) 

set(CMAKE_INCLUDE_CURRENT_DIR ON) 
set(CMAKE_PREFIX_PATH "/home/usrp/Qt5.14.2/5.14.2/gcc_64") # Qt Kit Dir
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CUDA_ARCHITECTURES 75)
find_package(CUDAToolkit REQUIRED)
find_package(Qt5 COMPONENTS Widgets Core Gui PrintSupport REQUIRED) # Qt COMPONENTS
#find_package(FFTW3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3 REQUIRED fftw3)

# 手动添加头文件路径和库路径
#include_directories(${FFTW3_INCLUDE_DIRS})
#link_directories(${FFTW3_LIBRARY_DIRS})

#include_directories("D:/Program Files (x64)/vcpkg/installed/x64-windows/include")
include_directories("${CUDAToolkit_INCLUDE_DIRS}")
include_directories("/home/usrp/Desktop/New Folder/Passive Radar/src")


set(SRCS
    src/main.cpp
    src/mymap.cpp
    src/PassiveRadar.cpp
    src/ProcThread.cpp
    src/qcustomplot.cpp
    src/typedefs.cpp
    src/CudaApi.cu
) 


# 1. 添加qrc资源文件
set(QRC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/PassiveRadar.qrc")

# 2. 添加rc资源文件（Windows）
set(RC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/PassiveRadar.rc")


# 添加可执行文件
add_executable(${PROJECT_NAME} 
    #WIN32 # If you need a terminal for debug, please comment this statement 
    ${SRCS} 
    ${QRC_FILE}  # 添加qrc文件
    ${RC_FILE}   # 添加rc文件
) 



target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt5::Widgets 
    Qt5::Core
    Qt5::Gui
    Qt5::PrintSupport
    ${FFTW3_LIBRARIES}
    CUDA::cudart
    CUDA::cufft
    CUDA::cublas
    CUDA::cusolver 
)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying res folder to binary directory"
)