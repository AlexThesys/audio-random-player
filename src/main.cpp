// audio_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <windows.h>
#include <tchar.h>

#include "input.h"

#define MAX_DATA_SIZE 0x100000

bool load_files(std::vector<AudioFile<float>> &audioFiles, const char* dirpath) {
    char path[MAX_PATH];
    memset(path, 0, sizeof(path));
    strcpy_s(path, dirpath);
    strcat_s(path, "/*.*");

    TCHAR search_path[MAX_PATH];
    memset(search_path, 0, sizeof(search_path));
    if (MultiByteToWideChar(CP_UTF8, 0, path, (int)strlen(path), search_path, (int)_countof(search_path)) <= 0)
        return false;


    char filename[MAX_PATH];

    audioFiles.reserve(NUM_FILES);

    size_t total_size = 0;

    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                memset(path, 0, sizeof(path));
                memset(filename, 0, sizeof(filename));
                strcpy_s(path, dirpath);
                strcat_s(path, "/");
                const size_t path_len = ::WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, NULL, 0, 0, NULL);
                ::WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, (LPSTR)filename, path_len, 0, NULL);
                strcat_s(path, filename);

                audioFiles.push_back(AudioFile<float>());
                audioFiles.back().load(path);
                total_size += audioFiles.back().getNumSamplesPerChannel() * audioFiles.back().getNumChannels() * sizeof(float);
                if (total_size > MAX_DATA_SIZE) {
                    puts("Max data size exceeded, no more files are going to be loaded.");
                    break;
                }
            }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);
        return true;
    }
    return false;
}

void process_cmdline_args(int argc, char** argv, const char** res, bool *nofadeout) {
    bool nofade;
    for (int i = 1, sz = _min(argc, 3); i < sz; i++) {
        nofade = !strcmp(argv[i], "--no-fadeout");
        if (!nofade) {
            *res = argv[i];
        } else {
            *nofadeout = nofade;
        }
    }
}

int main(int argc, char** argv)
{
    bool disable_fadeout = false;
    const char* path = "audio_data";
    process_cmdline_args(argc, argv, &path, &disable_fadeout);

    std::vector<AudioFile<float>> audioFiles;
    if (!load_files(audioFiles, path)) {
        puts("Error loading files...exiting.");
        return -1;
    }

    play_params p_params[2];
    paData data = paData(&audioFiles);
    int selector = 0;
    get_user_params(p_params, audioFiles, disable_fadeout);
    data.p_params = p_params;

    pa_player audio_player;
    PaError err = audio_player.init_pa(&data);
    if (err != paNoError)
        return -1;

    do {
        puts("\nEnter \'q\' to stop the playback or '\p'\ to change parameters...");
        const char ch = getchar();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'p' || ch == 'P') {
            selector ^= 1;
            get_user_params(&p_params[selector], audioFiles, disable_fadeout);
            data.p_params = &p_params[selector];
        } else {
            while ((getchar()) != '\n');    // flush stdin
        }
    } while (true);

    audio_player.deinit_pa();
    if (err != paNoError)
        return -1;

    return 0;
}
