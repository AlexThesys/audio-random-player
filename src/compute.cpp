#include "compute.h"

#include <cstdio>

#include <CL/cl_gl.h>

#include "profiling.h"

#define check_result(msg)							\
{													\
	if (ret != CL_SUCCESS) {						\
		fprintf(stderr, "!%s, Error Code: %d", msg, ret); \
		goto				exit;					\
	}												\
}

#define MAX_SOURCE_SIZE 0x100000

#define WORK_GROUP_SIZE 0x40

cl_int compute_fft::compute_context::init(const char* filename, const compute_fft* owner)
{
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    char* source_str = (char*)malloc(MAX_SOURCE_SIZE);
    size_t source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    // Get platform and device information
    cl_int ret = CL_SUCCESS;
    std::vector<cl_platform_id> platform_ids;
    std::vector<cl_device_id> device_ids;
    cl_platform_id platform_id;
    cl_uint ret_num_platforms;
    ret = clGetPlatformIDs(0, nullptr, &ret_num_platforms);
    platform_ids.resize(ret_num_platforms);
    ret |= clGetPlatformIDs(ret_num_platforms, platform_ids.data(), nullptr);
    check_result("Error: CL failed to retrive platform IDs!");
    bool platform_found = false;
    for (int i = 0; i < ret_num_platforms; i++) {
        size_t extension_size;
        ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_EXTENSIONS, 0, NULL, &extension_size);
        check_result("Error CL: Failed retrieving platform info!");
        char* extensions = (char*)malloc(extension_size);
        memset(extensions, 0, extension_size);
        ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_EXTENSIONS, extension_size, extensions, nullptr);
        check_result("Error CL: Failed retrieving platform info!");
        char* ext_string_start = nullptr;
        ext_string_start = strstr(extensions, "cl_khr_gl_sharing");
        if (ext_string_start != nullptr) {
            platform_id = platform_ids[i];
            platform_found = true;
            break;
        }
    }
    if (!platform_found) {
        puts("Platform doesn't support cl_khr_gl_sharing.");
        free(source_str);
        return -1;
    }

    // get device IDs
    cl_uint num_devices;
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
    check_result("Error CL: couldn't retrieve device ids.");
    device_ids.resize(num_devices);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, num_devices, device_ids.data(), nullptr);
    check_result("Error CL: couldn't retrieve device ids.");
    bool device_found = false;
    for (int i = 0; i < num_devices; i++) {
        size_t extension_size;
        ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_EXTENSIONS, 0, NULL, &extension_size);
        check_result("Error CL: Failed retrieving device info!");
        char* extensions = (char*)malloc(extension_size);
        memset(extensions, 0, extension_size);
        ret = clGetDeviceInfo(device_ids[i], CL_DEVICE_EXTENSIONS, extension_size, extensions, nullptr);
        check_result("Error CL: Failed retrieving device info!");
        char* ext_string_start = nullptr;
        ext_string_start = strstr(extensions, "cl_khr_gl_sharing");
        if (ext_string_start != nullptr) {
            device_id = device_ids[i];
            device_found = true;
            break;
        }
    }
    if (!device_found) {
        puts("Device doesn't support cl_khr_gl_sharing.");
        free(source_str);
        return -1;
    }

        // GL context props
    cl_context_properties cps[] =
    {
        CL_GL_CONTEXT_KHR, (cl_context_properties)owner->gl_context.hGLRC,
        CL_WGL_HDC_KHR, (cl_context_properties)owner->gl_context.hDC,
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id,
        0
    };

    // Create cl context
    context = clCreateContext(cps, 1, &device_id, NULL, NULL, &ret);
    check_result("Error: CL failed to create context!");

    // Select cl device bound to the gl context
    clGetGLContextInfoKHR_fn pclGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(platform_id, "clGetGLContextInfoKHR");
    size_t dev_bytes = 0;
    ret = pclGetGLContextInfoKHR(cps, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, 0, NULL, &dev_bytes);
    check_result("Error CL: clGetGLContextInfoKHR failed!");
    const cl_uint num_devs = dev_bytes / sizeof(cl_device_id);
    if (num_devs <= 0) {
        puts("CL: no devices bound to GL context found!");
        goto exit;
    }
    device_ids.resize(num_devs);
    ret = pclGetGLContextInfoKHR(cps, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, dev_bytes, device_ids.data(), NULL);
    check_result("Error CL: clGetGLContextInfoKHR failed!");
    device_id = device_ids[0]; // just pick the first one

    // Create command queues
    command_queue[0] = clCreateCommandQueue(context, device_id, 0, &ret);  // in-order execution
    check_result("Error: CL failed to create command queue #0!");
    command_queue[1] = clCreateCommandQueue(context, device_id, 0, &ret);  // in-order execution
    check_result("Error: CL failed to create command queue #1!");

    // Create a program from the kernel source
    program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t*)&source_size, &ret);
    check_result("Error: CL failed to create programm!");

    // Build the program
    ret = clBuildProgram(program, 1, &device_id, "-Werror -cl-denorms-are-zero -cl-fast-relaxed-math", NULL, NULL);
    if (ret != CL_SUCCESS) {
        size_t log_size;
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        check_result("Error: CL failed retrieving shader compilation error log!");
        char* log = (char*)malloc(log_size);
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        if (ret != CL_SUCCESS) {
            free(log);
            check_result("Error: CL failed retrieving shader compilation error log!");
        }
        printf("Compilation log:\n%s\n", log);
        free(log);
    }

    // Create the OpenCL kernels
    kernel[0] = clCreateKernel(program, "compute_fft", &ret);
    check_result("Error: CL failed to create kernel #0!");

    // allocate input buffer large enough to hold both floats and s16s alike
    input[0] = clCreateBuffer(context, CL_MEM_READ_ONLY, VIZ_BUFFER_SIZE * sizeof(float), NULL, &ret);
    check_result("Error: CL failed allocating input #0!");
    input[1] = clCreateBuffer(context, CL_MEM_READ_ONLY, VIZ_BUFFER_SIZE * sizeof(float), NULL, &ret);
    check_result("Error: CL failed allocating input #1!");
    // zeroize input buffers
    int8_t zero_buf[VIZ_BUFFER_SIZE * sizeof(float)];
    memset(zero_buf, 0, sizeof(zero_buf));
    ret = clEnqueueWriteBuffer(command_queue[0], input[0], CL_FALSE, 0, sizeof(zero_buf), zero_buf, 0, NULL, NULL);
    ret |= clEnqueueWriteBuffer(command_queue[1], input[1], CL_FALSE, 0, sizeof(zero_buf), zero_buf, 0, NULL, NULL);
    check_result("Error: CL failed intializing input buffers!");

    // link output buffers
    output[0] = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, owner->gl_context.ssbo[0], &ret);
    check_result("Error: CL failed linking output #0!");
    output[1] = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, owner->gl_context.ssbo[1], &ret);
    check_result("Error: CL failed linking output #1!");
    output[2] = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, owner->gl_context.ssbo[2], &ret);
    check_result("Error: CL failed linking output #2!");

