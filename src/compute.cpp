#include "compute.h"

#include <cstdio>

#include <CL/cl_gl.h>

#define check_result(msg)							\
{													\
	if (ret != CL_SUCCESS) {						\
		fprintf(stderr, "!%s, Error Code: %d", msg, ret); \
		goto				exit;					\
	}												\
}

#define MAX_SOURCE_SIZE 0x100000

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
    constexpr cl_uint max_platforms = 4;
    std::vector<cl_platform_id> platform_ids;
    cl_platform_id platform_id;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    ret = clGetPlatformIDs(max_platforms, nullptr, &ret_num_platforms);
    platform_ids.resize(ret_num_platforms);
    ret |= clGetPlatformIDs(max_platforms, platform_ids.data(), &ret_num_platforms);
    check_result("Error: CL failed to retrive platform IDs!");
    bool platform_found = false;
    for (int i = 0; i < ret_num_platforms; i++) {
        char extension_string[1024];
        memset(extension_string, ' ', 1024);
        ret = clGetPlatformInfo(platform_ids[i], CL_PLATFORM_EXTENSIONS, sizeof(extension_string), extension_string, nullptr);
        if (ret != CL_SUCCESS) {
            continue;
        }
        char* ext_string_start = nullptr;
        ext_string_start = strstr(extension_string, "cl_khr_gl_sharing");
        if (ext_string_start == 0) {
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

    // GL context props
    cl_context_properties cps[] =
    {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id,
        CL_GL_CONTEXT_KHR, (cl_context_properties)owner->gl_context.hGLRC,
        CL_WGL_HDC_KHR, (cl_context_properties)owner->gl_context.hDC,
        0
    };

    // Select cl device
    clGetGLContextInfoKHR_fn pclGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddressForPlatform(platform_id, "clGetGLContextInfoKHR");
    ret = pclGetGLContextInfoKHR(cps, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(device_id), device_id, NULL);
    check_result("Error: CL failed to select an appropriate device!");

    // Create cl context
    context = clCreateContext(cps, 1, &device_id, NULL, NULL, &ret);
    check_result("Error: CL failed to create context!");

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
    check_result("Error: CL failed to build programm!");

    // Create the OpenCL kernels
    kernel[0] = clCreateKernel(program, "compute_fft", &ret);
    check_result("Error: CL failed to create kernel #0!");

    // allocate input buffer large enough to hold both floats and s16s alike
    input[0] = clCreateBuffer(context, CL_MEM_READ_WRITE, VIZ_BUFFER_SIZE * sizeof(float), NULL, &ret);
    check_result("Error: CL failed allocating input #0!");
    input[1] = clCreateBuffer(context, CL_MEM_READ_WRITE, VIZ_BUFFER_SIZE * sizeof(float), NULL, &ret);
    check_result("Error: CL failed allocating input #1!");
    // zeroize input buffers
    int8_t zero_buf[VIZ_BUFFER_SIZE * sizeof(float)];
    memset(zero_buf, 0, sizeof(zero_buf));
    ret = clEnqueueWriteBuffer(command_queue[0], input[0], CL_TRUE, 0, sizeof(zero_buf), zero_buf, 0, NULL, NULL);
    ret |= clEnqueueWriteBuffer(command_queue[0], input[1], CL_TRUE, 0, sizeof(zero_buf), zero_buf, 0, NULL, NULL);
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
    // outputs are handled by openGL

	ret |= clReleaseContext(context);
	
    if (ret != CL_SUCCESS) {
        fprintf(stderr, "Failed releasing CL objects, Error Code: %d", ret);
    }
	return ret;
}

cl_int compute_fft::init(visualizer::fft_t& fft, producer_consumer<waveform_data>* wf_consumer)
{
	filename = "shaders/fft.cl";
    ssbo_buffer_ids = &fft.ssbo_buffer_ids;

    waveform_consumer = wf_consumer;

    fft.sem_cl.wait(); // wait until ssbo is created
    gl_context.ssbo[0] = fft.SSBO[0];
    gl_context.ssbo[1] = fft.SSBO[1];
    gl_context.ssbo[2] = fft.SSBO[2];
    gl_context.hGLRC = fft.hGLRC;
    gl_context.hDC = fft.hDC;
    if (gl_context.ssbo[0] == 0 || gl_context.ssbo[1] == 0 || gl_context.ssbo[2] == 0 || gl_context.hGLRC == NULL || gl_context.hDC == NULL) {
        puts("GL intialization failed. Exiting...");
        return 0;
    }

	compute_thread = std::thread(compute_mt, this);

    return -1;
}

void compute_fft::deinit()
{
	state_compute.store(0);
	if (compute_thread.joinable()) {
		compute_thread.join();
	}
}

void compute_fft::run()
{
    cl_int ret = CL_SUCCESS;

    const size_t local_item_size = 64;
    const size_t global_item_size = VIZ_BUFFER_SIZE; // we are computing FFTs for both channels independentely

    while (state_compute.load()) {
        const size_t q_id[2] = { (queue_selector + 0) & 0x01, (queue_selector + 1) & 0x01 };
        waveform_data& wf_data = waveform_consumer->begin_consuming();
        const cl_int fp_mode = (cl_int)wf_data.fp_mode;
        const size_t size = VIZ_BUFFER_SIZE * (!fp_mode ? sizeof(int16_t) : sizeof(float));
        ret = clEnqueueWriteBuffer(context.command_queue[q_id[0]], context.input[q_id[0]], CL_FALSE, 0, size * sizeof(float), wf_data.container.data(), 0, NULL, NULL);
        check_result("CL: Failed writing data to device.");

        const cl_int output_id = (cl_int)ssbo_buffer_ids->get_back_buffer();
        ssbo_buffer_ids->publish();
        ret = clEnqueueAcquireGLObjects(context.command_queue[q_id[1]], 1, &context.output[output_id], 0, nullptr, nullptr);
        check_result("CL: Failed acquiring GL objects.");

        // set kernel args
        const cl_int num_fft_bands = FRAMES_PER_BUFFER; // fixed for now
        ret |= clSetKernelArg(context.kernel[0], 0, sizeof(cl_mem), (void*)&context.input[q_id[0]]);
        ret |= clSetKernelArg(context.kernel[0], 1, sizeof(cl_mem), (void*)&context.output[output_id]);
        ret |= clSetKernelArg(context.kernel[0], 3, sizeof(cl_int), (void*)&num_fft_bands);
        ret |= clSetKernelArg(context.kernel[0], 4, sizeof(cl_int), (void*)&fp_mode);
        check_result("CL: Failed setting kernel args.");

        ret = clEnqueueNDRangeKernel(context.command_queue[q_id[1]], context.kernel[0], 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
        check_result("CL: Failed running the kernel.");

        ret = clFinish(context.command_queue[q_id[1]]); // we nned to wait for the kernel to finish to modify queue_selector
        check_result("CL: clFinish failed.");
        ret = clEnqueueReleaseGLObjects(context.command_queue[q_id[1]], 1, &context.output[output_id], 0, nullptr, nullptr);
        check_result("CL: Failed releasing GL objects.");

        queue_selector ^= 0x01;
    }
exit:
    return;
}

void compute_fft::compute_mt(void* args)
{
	compute_fft* self = (compute_fft*)args;

	if (self->context.init(self->filename, self) == CL_SUCCESS) {
		self->run();
		self->context.deinit();
	}
}