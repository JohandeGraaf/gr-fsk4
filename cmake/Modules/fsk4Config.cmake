INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_FSK4 fsk4)

FIND_PATH(
    FSK4_INCLUDE_DIRS
    NAMES fsk4/api.h
    HINTS $ENV{FSK4_DIR}/include
        ${PC_FSK4_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    FSK4_LIBRARIES
    NAMES gnuradio-fsk4
    HINTS $ENV{FSK4_DIR}/lib
        ${PC_FSK4_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FSK4 DEFAULT_MSG FSK4_LIBRARIES FSK4_INCLUDE_DIRS)
MARK_AS_ADVANCED(FSK4_LIBRARIES FSK4_INCLUDE_DIRS)