exit:
    free(source_str);
    return ret;
}

cl_int compute_fft::compute_context::deinit()
{
	cl_int ret = 0;

	ret |= clFlush(command_queue[0]);
	ret |= clFlush(command_queue[1]);
	ret |= clFinish(command_queue[0]);
	ret |= clFinish(command_queue[1]);
	ret |= clReleaseKernel(kernel[0]);
	ret |= clReleaseProgram(program);
	ret |= clReleaseCommandQueue(command_queue[0]);
	ret |= clReleaseCommandQueue(command_queue[1]);

    ret = clReleaseMemObject(input[0]);
    ret = clReleaseMemObject(input[1]);
    ret = clReleaseMemObject(output[0]);
    ret = clReleaseMemObject(output[1]);
    ret = clReleaseMemObject(output[2]);

	ret |= clReleaseContext(context);
	
    if (ret != CL_SUCCESS) {
        fprintf(stderr, "Failed releasing CL objects, Error Code: %d", ret);
    }
	return ret;
}

cl_int compute_fft::init(visualizer::fft_t& fft)
{
	filename = "shaders/fft.cl";
    ssbo_buffer_ids = &fft.ssbo_buffer_ids;

    gl_context.ssbo[0] = fft.SSBO[0];
    gl_context.ssbo[1] = fft.SSBO[1];
    gl_context.ssbo[2] = fft.SSBO[2];
    gl_context.hGLRC = fft.hGLRC;
    gl_context.hDC = fft.hDC;

    return context.init(filename, this) == CL_SUCCESS;
}

