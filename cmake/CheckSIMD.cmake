include(CheckCSourceRuns)

# Check MMX
if(CMAKE_COMPILER_IS_GNUCC)
	set(MMX_FLAG -mmmx)
elseif(MSVC)
	set(MMX_FLAG /arch:MMX)
endif()
set(CMAKE_REQUIRED_FLAGS ${MMX_FLAG})
check_c_source_runs(
	"#include <mmintrin.h>
	int main(){_mm_setzero_si64(); return 0;}"
	MMX_FOUND)
if(MMX_FOUND)
	set(MMX_SWITCH ON)
else()
	set(MMX_SWITCH OFF)
endif()

# Check SSE2
if(CMAKE_COMPILER_IS_GNUCC)
	set(SSE2_FLAG -msse2)
elseif(MSVC)
	set(SSE2_FLAG /arch:SSE2)
endif()
set(CMAKE_REQUIRED_FLAGS ${SSE2_FLAG})
check_c_source_runs(
	"#include <emmintrin.h>
	int main(){_mm_setzero_si128(); return 0;}"
	SSE2_FOUND)
if(SSE2_FOUND)
	set(SSE2_SWITCH ON)
else()
	set(SSE2_SWITCH OFF)
endif()

# Check AVX
if(CMAKE_COMPILER_IS_GNUCC)
	set(AVX_FLAG -mavx)
elseif(MSVC)
	set(AVX_FLAG /arch:AVX)
endif()
set(CMAKE_REQUIRED_FLAGS ${AVX_FLAG})
check_c_source_runs(
	"#include <avxintrin.h>
	int main(){_mm256_setzero_si256(); return 0;}"
	AVX_FOUND)
if(AVX_FOUND)
	set(AVX_SWITCH ON)
else()
	set(AVX_SWITCH OFF)
endif()