#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <memory>
#include <stdio.h>

#include "user_input.h"
#include "visualization.h"
#include "profiling.h"

int main(int argc, char **argv)
{
    PROFILE_SET_THREAD_NAME("Main/User");

    int ret = 0;
    if (std::thread::hardware_concurrency() < 2) {
        puts("The application is not meant to be run on a single core system...exiting.");
        return 1;
    }

    user_params uparams;
    uparams.process_cmdline_args(argc, argv);
    uparams.get_folder_path();

    std::unique_ptr<audio_renderer> renderer = std::make_unique<audio_renderer>();

    if (!renderer->init(uparams.folder_path, &uparams.max_lenght_samples)) {
        puts("Error loading files...exiting.");
        return -1;
    }

    visualizer audio_viz;
    audio_viz.init(renderer->get_viz_data_buffer());

    uparams.get_user_params(renderer->get_params_buffer()->get_data(1));

    renderer->start_rendering();

    pa_player audio_player;
    if (audio_player.init_pa(renderer.get()) != paNoError) {
        ret = -1;
        goto exit;
    }

    uparams.run_user_loop(*renderer->get_data(), renderer->get_params_buffer());

    if (audio_player.deinit_pa() != paNoError) {
        ret = -1;
    }

exit:
    audio_viz.deinit();
    renderer->deinit();

    return ret;
}
