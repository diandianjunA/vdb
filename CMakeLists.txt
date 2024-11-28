cmake_minimum_required(VERSION 3.2) #设置cmake的最低版本
project(vdb LANGUAGES CXX) #设置项目名称 vdb
 
# 设置C++标准  基本不用改
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED True)  

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

FIND_PACKAGE(OpenMP REQUIRED)
if(OPENMP_FOUND)
message("OPENMP FOUND")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# file自定义搜索源文件，塞给集合SOURCES 
file(GLOB SOURCES *.h *.cpp include/*.h index/*.cpp log/*.cpp)

include_directories("/usr/local/include/")
link_directories("./ThirdParty/OpenCV/lib")
 
# 生成可执行文件 首位是项目的名称  
add_executable(vdb vdb_server.cpp ${SOURCES})  
 
#补充添加链接库，如果需要添加链接库较多，最好这样写
target_link_libraries(vdb PRIVATE 
	# faiss openblas pthread rocksdb snappy z bz2 zstd lz4
    faiss openblas pthread rocksdb
)