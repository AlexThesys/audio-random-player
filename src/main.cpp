#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <memory>
#include <stdio.h>

#include "user_input.h"
#include "visualization.h"

volatile  play_params *params_middle_buffer;
volatile LONG new_data;

int main(int argc, char **argv)
{
    if (std::thread::hardware_concurrency() < 2) {
        puts("The application is not meant to be run on a single core system...exiting.");
        return 0;
    }

    const char *folder_path = "audio_data";
    user_params uparams;
    uparams.process_cmdline_args(argc, argv, &folder_path);

    std::unique_ptr<audio_renderer> renderer = std::make_unique<audio_renderer>();

    if (!renderer->init(folder_path, &uparams.max_lenght_samples)) {
        puts("Error loading files...exiting.");
        return -1;
    }

    visualizer audio_viz;
    audio_viz.init();

    play_params p_params[3];
    uparams.get_user_params(&p_params[1]);
    renderer->get_data()->params_front_buffer = &p_params[2];
    params_middle_buffer = &p_params[1];
    new_data = 1;

    renderer->start_rendering();

    pa_player audio_player;
    if (audio_player.init_pa(renderer.get()) != paNoError)
        return -1;

    uparams.run_user_loop(*renderer->get_data(), p_params);

    if (audio_player.deinit_pa() != paNoError)
        return -1;

    audio_viz.deinit();
    renderer->deinit();

    return 0;
}
