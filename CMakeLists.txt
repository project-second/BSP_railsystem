cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 설정
project(VmsEdgeNode)

# 모던 C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 헤더 파일들이 모여있는 폴더 지정
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 나중에 사용할 멀티스레딩 및 SQLite 라이브러리 찾기
find_package(Threads REQUIRED)
# 도커/우분투 환경을 위한 SQLite3 링크 설정
find_library(SQLITE3_LIBRARY sqlite3)

# 컴파일할 소스 파일들 목록
set(SOURCES
    src/main.cpp
    src/hardware/MotorController.cpp
    src/database/PresetManager.cpp
)

# 최종 실행 파일(VmsEdgeApp) 생성
add_executable(VmsEdgeApp ${SOURCES})

# 필요한 라이브러리 연결 (스레드, SQLite3)
target_link_libraries(VmsEdgeApp PRIVATE Threads::Threads ${SQLITE3_LIBRARY})