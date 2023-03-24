#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# TODO: Check version
cmake_minimum_required(VERSION 3.7.2)

project(manager C)

set(configure_string "")

# App-specific settings

config_option(
	LibSel4CacheColouring
	LIB_SEL4_CACHECOLOURING
	"Enable cache colouring"
    DEFAULT OFF
)

config_string(
	BenchUntypeCount
	BENCH_UNTYPE_COUNT
	"Number of untypeds to allocate to each benchmark thread"
	DEFAULT
	64
	UNQUOTE
)

config_string(
	BenchUntypeSizebits
	BENCH_UNTYPE_SIZEBITS
	"Size of each untyped to pass to the benchmark thread"
	DEFAULT
	12
	UNQUOTE
)

config_option(
	ManagerMitigation
	MANAGER_MITIGATION
	"Configure kernel objects to mitigate timing channels"
	DEFAULT
	OFF
)

config_option(
	ManagerColourOnly
	MANAGER_COLOUR_ONLY
	"Configure user-level with cache-colored objects"
	DEFAULT
	OFF
)

config_option(
	ManagerUncolourKernel
	MANAGER_UNCOLOUR_KERNEL
	"Configure kernel image with cache-colored objects"
	DEFAULT
	OFF
)

config_option(
	ManagerIPC
	MANAGER_IPC
	"Monitor IPC performance from the runner"
	DEFAULT
	OFF
)

config_option(
	ChannelBenchMonitorSameColor
	MANAGER_IPC_SAME_COLOUR
	"Test IPC with caller and receiver in same cache color"
	DEFAULT
	OFF
	DEPENDS "ManagerMitigation"
)

config_option(
	ManagerCacheDivUneven
	MANAGER_CACHE_DIV_UNEVEN
	"Provide unequal (n - 1) cache colors to the testing thread"
	DEFAULT
	OFF
)

config_option(
	ManagerCacheDivLLC
	MANAGER_CACHE_DIV_LLC
	"Divide cache colours based on LLC"
	DEFAULT
	OFF
)

config_option(
	ManagerPMUCounter
	MANAGER_PMU_COUNTER
	"Monitor performance via the PMU counters from the runner"
	DEFAULT
	OFF
)

config_option(
	ManagerCacheFlush
	MANAGER_CACHE_FLUSH
	"Enable cache flushing"
	DEFAULT
	OFF
)

config_option(
	ManagerFuncTests
	MANAGER_FUNC_TESTS
	"Enable functional correctness testing"
	DEFAULT
	OFF
)

config_option(
	ManagerCovertBench
	MANAGER_COVERT_BENCH
	"Enable covert channel benchmarks"
	DEFAULT
	OFF
)

config_option(
	ManagerSplashBench
	MANAGER_SPLASH_BENCH
	"Enable splash-2 benchmarks"
	DEFAULT
	OFF
)

config_option(
	ManagerSplashBenchSwitch
	MANAGER_SPLASH_BENCH_SWITCH
	"Enable splash-2 benchmarks with context switch"
	DEFAULT
	OFF
	DEPENDS "ManagerSplashBench"
)

config_option(
	ManagerDCacheSwitch
	MANAGER_DCACHE_ATTACK
	"L1 d-cache attack"
	DEFAULT
	OFF
)

config_option(
	Manager
	MANAGER_HUGE_PAGES
	"Manager configures huge pages for the benchmarking thread heap"
	DEFAULT
	OFF
)

add_config_library(manager "${configure_string}")

find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)
find_package(sel4_projects_libs REQUIRED)

# Need to disable GC sections as it causes our tests to be stripped sometimes
# This option is tested in the following musllibc_setup_build_environment_with_sel4runtime call.
set(UserLinkerGCSections OFF CACHE BOOL "" FORCE)
# This sets up environment build flags and imports musllibc and runtime libraries.
musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()
util_libs_import_libraries()
sel4_libs_import_libraries()
sel4benchsupport_import_libraries()

# Setting LibNanopb On causes its libraries to be created when sel4_projects_libs_import_libraries
# is called.
set(LibNanopb ON CACHE BOOL "" FORCE)
sel4_projects_libs_import_libraries()

# TODO: Fix file selections
file(
    GLOB
		static
        src/*.c
)

# Import build rules for test app
add_subdirectory(../side-bench side-bench)
include(cpio)
MakeCPIO(archive.o "$<TARGET_FILE:side-bench>")

add_executable(manager EXCLUDE_FROM_ALL ${static} archive.o)
target_include_directories(manager PRIVATE "include" "../../include")
target_link_libraries(
    manager
    PUBLIC
        sel4_autoconf
 		sel4
		sel4runtime
		muslc
		sel4vka
		cpio
		elf
        sel4allocman
        platsupport
		sel4platsupport
		sel4muslcsys
        sel4simple
		sel4vspace
		sel4utils
		utils
        sel4cachecolour
		sel4bench
    PRIVATE
		manager_Config
		side-bench_Config
)
target_compile_options(manager PRIVATE -Werror -g)

if(KernelArchX86_64)
	target_link_options(side-bench PRIVATE -mno-sse)
endif()

# Set this image as the rootserver
include(rootserver)
DeclareRootserver(manager)
