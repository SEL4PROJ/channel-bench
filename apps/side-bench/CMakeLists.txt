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

cmake_minimum_required(VERSION 3.7.2)

project(side-bench C CXX)

config_string(
	BenchCacheLineSize
	BENCH_CACHE_LINE_SIZE
	"Cache line size (bytes)"
	DEFAULT
	64
	UNQUOTE
)

config_option(
	BenchCacheFlush
	BENCH_CACHE_FLUSH
	"Enable cache flush attack"
	DEFAULT
	OFF
)

config_string(
	BenchBTBEntries
	BENCH_BTB_ENTRIES
	"Number of entries in BTB"
	DEFAULT
	16
	UNQUOTE
)

config_string(
	BenchBTBAlign
	BENCH_BTB_ALIGN
	"Alignment of indirect branch instructions (bytes). This should guarantee iterating over all BTB indices."
	DEFAULT
	20
	UNQUOTE
)

config_string(
	BenchBHTEntries
	BENCH_BHT_ENTRIES
	"Number of entries in BHT"
	DEFAULT
	64
	UNQUOTE
)

config_string(
	BenchBHTAlign
	BENCH_BHT_ALIGN
	"Alignment of conditional branch instructions (bytes). This should guarantee iterating over all BHT indices."
	DEFAULT
	20
	UNQUOTE
)

config_choice(
	BenchCacheFlushChoice
	BENCH_CACHE_FLUSH_CHOICE
	"Cache flushing benchmark to run"
	"idle;BenchCacheFlushNothing;BENCH_CACHE_FLUSH_NOTHING;BenchCacheFlush"
	"l1d;BenchCacheFlushL1Caches;BENCH_CACHE_FLUSH_L1_CACHES;BenchCacheFlush"
	"l1i;BenchCacheFlushL1CachesInstruction;BENCH_CACHE_FLUSH_L1_CACHES_INSTRUCTION;BenchCacheFlush"
	"llc;BenchCacheFlushAttack;BENCH_CACHE_FLUSH_ATTACK;BenchCacheFlush"
	"all;BenchCacheFlushAllCaches;BENCH_CACHE_FLUSH_ALL_CACHES;BenchCacheFlush"
	"l2;BenchCacheFlushL2Caches;BENCH_CACHE_FLUSH_L2_CACHES;BenchCacheFlush;KernelArchX86"
	"l1-flush;BenchCacheFlushL1CachesOnly;BENCH_CACHE_FLUSH_L1_CACHES_ONLY;BenchCacheFlush"
	"all-flush;BenchCacheFlushAllCachesOnly;BENCH_CACHE_FLUSH_ALL_CACHES_ONLY;BenchCacheFlush"
)

config_option(
	BenchDCacheAttack
	BENCH_DCACHE_ATTACK
	"Simulate the L1 dcache attack (Tromer_OS_10)"
	DEFAULT
	OFF
)

config_option(
	BenchDCacheAttack
	BENCH_DCACHE_ATTACK
	"Simulate the L1 dcache attack (Tromer_OS_10)"
	DEFAULT
	OFF
)

config_option(
	BenchIPC
	BENCH_IPC
	"Enable IPC benchmarking"
	DEFAULT
	OFF
)

config_option(
	BenchPMUCounter
	BENCH_PMU_COUNTER
	"Use PMU counter to monitor events"
	DEFAULT
	OFF
)

config_option(
	BenchCovert
	BENCH_COVERT
	"Enable covert channel benchmark"
	DEFAULT
	OFF
)

config_choice(
	BenchCovertChoice
	BENCH_COVERT_CHOICE
	"Covert channel benchmark to run"
	"l1d;BenchCovertL1D;BENCH_COVERT_L1D;BenchCovert"
	"l1i;BenchCovertL1I;BENCH_COVERT_L1I;BenchCovert"
	"l2;BenchCovertL2;BENCH_COVERT_L2;KernelArchX86;BenchCovert"
	"llc;BenchCovertLLC;BENCH_COVERT_LLC;KernelArchArm;BenchCovert"
	"llc kernel schedule;BenchCovertLLCKernelSchedule;BENCH_COVERT_LLC_KERNEL_SCHEDULE;BenchCovert"
	"tlb;BenchCovertTLB;BENCH_COVERT_TLB;BenchCovert"
	"btb;BenchCovertBTB;BENCH_COVERT_BTB;BenchCovert"
	"bp;BenchCovertBP;BENCH_COVERT_BP;BenchCovert"
	"timer;BenchCovertTimer;BENCH_COVERT_TIMER;BenchCovert"
	"llc kernel;BenchCovertLLCKernel;BENCH_COVERT_LLC_KERNEL;BenchCovert"
)

config_string(
	BenchDataPoints
	BENCH_DATA_POINTS
	"Total number of random data points from high domain"
	DEFAULT
	409600
	DEPENDS "BenchCovert"
	UNQUOTE
)

config_option(
	BenchCovertL2KernelSchedule
	BENCH_COVERT_L2_KERNEL_SCHEDULE
	"Use the L2 cache for the KD channel"
	DEFAULT
	OFF
	DEPENDS "BenchCovertLLCKernelSchedule"
)

config_option(
	BenchCovertL1DWrite
	BENCH_COVERT_L1D_WRITE
	"L1D covert channel with writes"
	DEFAULT
	OFF
	DEPENDS "BenchCovertL1D"
)

