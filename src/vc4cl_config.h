/*
 * Author: doe300
 *
 * See the file "LICENSE" for the full license governing this code.
 */

#ifndef VC4CL_CONFIG_H
#define VC4CL_CONFIG_H

#include <CL/opencl.h>

#include <array>
#include <string>
#include <vector>

namespace vc4cl
{
    /*
     * Platform configuration
     */
    namespace platform_config
    {
        static const std::string OPENCL_VERSION = "1.2";
        //#define VC4CL_VERSION is set via CMake
        static const std::string VC4CL_VERSION = VC4CL_LIBRARY_VERSION;
        static const std::string NAME = "OpenCL for the Raspberry Pi VideoCore IV GPU";
        static const std::string VENDOR = "doe300";
        // we can't have FULL_PROFILE, since e.g. long is not supported
        static const std::string PROFILE = "EMBEDDED_PROFILE";
        static const std::string VERSION =
            std::string("OpenCL ") + OPENCL_VERSION + std::string(" VC4CL ") + VC4CL_VERSION;

        static const std::string ICD_SUFFIX = "VC4CL";
        static const std::string VC4CL_PERFORMANCE_EXTENSION = "cl_vc4cl_performance_counters";
        static const std::vector<std::string> EXTENSIONS = {
#if HAS_COMPILER
            // supports SPIR-V code as input for programs (OpenCL 2.0 extension)
            "cl_khr_il_program",
            // supports SPIR (subset of LLVM IR) code as input for programs
            // SPIR is supported by both supported LLVM version ("default" and SPIRV-LLVM)
            "cl_khr_spir",
#endif
            // supports creating of command queue with properties for OpenCL 1.x
            "cl_khr_create_command_queue",
            // supports querying the device temperature with clGetDeviceInfo
            "cl_altera_device_temperature",
            // supports additional functions to query all currently live OpenCL objects
            "cl_altera_live_object_tracking",
#if use_cl_khr_icd
            // supports being used by the Khronos ICD loader
            "cl_khr_icd",
#endif
            VC4CL_PERFORMANCE_EXTENSION};
    } // namespace platform_config

    /*
     * Device configuration
     */
    namespace device_config
    {
        static const std::string NAME = "VideoCore IV GPU";
        static const std::string VENDOR = "Broadcom";
        static constexpr cl_uint VENDOR_ID = 0x0A5C;
        static const std::string COMPILER_VERSION = std::string("OpenCL C ") + platform_config::OPENCL_VERSION + " ";
        // CACHE
        // http://maazl.de/project/vc4asm/doc/VideoCoreIV-addendum.html
        static constexpr cl_uint CACHE_LINE_SIZE = 64;
        static constexpr cl_uint CACHE_SIZE = 32 * 1024;

        /*
         * Device extension configuration
         */
        //"The following approved Khronos extension names must be returned by all device that support OpenCL C 1.2:"
        // cl_khr_global_int32_base_atomics, cl_khr_global_int32_extended_atomics, cl_khr_local_int32_base_atomics,
        // cl_khr_local_int32_extended_atomics, cl_khr_byte_addressable_store"
        // The  following extensions are all core features as of OpenCL 1.2
        //"These transactions are atomic for the device executing these atomic functions. There is no guarantee of
        // atomicity if the atomic operations to the same memory location are being performed by kernels executing on
        // multiple devices. "
        //-> with a mutex on the VPM, the atomic functions are atomic for all QPU-access, but not CPU access!
        // "cl_nv_pragma_unroll" simply specifies, that a "#pragma unroll <factor>" is available as hint to unroll
        // loops, which is natively supported by CLang "cl_arm_get_core_id" offers a method to get the core-ID (OpenCL
        // Compute Unit), the work-group runs on, this is always 0 here
        static const std::vector<std::string> EXTENSIONS = {
            // 32-bit atomics, required to be supported by OpenCL 1.2
            "cl_khr_global_int32_base_atomics",
            // 32-bit atomics, required to be supported by OpenCL 1.2
            "cl_khr_global_int32_extended_atomics",
            // 32-bit atomics, required to be supported by OpenCL 1.2
            "cl_khr_local_int32_base_atomics",
            // 32-bit atomics, required to be supported by OpenCL 1.2
            "cl_khr_local_int32_extended_atomics",
            // byte-wise addressable storage, required to be supported by OpenCL 1.2
            "cl_khr_byte_addressable_store",
#ifdef IMAGE_SUPPORT
            // Supports writing of 3D images
            "cl_khr_3d_image_writes",
            // Support for packed YUV image-types
            "cl_intel_packed_yuv",
#endif
            // officially supports the "#pragma unroll <factor>
            "cl_nv_pragma_unroll",
            // adds function to OpenCL C to query current compute unit
            "cl_arm_core_id",
            // adds a pair of atomic_inc/atomic_dec functions for a 32-bit counter-type, alias to standard
            // atomic_inc/atomic_dec
            "cl_ext_atomic_counters_32",
            // allows local/private memory to be initialized with zeroes before kernel execution
            "cl_khr_initialize_memory",
            // adds a list of integer dot products
            "cl_arm_integer_dot_product_int8", "cl_arm_integer_dot_product_accumulate_int8",
            "cl_arm_integer_dot_product_accumulate_int16"};

        /*
         * "The work-items in a given work-group execute concurrently on the processing elements of a single compute
         * unit." (page 24) Since there is no limitation, that work-groups need to be executed in parallel, we set 1
         * compute unit with all 12 QPUs, allowing us to run 12 work-items in a single work-group in parallel and run
         * the work-groups sequentially.
         */
        static constexpr cl_uint NUM_COMPUTE_UNITS = 1;
        // A QPU (compute unit) is a 16-way SIMD
        static constexpr cl_uint PREFERRED_VECTOR_WIDTH = 16;
        // since the QPU is a 16-way 32-bit processor, the maximum supported type is a 16-element 32-bit vector
        static constexpr cl_uint BUFFER_ALIGNMENT = sizeof(cl_int16);
        // according to tests, values are always rounded to zero
        static constexpr cl_uint FLOATING_POINT_CONFIG = CL_FP_ROUND_TO_ZERO;
    } // namespace device_config

    /*
     * Kernel and parameter configuration
     */
    namespace kernel_config
    {
        /*
         * Work-group configuration
         */
        static constexpr cl_uint NUM_DIMENSIONS = 3;
        static const std::array<size_t, NUM_DIMENSIONS> MAX_WORK_ITEM_DIMENSIONS = {SIZE_MAX, SIZE_MAX, SIZE_MAX};

        /*
         * Parameter configuration
         */
        // the number of UNIFORMS seems to be unlimited, see official documentation, page 91, table 67.
        // But loading more than 64 puts to much pressure on registers (and why would someone need that many anyway?)
        static constexpr cl_uint MAX_PARAMETER_COUNT = 64;
        // the maximum size of images(per dimension)
        // minimum is 2048 (width, height, buffer-size) or 256 (array-size)
        // TMU supports width/height of 2048 pixels
        static constexpr cl_uint MAX_IMAGE_DIMENSION = 2048;

        /*
         * Program configuration
         */
        // magic number to recognize VC4CL binaries, must be the same as set in VC4C
        static constexpr cl_uint BINARY_MAGIC_NUMBER = 0xDEADBEAF;
    } // namespace kernel_config

} // namespace vc4cl

#endif /* VC4CL_CONFIG_H */
