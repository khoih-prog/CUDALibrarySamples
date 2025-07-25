#
# Copyright 2023-2025 NVIDIA Corporation.  All rights reserved.
#
# NOTICE TO LICENSEE:
#
# This source code and/or documentation ("Licensed Deliverables") are
# subject to NVIDIA intellectual property rights under U.S. and
# international Copyright laws.
#
# These Licensed Deliverables contained herein is PROPRIETARY and
# CONFIDENTIAL to NVIDIA and is being provided under the terms and
# conditions of a form of NVIDIA software license agreement by and
# between NVIDIA and Licensee ("License Agreement") or electronically
# accepted by Licensee.  Notwithstanding any terms or conditions to
# the contrary in the License Agreement, reproduction or disclosure
# of the Licensed Deliverables to any third party without the express
# written consent of NVIDIA is prohibited.
#
# NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
# LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
# SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
# PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
# NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
# DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
# NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
# NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
# LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
# SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
# DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THESE LICENSED DELIVERABLES.
#
# U.S. Government End Users.  These Licensed Deliverables are a
# "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
# 1995), consisting of "commercial computer software" and "commercial
# computer software documentation" as such terms are used in 48
# C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
# only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
# 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
# U.S. Government End Users acquire the Licensed Deliverables with
# only those rights set forth herein.
#
# Any use of the Licensed Deliverables in individual and commercial
# software must include, in the user documentation and internal
# comments to the code, the above Disclaimer and U.S. Government End
# Users Notice.
#
cmake_minimum_required(VERSION 3.19)

set(EXAMPLE_NAME simple_mgmn_mode)

project("cuDSS_${EXAMPLE_NAME}_example"
        DESCRIPTION  "cuDSS"
        HOMEPAGE_URL "https://docs.nvidia.com/cuda/cudss/index.html"
        LANGUAGES    CXX CUDA)

set(CMAKE_CUDA_STANDARD          11)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS        OFF)

option(BUILD_STATIC "Building statically linked examples" OFF)

# MGMN-related CMake options
set(MGMN_NPROC 1 CACHE STRING  "Number of MPI processes the MGMN mode examples")
set(MPIRUN_EXECUTABLE "mpirun" CACHE STRING "MPI executable (mpirun) for launching example binaries")
set(MPIRUN_NUMPROC_FLAG "-np" CACHE STRING "MPI flag for setting the number of processes")
set(MPIRUN_EXTRA_FLAGS "--allow-run-as-root" CACHE STRING "Extra flags to be passed to the MPI executable")
option(BUILD_MGMN_WITH_OPENMPI "Enable OpenMPI backend for the MGMN mode examples" ON)
option(BUILD_MGMN_WITH_NCCL    "Enable NCCL backend for the MGMN mode examples" ON)

set(OPENMPI_PATH "$ENV{OPENMPI_PATH}" CACHE STRING "Path to the root OpenMPI directory")
set(NCCL_PATH "$ENV{NCCL_PATH}" CACHE STRING "Path to the root NCCL directory")

set(OPENMPI_INCLUDE_DIRECTORIES "${OPENMPI_PATH}/include")
set(OPENMPI_LINK_DIRECTORIES "${OPENMPI_PATH}/lib")
set(NCCL_INCLUDE_DIRECTORIES "${NCCL_PATH}/include")
set(NCCL_LINK_DIRECTORIES "${NCCL_PATH}/lib")

# Find cuDSS
find_package(cudss 0.6.0 REQUIRED)


if(BUILD_MGMN_WITH_OPENMPI AND
        ((NOT OPENMPI_PATH) OR NOT IS_DIRECTORY ${OPENMPI_PATH}))
    message(FATAL_ERROR "For building MGMN examples with OpenMPI, path to the \
OpenMPI root directory must be set in OPENMPI_PATH" )
endif()

if(BUILD_MGMN_WITH_NCCL AND
        ((NOT NCCL_PATH) OR NOT IS_DIRECTORY ${NCCL_PATH}))
    message(FATAL_ERROR "For building MGMN examples with NCCL, path to the \
NCCL root directory must be set in \
NCCL_PATH" )
endif()

# (optional: only if samples are run with ctest)
enable_testing()