config_option(
	BenchCovertL1IRewrite
	BENCH_COVERT_L1I_REWRITE
	"Rewrite the L1I probe buffer"
	DEFAULT
	OFF
	DEPENDS "BenchCovertL1I"
)

config_option(
	BenchCovertL1IProbVAddr
	BENCH_COVERT_L1I_PROB_VADDR
	"The trojan and spy uses different vaddr for probing"
	DEFAULT
	OFF
	DEPENDS "BenchCovertL1I"
)

config_option(
	MastikAttack
	MASTIK_ATTACK
	"Multicore cache-based timing attack"
	DEFAULT
	OFF
)

config_choice(
	MastikAttackFormat
	MASTIK_ATTACK_FORMAT
	"Format to use for Mastik attack"
	"covert;MastikAttackCovert;MASTIK_ATTACK_COVERT;MastikAttack"
	"side;MastikAttackSide;MASTIK_ATTACK_SIDE;MastikAttack"
)

config_option(
	BenchSplash
	BENCH_SPLASH
	"Enable splash-2 benchmark"
	DEFAULT
	OFF
)

config_option(
	BenchSplashMorecore
	BENCH_SPLASH_MORECORE
	"Override the morecore allocator for musl libc"
	DEFAULT
	OFF
	DEPENDS "BenchSplash"
)

config_option(
	BenchSplashSwitchIdle
	BENCH_SPLASH_SWITCH_IDLE
	"Enable an idle thread with the splash benchmark"
	DEFAULT
	OFF
	DEPENDS "BenchSplash"
)

config_choice(
	BenchSplashChoice
	BENCH_SPLASH_CHOICE
	"Splash benchmark to run"
	"fft;BenchSplashFFT;BENCH_SPLASH_FFT;BenchSplash"
	"cholesky;BenchSplashCholesky;BENCH_SPLASH_CHOLESKY;BenchSplash"
	"lu;BenchSplashLU;BENCH_SPLASH_LU;BenchSplash"
	"radix;BenchSplashRadix;BENCH_SPLASH_RADIX;BenchSplash"
	"barnes;BenchSplashBarnes;BENCH_SPLASH_BARNES;BenchSplash"
	"fmm;BenchSplashFMM;BENCH_SPLASH_FMM;BenchSplash"
	"ocean;BenchSplashOcean;BENCH_SPLASH_OCEAN;BenchSplash"
	"radiosity;BenchSplashRadiosity;BENCH_SPLASH_RADIOSITY;BenchSplash"
	"raytrace;BenchSplashRaytrace;BENCH_SPLASH_RAYTRACE;BenchSplash"
	"water-nsquared;BenchSplashWaterNSquared;BENCH_SPLASH_WATER_NSQUARED;BenchSplash"
	"water-spatial;BenchSplashWaterSpatial;BENCH_SPLASH_WATER_SPATIAL;BenchSplash"
)

add_config_library(side-bench "${configure_string}")

file(
    GLOB
		deps
        src/*.c
        src/*.S
		src/arch/${KernelArch}/*.c
		src/mastik_common/*.c
)

# x86 files
if (KernelArchX86)
	file(
		GLOB
			arch_deps
			src/mastik/*.S
			src/aes/*.c
			src/mastik/*.c
			src/MPI/*.c
	)
	file(
		GLOB
			arch_include
			src/mastik
	)
endif()

# ARM files
if(KernelArchARM)
	file(
		GLOB
			arch_deps
			src/mastik_arm/*.S
			src/mastik_arm/*.c
	)
	file(
		GLOB
			arch_include
			src/mastik_arm
	)
endif()

# RISC-V files
if (KernelArchRiscV)
	file(
		GLOB
			arch_deps
			src/mastik_riscv/*.S
			src/aes/*.c
			src/mastik_riscv/*.c
			src/MPI/*.c
	)
	file(
		GLOB
			arch_include
			src/mastik_riscv
	)
endif()

if(BenchSplash)
	file(
		GLOB
			splash
			src/splash/*.c
			src/splash/${BenchSplashChoice}/*.c
	)
endif()
set(splash_datafiles "")

if(BenchSplashCholesky)
	file(
		GLOB
			splash_datafiles
			data/cholesky_tk29_data
	)
endif()
if(BenchSplashRaytrace)
	file(
		GLOB
			splash_datafiles
			data/balls4.env
			data/balls4.geo
	)
endif()
if(BenchSplashWaterNSquared)
	file(
		GLOB
			splash_datafiles
			data/random.in
	)
endif()
if(BenchSplashWaterSpatial)
	file(
		GLOB
			splash_datafiles
			data/random.in
	)
endif()

include(cpio)
MakeCPIO(splash-data.o "${splash_datafiles}")


add_executable(
	side-bench
	EXCLUDE_FROM_ALL
		${deps}
		${arch_deps}
		${splash}
		splash-data.o
)
target_include_directories(
    side-bench
    PRIVATE
		include
		"../../include"
		arch/${KernelArch}
		arch_include/${KernelSel4Arch}
		src/mastik_common
		${arch_include}
)

target_link_libraries(
    side-bench
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
        sel4bench
    PRIVATE
		manager_Config
		side-bench_Config
)

target_compile_options(side-bench PRIVATE -DDBG_MEMORY=0)
target_link_options(side-bench PRIVATE -u __vsyscall_ptr -lm)
