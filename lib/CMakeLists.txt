# Copyright 2011,2012 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND fsk4_sources
    demod_ff_impl.cc
    generic_f_impl.cc
    apco25_f_impl.cc
    rdlap_f_impl.cc
)

set(fsk4_sources "${fsk4_sources}" PARENT_SCOPE)
if(NOT fsk4_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT fsk4_sources)

add_library(gnuradio-fsk4 SHARED ${fsk4_sources})
target_link_libraries(gnuradio-fsk4 ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES})
set_target_properties(gnuradio-fsk4 PROPERTIES DEFINE_SYMBOL "gnuradio_fsk4_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-fsk4 PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-fsk4
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_fsk4_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_fsk4.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_fsk4.cc
)

add_executable(test-fsk4 ${test_fsk4_sources})

target_link_libraries(
  test-fsk4
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-fsk4
)

GR_ADD_TEST(test_fsk4 test-fsk4)