# Helper to make crun launch examples with mpirun (optional: only if samples are run with ctest)
function(test_mpi_launcher target test Nproc)

    if(NOT (DEFINED MPIRUN_EXECUTABLE AND DEFINED MPIRUN_NUMPROC_FLAG))
        message(FATAL_ERROR "MPIRUN_EXECUTABLE and MPIRUN_NUMPROC_FLAG must be defined to use test_mpi_launcher")
    endif()

    if(NOT Nproc)
        message(FATAL_ERROR "Nproc must be defined to use test_mpi_launcher() function in cmake")
    endif()

    # This conversion is a workaround for the case when cmake passes multiple arguments to mpirin with extra ""
    string(REPLACE " " ";" MPIRUN_EXTRA_FLAGS_LIST ${MPIRUN_EXTRA_FLAGS})

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
        set_property(TARGET ${target} PROPERTY TEST_LAUNCHER ${MPIRUN_EXECUTABLE} ${MPIRUN_EXTRA_FLAGS_LIST} ${MPIRUN_NUMPROC_FLAG} ${Nproc})
    else()
        set_property(TARGET ${target} PROPERTY CROSSCOMPILING_EMULATOR ${MPIRUN_EXECUTABLE} ${MPIRUN_EXTRA_FLAGS_LIST} ${MPIRUN_NUMPROC_FLAG} ${Nproc})
    endif()

    set_property(TEST ${test} PROPERTY PROCESSORS ${Nproc})

endfunction()

set_source_files_properties(${EXAMPLE_NAME}.cpp PROPERTIES LANGUAGE CUDA)

# Determine which communication backend options are passed
# (can be openmpi, nccl or both based on the CMake flags)
set(CUDSS_MGMN_COMM_BACKENDS "")
if (BUILD_MGMN_WITH_OPENMPI)
    set(CUDSS_MGMN_COMM_BACKENDS ${CUDSS_MGMN_COMM_BACKENDS} "openmpi")
endif()
if (BUILD_MGMN_WITH_NCCL)
    set(CUDSS_MGMN_COMM_BACKENDS ${CUDSS_MGMN_COMM_BACKENDS} "nccl")
endif()
message(STATUS "CUDSS_MGMN_COMM_BACKENDS = ${CUDSS_MGMN_COMM_BACKENDS}")

if (BUILD_MGMN_WITH_OPENMPI OR BUILD_MGMN_WITH_NCCL)
    set(CUDSS_USE_MPI 1)
else()
    set(CUDSS_USE_MPI 0)
endif()

####################################

