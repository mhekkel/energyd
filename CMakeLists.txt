cmake_minimum_required(VERSION 3.16)

# set the project name
project(energyd VERSION 2.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# Create a revision file, containing the current git version info, if any
include(VersionString)
write_version_header(${PROJECT_SOURCE_DIR}/src)

# use mrc to create resources
find_package(Mrc QUIET)

if(NOT MRC_FOUND)
	message(FATAL_ERROR "Cannot use resources since mrc was not found")
endif()

add_compile_definitions(WEBAPP_USES_RESOURCES)
mrc_write_header(${PROJECT_SOURCE_DIR}/src/mrsrc.hpp)

# Libraries
find_package(date REQUIRED)
find_package(zeep 6 REQUIRED)
find_package(libmcfp REQUIRED)
find_package(libpqxx 7.8 REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(energyd
	${PROJECT_SOURCE_DIR}/src/energyd.cpp
	${PROJECT_SOURCE_DIR}/src/data-service.cpp
	${PROJECT_SOURCE_DIR}/src/https-client.cpp
	${PROJECT_SOURCE_DIR}/src/sessy-service.cpp
	${PROJECT_SOURCE_DIR}/src/p1-service.cpp)

target_link_libraries(energyd date::date libpqxx::pqxx libmcfp::libmcfp zeep::zeep
	OpenSSL::SSL OpenSSL::Crypto)

# yarn rules for javascripts

find_program(YARN yarn REQUIRED)

set(webpack_input
	${CMAKE_CURRENT_SOURCE_DIR}/webapp/grafiek.js
	${CMAKE_CURRENT_SOURCE_DIR}/webapp/index.js
	${CMAKE_CURRENT_SOURCE_DIR}/webapp/invoer.js
	${CMAKE_CURRENT_SOURCE_DIR}/webapp/opname.js
	${CMAKE_CURRENT_SOURCE_DIR}/webapp/style.scss
	${CMAKE_CURRENT_SOURCE_DIR}/webpack.config.js
)

set(webpack_output ${CMAKE_CURRENT_SOURCE_DIR}/docroot/scripts/index.js)

add_custom_command(
	OUTPUT ${webpack_output}
	DEPENDS ${webpack_input}
	COMMAND ${YARN} run $<IF:$<CONFIG:Debug>,build,build-production>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(webpack DEPENDS ${webpack_output})

add_dependencies(energyd webpack)

mrc_target_resources(energyd ${PROJECT_SOURCE_DIR}/docroot/)

install(TARGETS energyd DESTINATION ${CMAKE_INSTALL_SBINDIR})

# enable_testing()

# add_test(NAME energyd
# 	COMMAND $<TARGET_FILE:energyd>
# 	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
