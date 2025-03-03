cmake_minimum_required(VERSION 3.2) #设置cmake的最低版本
project(vdb LANGUAGES CXX) #设置项目名称 vdb
 
# 设置C++标准  基本不用改
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED True)  

set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl") 
# set (LIBCUVS, "/home/xjs/anaconda3/pkgs/libcuvs-24.12.00-cuda12_241211_g0ce6a03_0/lib")

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
file(GLOB VDB_SERVER_SOURCES vdb_server/*.cpp include/*.h index/*.cpp log/*.cpp raft/*.cpp)
file(GLOB MASTER_SERVER_SOURCES master_server/*.cpp log/*.cpp)
file(GLOB PROXY_SERVER_SOURCES proxy_server/*.cpp log/*.cpp)

include_directories("/usr/local/include/")
include_directories("/usr/local/cuda-12.5/targets/x86_64-linux/include")
include_directories("/usr/local/cuda-12.2/targets/x86_64-linux/include")
link_directories("./ThirdParty/OpenCV/lib")
link_directories("/usr/lib/x86_64-linux-gnu")
link_directories("/usr/local/lib")
link_directories("~/anaconda3/lib")
link_directories("./lib")
include_directories("/usr/local/cuda-12.5/lib64")

add_executable(vdb_server vdb_server.cpp ${VDB_SERVER_SOURCES})
add_executable(master_server master_server.cpp ${MASTER_SERVER_SOURCES})
add_executable(proxy_server proxy_server.cpp ${PROXY_SERVER_SOURCES})

target_link_libraries(vdb_server PRIVATE 
    # faiss openblas pthread rocksdb snappy z bz2 zstd lz4 nuraft ssl
    # faiss faiss_gpu gpu cuvs cudart cublas cuda openblas pthread rocksdb snappy z bz2 zstd lz4 nuraft ssl
    faiss faiss_gpu gpu cuvs cublas cudart cuda curand openblas pthread rocksdb snappy z bz2 zstd lz4 nuraft ssl
)
target_link_libraries(master_server PRIVATE
    etcd-cpp-api cpprest
)
target_link_libraries(proxy_server PRIVATE
    curl pthread
)