cmake_minimum_required(VERSION 3.28)
project(BattleShip)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BOOST_ROOT "D:/Libraries/Boost/Compiled")
set(BOOST_LIBRARYDIR "D:/Libraries/Boost/Compiled/lib")

set(CMAKE_PREFIX_PATH "D:/Libraries/Wt/installed")
include_directories("D:/Libraries/Wt/installed/include")

include_directories("D:/Programms/mingw64/include/nlohmann")

find_package(Boost REQUIRED COMPONENTS thread program_options filesystem)

find_package(Wt REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/applications/authorization/telegram_authorization.cpp
    src/data_bases/user_db/user_db.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Wt::Wt
        Wt::HTTP
        Wt::Dbo
        Wt::DboSqlite3
        Boost::thread
        Boost::program_options
        Boost::filesystem
)

add_executable(user_db src/data_bases/user_db/user_db.cpp)