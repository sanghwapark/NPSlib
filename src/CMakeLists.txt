# CMakeLists.txt for NPSlib sources

cmake_minimum_required(VERSION 3.5)

#----------------------------------------------------------------------------
# Names of source files and target libraries

# The generated shared library will be named lib$(PACKAGE).so/dylib
set(PACKAGE NPS)

# Sources
set(src
  THcNPSAnalyzer.cxx
  THcNPSApparatus.cxx
  THcNPSArray.cxx
  THcNPSCalorimeter.cxx
  THcNPSShowerHit.cxx
  THcNPSCluster.cxx
  THcNPSSecondaryKine.cxx
  THcNPSCoinTime.cxx
  VTPModule.cxx
  VLDModule.cxx
  )

# Headers
string(REPLACE .cxx .h headers "${src}")

# If you have any extra headers needed for building the ROOT dictionary,
# add them here. Otherwise leave this line commented out.
#list(APPEND headers ExtraHeaders.h)

# If you have other headers NOT needed for the ROOT dictionary but needed for
# the library, add them here as shown in the line commented out below.
set(allheaders ${headers})
#set(allheaders ${headers} StandaloneHeader1.h StandaloneHeader2.h)

#------------------------------------------------------------------------------
# Do not change anything below here unless you know what you are doing

#----------------------------------------------------------------------------
# Library
add_library(${PACKAGE} SHARED ${src} ${allheaders} ${PACKAGE}Dict.cxx)

target_compile_options(${PACKAGE} PRIVATE ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST})

target_include_directories(${PACKAGE}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )
target_link_libraries(${PACKAGE} PUBLIC hcana::HallC)

include(GNUInstallDirs)

install(TARGETS ${PACKAGE}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(FILES ${allheaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# ROOT dictionary
build_root_dictionary(${PACKAGE} ${headers}
  TARGETS ${PACKAGE}
  LINKDEF ${PACKAGE}_LinkDef.h
  )
