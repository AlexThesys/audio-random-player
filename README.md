***Main Features List***

* Looped audio playback of the provided files in random order without playing the same sound twice in a row
* Setting the speed of the playback which emulates (sort of) walking speed
* Randomized pitch with custom parameters
* Volume can be either randomized or modulated with a built-in LFO
* Low pass filter with randomized cutoff frequency and Q
* Optional distortion processing
* Ability to enter custom parameters from console and change the on the fly
* Option to specify audio files folder

***Description***
- Solution is x86 & Windows specific
- Overall I tryed to keep my solution simple, solving specifically the problems at hand. I didn't attempt to design the system to be a part of something bigger or 'design for the future', although I still tryed to make certain parts of the code reusable.
- I didn't try to adopt the code style that you use in cryengine in this project because of the time constrains. Instead I went with what I am more used (with slight modifications, e.g. no C-style casts) to prioritize consistency (although I used clang format). Overall I tryed not to overcomplicate things for no reason.
- I've intentionaly avoided using std::string and iostream operations. I didn't have enough time to find a library with a proper implementation, so I resorted to using C API for string, std io and file io operations.
- Probably my main desing decision in how the playback is handled. For this type of random container, within a timeframe that I had available, I went for a simpler solution. If I were to try and implement sample accurate switching between audio files, then I'd need to have the next file prepared when the previous one stops plaing within the same callback. With the current solution the switch to the next file would only be possible on the next callback.
- The randomization of the parameters happens, when the next file is (randomly) selected. The randomizer itself (librandom.h) is not my code, I just wanted something simpler and more lightweight that what c++ <random> provides.
- Since provided audio files contain step sounds, I've decided that it would be interesting to specify the playback speed as a walking speed (in kph) instead of a more generic solution.
- I used linear interpolation for handling audio pitch. User can provide the pitch range in semitones.
- The volume can be either randomized within a range (specified in dB) or be modulated via LFO. If using randomization user is specifing the lower bound of the range (upper is 0 dB).
- Filter is a simple second order biquad low pass filter. I think the processing function is pretty efficient though. 
- LFO is a wavetable precomputed at the start of the application. It has configurable rate and amount parameters. It can only produce sine wave.
- Distortion has no user-configurable parameters. It can either be turnen on or off. I've done the vectorized version of it beforehand, but decided to go with a simpler one here.
- All the user parameters can be entered from console and changed while the application is running without interrupting the playback. Despite having no sycnronization primives between main and audio threads, I've made sure that the data won't be corrupted. Basically I use double buffering and switch the pointer (shared between thread) atomically.
User can enter any random letter instead of a numeric value in order to use a default parameter value.
- Two commandline parameters can be optionally provided to the programm (in any order):
	1. Custom path to the folder containing audio files. If not provided, application will use the default path relative to solution. Its going to try and load all the *.wav files in this folder.
	2. --no-fadeout - this option changes the behavior of how the playback is handled when the timeframe (defined by "walk speed") is less than the current file lenght.
- I've provided	both a .sln file and a cmake file just in case. The former is a prefered option. If generating solution with cmake you might need to manually specify the path to the folder containig audio files. When using provided .sln file, the folder should be found automatically.
- Note that due to the way cmake generate solutions, in order for it to be compatitable with the provided .sln file out of the box, the 'Character Set' option is set to 'Use Multi-Byte Character Set'. 

***Limitations & Future Improvements***

* Streaming of audio files:
	For now all the files are just loaded into memory on the start (althoug there is a hard limit on the total data size). In a real application the loading should be handeled on a separate thread in chunks as needed.
* SIMD DSP processing:
	Didn't have time to vectorize most of the DSP computations. Although manual vectorization by itself would've not been that hard, I've decided aginst doing it for now for two main reasons: 
			1. It would've been required to properly handle data aligment and padding, which I didn't have time to do.
			2. I didn't have time to profile the application and going blind would've been pointless.
* Unicode support for filenames and path:
	Actually I had to remove to be compatitable with the cmake generated solution. I still have it version controlled.
* Support for multiple samplerates, buffer sizes:
	For now its hardcoded to 48 Khz, 256 frames. Files with different samplerate would play with a wrong pitch. Could have made it so that they would be resampled on load, but didn't have time to do that.
* Support for multiple output channels configurations:
	For now it only supports 2 channels stereo
* Option to load parameters from a configuration file.
* Ability to play sounds completly seamlesly - sample accurate:
	My solution does not currently provide this option. To implement this it'll be required to almost completely rewrite all the playback code. It would be a different type of container then. But all the processing code should be reusable.
* Ability to blend different sounds:
	Once again it would be a different type of a container in terms of how a playback is handled, but still an interesting future project.
* Reverberation:
	Didn't have time to implement reverb. I have a solution for reverberation that I made before, but it can only handle four channel output. And it would've taken me too much time to implement something from scratch, that would actually sound any good. So, its' also something to do in the future.
* Add more waveforms to the LFO.
* Alternative interpolation method (e.g. cubic) for the audio pitch.
* User parameters for distortion.
* Bonus: option to change audio pitch without changing playback speed:
	It would not be easy to implement (it's basically granular synthezis), but it's a pretty cool feature and I always wanted to try implementing this.

- If you find any of the missing features or limitations crucial, please don't hesitate to contact me.