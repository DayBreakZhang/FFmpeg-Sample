#pragma once

#ifdef AUDIOTRANSCODER1_EXPORTS
#define DLL_EXPORT __declspec(dllexport) 
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

extern "C" DLL_EXPORT int AudioTranscoderEx3(IN char* insrc,
	IN char *outsrc,IN int channel_num,
	IN int bit_rate,IN int sample_rate);