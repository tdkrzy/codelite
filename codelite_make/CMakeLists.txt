# define minimum cmake version
cmake_minimum_required(VERSION 2.6.2)
 
# Our project is called 'codelite' this is how it will be called in
# visual studio, and in our makefiles. 
project(codelite-make)

# It was noticed that when using MinGW gcc it is essential that 'core' is mentioned before 'base'.
find_package(wxWidgets COMPONENTS core base REQUIRED)

# wxWidgets include (this will do all the magic to configure everything)
#include( "${wxWidgets_USE_FILE}" )

# Include paths
#include_directories("${CL_SRC_ROOT}/Plugin" "${CL_SRC_ROOT}/sdk/wxsqlite3/include" "${CL_SRC_ROOT}/CodeLite" "${CL_SRC_ROOT}/PCH" "${CL_SRC_ROOT}/Interfaces")

# Add RPATH
set (LINKER_OPTIONS -Wl,-rpath,"${PLUGINS_DIR}")

# we need wxWidgets flags to be set only for the c++ files, so we do it like this
# by setting the CMAKE_CXX_FLAGS
execute_process(COMMAND ${CL_WX_CONFIG} --cxxflags OUTPUT_VARIABLE WX_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
set (CMAKE_CXX_FLAGS ${WX_CXX_FLAGS})

# prepare list of files
FILE(GLOB SRCS "*.cpp")

# and finally prepare list of includes directories
include_directories("${CL_SRC_ROOT}/Plugin" "${CL_SRC_ROOT}/sdk/wxsqlite3/include" "${CL_SRC_ROOT}/CodeLite" "${CL_SRC_ROOT}/PCH" "${CL_SRC_ROOT}/Interfaces")

# Define the output
add_executable(codelite-make ${SRCS})
set ( UTIL_LIB "" )
if ( UNIX )
    set( UTIL_LIB "-lutil")
endif ( UNIX )

target_link_libraries(codelite-make ${LINKER_OPTIONS} ${wxWidgets_LIBRARIES} -L"${CL_LIBPATH}" -lwxsqlite3 -lsqlite3lib -llibcodelite -lplugin ${UTIL_LIB} )

install(TARGETS codelite-make DESTINATION ${CL_PREFIX}/bin)
add_dependencies(codelite-make plugin)
