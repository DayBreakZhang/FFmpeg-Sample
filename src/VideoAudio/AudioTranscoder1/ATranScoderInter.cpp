#include "StdAfx.h"
#include "ATranScoderInter.h"

#define SWR_CH_MAX 1024
int filesize;

typedef struct {

	unsigned long samplerate;
	unsigned int bits_per_sample;
	unsigned int channels;
	unsigned long total_samples;

} WavHeader;
int write_wav_header(unsigned char header[], WavHeader *wavHeader) {
	//unsigned char header[44];
	unsigned char* p = header;
	unsigned int bytes = (wavHeader->bits_per_sample + 7) / 8;
	float data_size = (float) bytes * wavHeader->total_samples;
	unsigned long word32;

	*p++ = 'R';
	*p++ = 'I';
	*p++ = 'F';
	*p++ = 'F'; //RIFF 4

	word32 = (unsigned long) data_size + (44 - 8);
	*p++ = (unsigned char) (word32 >> 0);
	*p++ = (unsigned char) (word32 >> 8);
	*p++ = (unsigned char) (word32 >> 16);
	*p++ = (unsigned char) (word32 >> 24); //数据长度 4+4

	*p++ = 'W';
	*p++ = 'A';
	*p++ = 'V';
	*p++ = 'E'; //WAVE 4+4+4

	*p++ = 'f';
	*p++ = 'm';
	*p++ = 't';
	*p++ = ' '; //fmt 4+4+4+4

	*p++ = 0x10;
	*p++ = 0x00;
	*p++ = 0x00;
	*p++ = 0x00; //16  4+4+4+4+4=20

	*p++ = 0x01;
	*p++ = 0x00; //1   4+4+4+4+4+2=22

	*p++ = (unsigned char) (wavHeader->channels >> 0);
	*p++ = (unsigned char) (wavHeader->channels >> 8); //声道数   4+4+4+4+4+2+2=24

	word32 = (unsigned long) (wavHeader->samplerate + 0.5);
	*p++ = (unsigned char) (word32 >> 0);
	*p++ = (unsigned char) (word32 >> 8);
	*p++ = (unsigned char) (word32 >> 16);
	*p++ = (unsigned char) (word32 >> 24); //采样率   4+4+4+4+4+2+2+4=28

	word32 = wavHeader->samplerate * bytes * wavHeader->channels;
	*p++ = (unsigned char) (word32 >> 0);
	*p++ = (unsigned char) (word32 >> 8);
	*p++ = (unsigned char) (word32 >> 16);
	*p++ = (unsigned char) (word32 >> 24); //每秒所需字节数  4+4+4+4+4+2+2+4=32

	word32 = bytes * wavHeader->channels;
	*p++ = (unsigned char) (word32 >> 0);
	*p++ = (unsigned char) (word32 >> 8); // blockAlign（2个字节） 4+4+4+4+4+2+2+4+2=34
	//每个采样需要的字节数，计算公式：声道数 * 每个采样需要的bit  / 8

	*p++ = (unsigned char) (wavHeader->bits_per_sample >> 0);
	*p++ = (unsigned char) (wavHeader->bits_per_sample >> 8); //bitPerSample（2个字节）4+4+4+4+4+2+2+4+2+2=36
	//每个采样需要的bit数，一般为8或16

	*p++ = 'd';
	*p++ = 'a';
	*p++ = 't';
	*p++ = 'a'; //data 4+4+4+4+4+2+2+4+2+2+4=40

	word32 = (unsigned long) data_size;
	*p++ = (unsigned char) (word32 >> 0);
	*p++ = (unsigned char) (word32 >> 8);
	*p++ = (unsigned char) (word32 >> 16);
	*p++ = (unsigned char) (word32 >> 24);
	//size2（4个字节）   4+4+4+4+4+2+2+4+2+2+4+4=44
	// 录音数据的长度，不包括头部长度

	return 0;
}
int ffmpeg_audio_decode(const char * inFile, const char * outFile,
	int channel_num, int bit_rate, int sample_rate) {
		filesize = 1;

		FILE *fout;

		av_register_all();

		AVFrame* frame = av_frame_alloc();
		if (!frame) {
			fprintf(stderr, "Error allocating the frame");
			return 1;
		}

		AVFormatContext* formatContext = NULL;
		if (avformat_open_input(&formatContext, inFile, NULL, NULL) != 0) {
			av_free(frame);
			fprintf(stderr, "Error opening the file");
			return 1;
		}

		if (avformat_find_stream_info(formatContext, NULL) < 0) {
			av_free(frame);
			avformat_close_input(&formatContext);

			fprintf(stderr, "Error finding the stream info");
			return 1;
		}

		AVStream* audioStream = NULL;
		unsigned int i;
		// Find the audio stream (some container files can have multiple streams in them)
		for (i = 0; i < formatContext->nb_streams; ++i) {
			if (formatContext->streams[i]->codec->codec_type
				== AVMEDIA_TYPE_AUDIO) {
					audioStream = formatContext->streams[i];
					break;
			}
		}

		if (audioStream == NULL) {
			av_free(frame);
			avformat_close_input(&formatContext);
			fprintf(stderr, "Could not find any audio stream in the file");
			return 1;
		}

		AVCodecContext* codecContext = audioStream->codec;

		codecContext->codec = avcodec_find_decoder(codecContext->codec_id);
		if (codecContext->codec == NULL) {
			av_free(frame);
			avformat_close_input(&formatContext);
			fprintf(stderr, "Couldn't find a proper decoder");
			return 1;
		} else if (avcodec_open2(codecContext, codecContext->codec, NULL) != 0) {
			av_free(frame);
			avformat_close_input(&formatContext);
			fprintf(stderr, "Couldn't open the context with the decoder");
			return 1;
		}

		/*START*************************START***********************START*/
		/*设置输出格式，为了未来jni调用的简单，所以都传的int，反正是自己用的，直观一些*/
		int64_t outChannelLayout;
		if (channel_num == 1) {
			outChannelLayout = AV_CH_LAYOUT_MONO;
		} else {
			outChannelLayout = AV_CH_LAYOUT_STEREO;
		}

		enum AVSampleFormat outSampleFormat;
		if (bit_rate == 32) {
			outSampleFormat = AV_SAMPLE_FMT_S32;
		} else {
			outSampleFormat = AV_SAMPLE_FMT_S16;
		}

		int outSampleRate = sample_rate; //44100;

		SwrContext* swrContext = swr_alloc_set_opts(NULL, outChannelLayout,
			outSampleFormat, outSampleRate,
			av_get_default_channel_layout(codecContext->channels),
			codecContext->sample_fmt, codecContext->sample_rate, 0, NULL);
		if (swrContext == NULL) {
			av_free(frame);
			avcodec_close(codecContext);
			avformat_close_input(&formatContext);
			fprintf(stderr, "Couldn't create the SwrContext");
			return 1;
		}
		if (swr_init(swrContext) != 0) {
			av_free(frame);
			avcodec_close(codecContext);
			avformat_close_input(&formatContext);
			swr_free(&swrContext);
			fprintf(stderr, "Couldn't initialize the SwrContext");
			return 1;
		}
		/*END**************************END*************************END*/

		std::string _srcfile1=outFile;
		std::string _src11 = _srcfile1.substr(0,_srcfile1.size()-4);
		std::string _src21 = _srcfile1.substr(_srcfile1.size()-4,_srcfile1.size());
		std::string _src32 = "";
		_src32.append(_src11);
		_src32.append("_1");
		_src32.append(".wav");

		fout = fopen(_src32.c_str(), "wb+");

		AVPacket packet;
		av_init_packet(&packet);

		double duration = (double) formatContext->duration / 1000.0 / 1000.0;

		fprintf(stderr, "duration=%f\n", duration);
		int total_sample = (int) duration * channel_num * sample_rate;
		fprintf(stderr, "total_sample=%d\n", total_sample);

		int count = 0;
		fseek(fout,44, SEEK_SET);
		int cout1 = 1;
		int count2=0;
		while (av_read_frame(formatContext, &packet) == 0) {
			if(count2 == 4000){
				cout1++;
				unsigned char header[44];
				WavHeader h;
				h.bits_per_sample = bit_rate;
				h.channels = channel_num;
				h.samplerate = sample_rate;
				h.total_samples = count;
				write_wav_header(header, &h);
				fseek(fout,0, SEEK_SET);
				fwrite(&header, 1, 44, fout);
				fclose(fout);

				std::string _srcfile=outFile;
				std::string _src1 = _srcfile.substr(0,_srcfile.size()-4);
				std::string _src2 = _srcfile.substr(_srcfile.size()-4,_srcfile.size());
				char temp[64];
				std::string str="";
				sprintf(temp, "%d", cout1);
				str = temp;
				_src1.append("_");
				_src1.append(str);
				_src1.append(".wav");
				fout = fopen(_src1.c_str(), "wb+");
				fseek(fout,44, SEEK_SET);
				count=0;
				count2=0;
			}
			count2++;
			if (packet.stream_index == audioStream->index) {
				AVPacket decodingPacket = packet;

				while (decodingPacket.size > 0) {
					// Try to decode the packet into a frame
					int frameFinished = 0;
					int result = avcodec_decode_audio4(codecContext, frame,
						&frameFinished, &decodingPacket);

					if (result < 0 || frameFinished == 0) {
						break;
					}

					unsigned char buffer[100000];
					unsigned char* pointers[SWR_CH_MAX];
					pointers[0] = &buffer[0];

					//这个地方还不是很清楚，看官方的example感觉好复杂。尤其是参数。
					int numSamplesOut = swr_convert(swrContext, pointers,
						outSampleRate,
						(const unsigned char**) frame->extended_data,
						frame->nb_samples);

					fwrite((short *) buffer, sizeof(short),
						(size_t) numSamplesOut * channel_num, fout);
					count += numSamplesOut* channel_num;
					decodingPacket.size -= result;
					decodingPacket.data += result;
				}

			}
			av_free_packet(&packet);
		}
		fprintf(stderr, "count=%d\n", count);

		unsigned char header[44];
		WavHeader h;
		h.bits_per_sample = bit_rate;
		h.channels = channel_num;
		h.samplerate = sample_rate;
		h.total_samples = count;
		write_wav_header(header, &h);
		fseek(fout,0, SEEK_SET);
		fwrite(&header, 1, 44, fout);

		// Clean up!
		av_free(frame);
		avcodec_close(codecContext);
		avformat_close_input(&formatContext);
		fclose(fout);
		filesize = cout1;
		return filesize;
}


extern "C" DLL_EXPORT int AudioTranscoderEx3(IN char* insrc, IN char *outsrc,IN int channel_num, IN int bit_rate,IN int sample_rate)
{
	return ffmpeg_audio_decode(insrc, outsrc, channel_num, bit_rate, sample_rate);
}
