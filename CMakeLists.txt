# NPS (Neutral Particle Spectrometer) reconstruction code for Hall C/hcana

cmake_minimum_required(VERSION 3.5)

project(NPSlib LANGUAGES CXX)

#----------------------------------------------------------------------------
# Find Podd, if necessary, and load its CMake module
if(NOT TARGET hcana::HallC)
  if(DEFINED ENV{HCANA})
    set(CMAKE_PREFIX_PATH "$ENV{HCANA}" "${CMAKE_PREFIX_PATH}")
  endif()

  find_package(hcana 0.95 REQUIRED)
endif()
include(PoddCMakeEnv)

set_diagnostic_flags(WALL WEXTRA)
#report_build_info()

#----------------------------------------------------------------------------
add_subdirectory(src)