foreach(backend ${CUDSS_MGMN_COMM_BACKENDS})

    add_executable(${EXAMPLE_NAME}_example_${backend} ${src})

    target_sources(${EXAMPLE_NAME}_example_${backend}
        PUBLIC ${PROJECT_SOURCE_DIR}/${EXAMPLE_NAME}.cpp
    )

    if (WIN32)
        target_include_directories(${EXAMPLE_NAME}_example_${backend} PUBLIC ${cudss_INCLUDE_DIR})
        target_link_directories(${EXAMPLE_NAME}_example_${backend} PUBLIC ${cudss_LIBRARY_DIR})
    endif()

    target_compile_options(${EXAMPLE_NAME}_example_${backend} PRIVATE
        "$<$<CONFIG:Debug>:-lineinfo -g>"
        "$<$<CONFIG:RelWithDebInfo>:-lineinfo -g>"
    )

    target_link_libraries(${EXAMPLE_NAME}_example_${backend} PUBLIC
        cudss
    )

    if (CUDSS_USE_MPI)
        target_compile_options(${EXAMPLE_NAME}_example_${backend} PRIVATE -DUSE_MPI)
    endif()

    if ("${backend}" STREQUAL "openmpi")
        target_compile_options(${EXAMPLE_NAME}_example_${backend} PRIVATE -DUSE_OPENMPI)
    elseif ("${backend}" STREQUAL "nccl")
        target_compile_options(${EXAMPLE_NAME}_example_${backend} PRIVATE -DUSE_NCCL)
    endif()

    # Currently, we only support OpenMPI here for MPI but this might change
    if ("${backend}" STREQUAL "openmpi" OR CUDSS_USE_MPI)
        target_include_directories(${EXAMPLE_NAME}_example_${backend} PUBLIC
                                   ${OPENMPI_INCLUDE_DIRECTORIES}
        )
    endif()
    if ("${backend}" STREQUAL "nccl")
        target_include_directories(${EXAMPLE_NAME}_example_${backend} PUBLIC
                                   ${NCCL_INCLUDE_DIRECTORIES}
        )
    endif()

    # Note: for NCCL we still need the MPI header (for mpi.h) and library (for MPI_Init)
    if ("${backend}" STREQUAL "openmpi" OR CUDSS_USE_MPI)
        target_link_directories(${EXAMPLE_NAME}_example_${backend} PUBLIC
                                ${OPENMPI_LINK_DIRECTORIES}
        )
        target_link_libraries(${EXAMPLE_NAME}_example_${backend} PUBLIC mpi)
    endif()
    if ("${backend}" STREQUAL "nccl")
        target_link_directories(${EXAMPLE_NAME}_example_${backend} PUBLIC
                                ${NCCL_LINK_DIRECTORIES}
        )
        target_link_libraries(${EXAMPLE_NAME}_example_${backend} PUBLIC nccl)
    endif()

    # (optional: only if samples are run with ctest)
    add_test(NAME ${EXAMPLE_NAME}_example_${backend} COMMAND ${EXAMPLE_NAME}_example_${backend} ${backend} ${cudss_LIBRARY_DIR}/libcudss_commlayer_${backend}.so)
    test_mpi_launcher(${EXAMPLE_NAME}_example_${backend} ${EXAMPLE_NAME}_example_${backend} ${MGMN_NPROC})

    if (BUILD_STATIC)
        add_executable(${EXAMPLE_NAME}_example_${backend}_static ${src})

        target_sources(${EXAMPLE_NAME}_example_${backend}_static
            PUBLIC ${PROJECT_SOURCE_DIR}/${EXAMPLE_NAME}.cpp
        )

        if (WIN32)
            target_include_directories(${EXAMPLE_NAME}_example_${backend}_static PUBLIC ${cudss_INCLUDE_DIR})
            target_link_directories(${EXAMPLE_NAME}_example_${backend}_static PUBLIC ${cudss_LIBRARY_DIR})
            target_link_libraries(${EXAMPLE_NAME}_example_${backend}_static PUBLIC
                cudss_static
            )
        else()
            target_link_libraries(${EXAMPLE_NAME}_example_${backend}_static PUBLIC
                cudss
            )
        endif()

        target_compile_options(${EXAMPLE_NAME}_example_${backend}_static PRIVATE
            "$<$<CONFIG:Debug>:-lineinfo -g>"
            "$<$<CONFIG:RelWithDebInfo>:-lineinfo -g>"
        )

        if (CUDSS_USE_MPI)
            target_compile_options(${EXAMPLE_NAME}_example_${backend}_static PRIVATE -DUSE_MPI)
        endif()

        # Note: OpenMPI here has priority over NCCL ( we have to
        if ("${backend}" STREQUAL "openmpi")
            target_compile_options(${EXAMPLE_NAME}_example_${backend}_static PRIVATE -DUSE_OPENMPI)
        elseif ("${backend}" STREQUAL "nccl")
            target_compile_options(${EXAMPLE_NAME}_example_${backend}_static PRIVATE -DUSE_NCCL)
        endif()

        # Currently, we only support OpenMPI here for MPI but this might change
        if ("${backend}" STREQUAL "openmpi" OR CUDSS_USE_MPI)
            target_include_directories(${EXAMPLE_NAME}_example_${backend}_static PUBLIC
                                       ${OPENMPI_INCLUDE_DIRECTORIES}
            )
        endif()
        if ("${backend}" STREQUAL "nccl")
            target_include_directories(${EXAMPLE_NAME}_example_${backend}_static PUBLIC
                                       ${NCCL_INCLUDE_DIRECTORIES}
            )
        endif()

        # Note: for NCCL we still need the MPI header (for mpi.h) and library (for MPI_Init)
        if ("${backend}" STREQUAL "openmpi" OR CUDSS_USE_MPI)
            target_link_directories(${EXAMPLE_NAME}_example_${backend}_static PUBLIC
                                    ${OPENMPI_LINK_DIRECTORIES}
            )
            target_link_libraries(${EXAMPLE_NAME}_example_${backend}_static PUBLIC mpi)
        endif()
        if ("${backend}" STREQUAL "nccl")
            target_link_directories(${EXAMPLE_NAME}_example_${backend}_static PUBLIC
                                    ${NCCL_LINK_DIRECTORIES}
            )
            target_link_libraries(${EXAMPLE_NAME}_example_${backend}_static PUBLIC nccl)
        endif()

        # (optional: only if samples are run with ctest)
        add_test(NAME ${EXAMPLE_NAME}_example_${backend}_static COMMAND ${EXAMPLE_NAME}_example_${backend}_static
            ${backend} ${cudss_LIBRARY_DIR}/libcudss_commlayer_${backend}.so)
        test_mpi_launcher(${EXAMPLE_NAME}_example_${backend}_static ${EXAMPLE_NAME}_example_${backend}_static ${MGMN_NPROC})

    endif()

endforeach()

