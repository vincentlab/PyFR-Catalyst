cmake_minimum_required(VERSION 2.8.8)
project(pyfr-catalyst)

# Always use a full RPATH
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
    "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endif()

find_package(ParaView 4.4 REQUIRED COMPONENTS vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore)
if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()
include("${PARAVIEW_USE_FILE}")
include(ParaViewPlugins)

set(CLIENT_OR_SERVER "Client" CACHE STRING "Build libraries for client or server-side functinality.")
set_property(CACHE CLIENT_OR_SERVER PROPERTY STRINGS "Client" "Server")

if(${CLIENT_OR_SERVER} STREQUAL Client)
  set (BuildClientLibs TRUE CACHE INTERNAL "Build client-side libraries")
  set (BuildServerLibs FALSE CACHE INTERNAL "Build server-side libraries")
endif()
if(${CLIENT_OR_SERVER} STREQUAL Server)
  set (BuildServerLibs TRUE CACHE INTERNAL "Build server-side libraries")
  set (BuildClientLibs FALSE CACHE INTERNAL "Build client-side libraries")
endif()

if (BuildServerLibs)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})

  find_package(CUDA REQUIRED)
  find_package(VTKm REQUIRED)
  include(VTKmMacros)
  vtkm_configure_device(Serial REQUIRED)
  vtkm_configure_device(Cuda REQUIRED)

  find_package(BoostHeaders ${VTKm_REQUIRED_BOOST_VERSION} REQUIRED)
  include_directories(${Boost_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS})

  set(fpList "float" CACHE INTERNAL "fpList")
  set(libSuffix_float fp32 CACHE INTERNAL "float suffix")
  set(libSuffix_double fp64 CACHE INTERNAL "double suffix")

  add_subdirectory(Data)
endif()

if (BuildClientLibs)
  add_subdirectory(Macros)
endif()

add_subdirectory(Source)