void compute_fft::deinit()
{
	state_compute.store(0);
    waveform_consumer->deinit(); // wake up the compute thread
	if (compute_thread.joinable()) {
		compute_thread.join();
	}
}

void compute_fft::run()
{
    cl_int ret = CL_SUCCESS;

    const size_t local_item_size = WORK_GROUP_SIZE;
    const size_t global_item_size = VIZ_BUFFER_SIZE; // we are computing FFTs for both channels independentely

    while (state_compute.load()) {
        const size_t q_id[2] = { (queue_selector + 0) & 0x01, (queue_selector + 1) & 0x01 };
        waveform_data& wf_data = waveform_consumer->begin_consuming();
        if (!state_compute.load()) {
            break;
        }

        PROFILE_START("compute_fft::run");

        const cl_int fp_mode = (cl_int)wf_data.fp_mode;
        const size_t size = VIZ_BUFFER_SIZE * (!fp_mode ? sizeof(int16_t) : sizeof(float));
        ret = clEnqueueWriteBuffer(context.command_queue[q_id[0]], context.input[q_id[0]], CL_FALSE, 0, size, wf_data.container.data(), 0, NULL, NULL);
        check_result("CL: Failed writing data to device.");

        const cl_int output_id = (cl_int)ssbo_buffer_ids->get_back_buffer();
        ssbo_buffer_ids->publish();
        ret = clEnqueueAcquireGLObjects(context.command_queue[q_id[1]], 1, &context.output[output_id], 0, nullptr, nullptr);
        check_result("CL: Failed acquiring GL objects.");

        // set kernel args
        const cl_int num_fft_bands = FRAMES_PER_BUFFER; // fixed for now
        ret |= clSetKernelArg(context.kernel[0], 0, sizeof(cl_mem), (void*)&context.input[q_id[0]]);
        ret |= clSetKernelArg(context.kernel[0], 1, sizeof(cl_mem), (void*)&context.output[output_id]);
        ret |= clSetKernelArg(context.kernel[0], 2, sizeof(cl_int), (void*)&fp_mode);
        check_result("CL: Failed setting kernel args.");

        ret = clEnqueueNDRangeKernel(context.command_queue[q_id[1]], context.kernel[0], 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
        check_result("CL: Failed running the kernel.");

        ret = clFinish(context.command_queue[q_id[1]]); // we nned to wait for the kernel to finish to modify queue_selector
        check_result("CL: clFinish failed.");
        ret = clEnqueueReleaseGLObjects(context.command_queue[q_id[1]], 1, &context.output[output_id], 0, nullptr, nullptr);
        check_result("CL: Failed releasing GL objects.");

        queue_selector ^= 0x01;

        PROFILE_STOP("compute_fft::run");
    }
exit:
    gl_sem->signal();

    return;
}

cl_int compute_fft::run_compute(semaphore& gl_sem) {
    this->gl_sem = &gl_sem;
    gl_sem.wait(); // wait until the context is initialized

    if (gl_context.ssbo[0] == 0 || gl_context.ssbo[1] == 0 || gl_context.ssbo[2] == 0 || gl_context.hGLRC == NULL || gl_context.hDC == NULL) {
        puts("GL intialization failed. Exiting...");
        return -1;
    }

    compute_thread = std::thread(compute_mt, this);

    return 0;
}

void compute_fft::compute_mt(void* args)
{
    PROFILE_SET_THREAD_NAME("CL/Compute FFT");

	compute_fft* self = (compute_fft*)args;
    self->run();
    self->context.deinit();
}