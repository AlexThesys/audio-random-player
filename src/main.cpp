#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <memory>
#include <stdio.h>

#include "user_input.h"
#include "visualization.h"
#include "compute.h"
#include "profiling.h"

int main(int argc, char **argv)
{
    PROFILE_SET_THREAD_NAME("Main/User");

    int ret = 0;
    if (std::thread::hardware_concurrency() < 2) {
        puts("The application is not meant to be run on a single core system...exiting.");
        return 1;
    }

    user_params u_params;
    u_params.process_cmdline_args(argc, argv);
    u_params.get_folder_path();

    std::unique_ptr<audio_renderer> audio_engine = std::make_unique<audio_renderer>();

    if (!audio_engine->init(u_params.folder_path, &u_params.max_lenght_samples)) {
        puts("Error loading files...exiting.");
        return -1;
    }
    // graphics is initialized afer audio engine
    visualizer graphics_engine;
    graphics_engine.init(audio_engine->get_waveform_data_buffer(), u_params.waveform_smoothing_level);

    // cl is initialized after both audio and video engines
    compute_fft fft_cl;
    if (0 != fft_cl.init(graphics_engine.get_fft_data(), audio_engine->get_waveform_producer())) {
        ret = -1;
        goto exit;
    }

    u_params.get_user_params(audio_engine->get_params_buffer()->get_data(1));

    audio_engine->start_rendering();

    pa_player audio_player;
    if (audio_player.init_pa(audio_engine.get()) != paNoError) {
        ret = -1;
        goto exit;
    }

    u_params.run_user_loop(*audio_engine->get_data(), audio_engine->get_params_buffer());

    if (audio_player.deinit_pa() != paNoError) {
        ret = -1;
    }

exit:
    // order is important
    fft_cl.deinit();
    graphics_engine.deinit();
    audio_engine->deinit();

    return ret;
}
