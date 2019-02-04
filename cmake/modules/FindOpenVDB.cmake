
#
# This source file is part of appleseed.
# Visit https://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2019 Artem Bishev, The appleseedhq Organization
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#


# Find OpenVDB headers and libraries.
#
#  This module defines
#  OPENVDB_INCLUDE_DIRS - where to find OpenVDB includes.
#  OPENVDB_LIBRARIES    - List of libraries when using OpenVDB.
#  OPENVDB_FOUND        - True if OpenVDB was found.

# Look for the header file.
find_path (OPENVDB_INCLUDE_DIR NAMES openvdb/openvdb.h)

# Look for the library.
find_library (OPENVDB_LIBRARY NAMES libopenvdb)

# handle the QUIETLY and REQUIRED arguments and set OPENVDB_FOUND to TRUE if all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (OPENVDB DEFAULT_MSG OPENVDB_LIBRARY OPENVDB_INCLUDE_DIR)

# Copy the results to the output variables.
if (OPENVDB_FOUND)
    set (OPENVDB_LIBRARIES ${OPENVDB_LIBRARY})
    set (OPENVDB_INCLUDE_DIRS ${OPENVDB_INCLUDE_DIR})
else ()
    set (OPENVDB_LIBRARIES)
    set (OPENVDB_INCLUDE_DIRS)
endif ()

mark_as_advanced (OPENVDB_INCLUDE_DIR OPENVDB_LIBRARY)
