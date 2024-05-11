#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "constants.h"
#include "user_input.h"
#include "utils.h"
#include "profiling.h"

extern volatile play_params *params_middle_buffer;
extern volatile LONG new_data;

static int calculate_step_time(int walk_speed, const size_t max_lenght_samples, bool no_fadeout)
{
    const float step_size = 0.762f; // for an average man
    const float kph_2_mps_rec = 1.0f / 3.6f;
    const float w_speed = static_cast<float>(walk_speed) * kph_2_mps_rec;
    int step_num_frames;
    if (no_fadeout) { // always play full length of all the files - don't fade out them
        step_num_frames = (int)max_lenght_samples;
    } else {
        step_num_frames = static_cast<int>(ceilf(static_cast<float>(SAMPLE_RATE) * step_size / w_speed));
    }
    return step_num_frames;
}

static inline float calculate_volume_lower_bound(float volume_deviation)
{
    return powf(10, -0.05f * volume_deviation);
}

static int get_input(int default_value)
{
    char line[4];
    memset(line, 0, sizeof(line));
    scanf_s("%s", line, static_cast<unsigned int>(sizeof(line)));
    char *end;
    long l = strtol((const char *)line, &end, (int)10); // C6054 is unfair
    if (end == line || end[0] != '\0') {
        l = default_value;
    }
    return (int)l;
}

bool user_params::get_folder_path() {
    bool res = false;
    memset(folder_path, 0, sizeof(folder_path));

    puts("Input full path to the folder with the audio files (no whitespace permitted):");
    res = !!scanf_s("%s", folder_path, static_cast<unsigned int>(sizeof(folder_path)));

    return res;
} 

void user_params::get_user_params(play_params *data)
{
    puts("Please provide the playback parameters in decimal integer format!");
    puts("Enter any letter to skip the current parameter and use default.");
    int value;
    printf("\nEnter the walk speed in kph [1...12]:\t");
    value = clamp_input(get_input(DEFAULT_WALK_SPEED), MIN_WALK_SPEED, MAX_WALK_SPEED);
    const int step_num_frames = calculate_step_time(value, max_lenght_samples, disable_fadeout);
    printf("%d\n", value);
    printf("\nEnter the pitch deviation in semitones [0...12]:\t");
    value = clamp_input(get_input(DEFAULT_PITCH_DEVIATION), MIN_PITCH_DEVIATION, MAX_PITCH_DEVIATION);
    printf("%d\n", value);
    const float pitch_deviation = static_cast<float>(value);
    printf("\nEnter the volume deviation in dB [0...90]:\t");
    value = clamp_input(get_input(DEFAULT_VOLUME_LOWER_BOUND), MIN_VOLUME_LOWER_BOUND, MAX_VOLUME_LOWER_BOUND);
    printf("%d\n", value);
    const float volume_lower_bound = calculate_volume_lower_bound(static_cast<float>(value));
    printf("\nEnter the LPF frequency deviation in KHZ [0...19]:\t");
    value = clamp_input(get_input(DEFAULT_LPF_FREQ_DEVIATION), MIN_LPF_FREQ_DEVIATION, MAX_LPF_FREQ_DEVIATION);
    printf("%d\n", value);
    const float lpf_freq = static_cast<float>(value) * 1000.0f;
    printf("\nEnter the LPF Q deviation [0...8]:\t");
    value = clamp_input(get_input(DEFAULT_LPF_Q_DEVIATION), MIN_LPF_Q_DEVIATION, MAX_LPF_Q_DEVIATION);
    printf("%d\n", value);
    const float lpf_q = static_cast<float>(value);

    puts("\nEnter non-zero value to use LFO for volume modulation.");
    printf("Enter LFO modulation frequency in Hz [1...10]:\t");
    value = get_input(DEFAULT_LFO_FREQ);
    const bool use_lfo = !!value;
    float lfo_freq = 0.0f;
    float lfo_amount = 0.0f;
    if (use_lfo) {
        value = clamp_input(value, MIN_LFO_FREQ, MAX_LFO_FREQ);
        printf("%d\n", value);
        lfo_freq = static_cast<float>(value);
        printf("\nEnter LFO modulation amount [0...100]:\t");
        value = clamp_input(get_input(DEFAULT_LFO_AMOUNT), MIN_LFO_AMOUNT, MAX_LFO_AMOUNT);
        printf("%d\n", value);
        lfo_amount = static_cast<float>(value) / static_cast<float>(MAX_LFO_AMOUNT);
    }

    while ((getchar()) != '\n'); // flush stdin

    printf("\nEnable distortion? [y/n]\t");
    char ch = static_cast<char>(getchar());
    const bool enable_dist = (ch == 'y' || ch == 'Y');
    printf("%c\n", enable_dist ? 'y' : 'n');

    while ((getchar()) != '\n'); // flush stdin

    printf("\nEnable floating point visualization? [y/n]\t");
    ch = static_cast<char>(getchar());
    const bool enable_fp_wf = (ch == 'y' || ch == 'Y');
    printf("%c\n", enable_fp_wf ? 'y' : 'n');

    while ((getchar()) != '\n'); // flush stdin

    data->init(step_num_frames, pitch_deviation, volume_lower_bound, lpf_freq, lpf_q, lfo_freq, lfo_amount, use_lfo,
               enable_dist, enable_fp_wf);
}

static const char* input_args[] = { "--no-fadeout", "-s=" };

void user_params::process_cmdline_args(int argc, char **argv)
{
     for (int i = 1, sz = _min(argc, (_countof(input_args)+1)); i < sz; i++) {
        if (!strcmp(argv[i], input_args[0])) {
            disable_fadeout = true;
        } else if (argv[i] == strstr(argv[i], input_args[1])) {
            char* end_ptr;
            const char* str = argv[i] + strlen(input_args[1]);
            int32_t smoothing_lvl = (int32_t)strtol(str, &end_ptr, 10);
            if (end_ptr != str) {
                smoothing_lvl = clampr(smoothing_lvl, VIZ_BUFFER_SMOOTHING_LEVEL_MIN, VIZ_BUFFER_SMOOTHING_LEVEL_MAX);
                waveform_smoothing_level = smoothing_lvl;
            }
        }
        // ...
    }
}

// main thread loop
void user_params::run_user_loop(pa_data &data, tripple_buffer<play_params> *p_params)
{
    do {
        puts("\nEnter \'q\' to stop the playback or \'p\' to change parameters...");
        const char ch = static_cast<char>(getchar());
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'p' || ch == 'P') { 
            play_params* params_back_buffer_ptr = p_params->get_back_buffer();
            get_user_params(params_back_buffer_ptr);
            p_params->publish();
        } else {
            while ((getchar()) != '\n'); // flush stdin
        }
    } while (true);
}