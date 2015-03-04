// FFmpegAudioConver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

std::string in_filename  = "";//Input file URL
std::string out_filename = "";//Output file URL
int in_sample_rate=0;//采样率
int in_channels = 0;//通道

int Parsng_cmd(int argc,_TCHAR* argv[])
{
	string tmpPara = "";  
	for(int i=1;i <argc; i++)  
	{  
		if(strlen(DayBreak::util::UnicodeToAnsi(argv[i])) == 0) //处理空字符串  
		{  
			tmpPara += char(31);  
		}  
		else  
		{  
			tmpPara += DayBreak::util::UnicodeToAnsi(argv[i]);  
		}  
		tmpPara += " ";  
	}  
	std::map<std::string, std::vector<std::string> > result;  
	DayBreak::ParsingArgs pa;  
	pa.AddArgType('i',"InputFile", DayBreak::ParsingArgs::MUST_VALUE);  
	pa.AddArgType('o',"OutFile", DayBreak::ParsingArgs::MUST_VALUE);  
	pa.AddArgType('s',"sample", DayBreak::ParsingArgs::MUST_VALUE); 
	pa.AddArgType('c',"channel", DayBreak::ParsingArgs::MUST_VALUE); 
	bool bExit = false;  
	//do  
	{  
		result.clear();  
		std::string errPos;  
		int iRet = pa.Parse(tmpPara,result, errPos);  
		if(0>iRet)  
		{  
			return -1; 
		}  
		else  
		{  
			map<std::string, std::vector<std::string> >::iterator it = result.begin();  
			for(; it != result.end(); ++it)  
			{  
				if(it->first == "i")
				{
					for(int i=0; i<it->second.size(); ++i)  
					{  
						in_filename = it->second[i];
					}  
				}
				if(it->first == "v")
				{
					for(int i=0; i<it->second.size(); ++i)  
					{  
						out_filename = it->second[i];
					}  
				}
				if(it->first == "s")
				{
					for(int i=0; i<it->second.size(); ++i)  
					{  
						in_sample_rate = atoi(it->second[i].c_str());
					}  
				}
				if(it->first == "c")
				{
					for(int i=0; i<it->second.size(); ++i)  
					{  
						in_channels = atoi(it->second[i].c_str());
					}  
				}		
			}  
		}  

	}//while(!bExit);  
	return 0;
}

int AudioResampling(AVCodecContext * audio_dec_ctx,AVFrame * pAudioDecodeFrame,
	int out_sample_fmt,int out_channels ,int out_sample_rate , uint8_t * out_buf)
{
	//////////////////////////////////////////////////////////////////////////
	SwrContext * swr_ctx = NULL;
	int data_size = 0;
	int ret = 0;
	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO; //初始化这样根据不同文件做调整
	int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO; //这里设定ok
	int dst_nb_channels = 0;
	int dst_linesize = 0;
	int src_nb_samples = 0;
	int dst_nb_samples = 0;
	int max_dst_nb_samples = 0;
	uint8_t **dst_data = NULL;
	int resampled_data_size = 0;

	//重新采样
	if (swr_ctx)
	{
		swr_free(&swr_ctx);
	}
	swr_ctx = swr_alloc();
	if (!swr_ctx)
	{
		printf("swr_alloc error \n");
		return -1;
	}

	src_ch_layout = (audio_dec_ctx->channel_layout && 
		audio_dec_ctx->channels == 
		av_get_channel_layout_nb_channels(audio_dec_ctx->channel_layout)) ? 
		audio_dec_ctx->channel_layout : 
	av_get_default_channel_layout(audio_dec_ctx->channels);

	if (out_channels == 1)
	{
		dst_ch_layout = AV_CH_LAYOUT_MONO;
	}
	else if(out_channels == 2)
	{
		dst_ch_layout = AV_CH_LAYOUT_STEREO;
	}
	else
	{
		//可扩展
	}

	if (src_ch_layout <= 0)
	{
		printf("src_ch_layout error \n");
		return -1;
	}

	src_nb_samples = pAudioDecodeFrame->nb_samples;
	if (src_nb_samples <= 0)
	{
		printf("src_nb_samples error \n");
		return -1;
	}

	/* set options */
	av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate",       audio_dec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate",       out_sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (AVSampleFormat)out_sample_fmt, 0);
	swr_init(swr_ctx);

	max_dst_nb_samples = dst_nb_samples =
		av_rescale_rnd(src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate, AV_ROUND_UP);
	if (max_dst_nb_samples <= 0)
	{
		printf("av_rescale_rnd error \n");
		return -1;
	}

	dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
	ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
		dst_nb_samples, (AVSampleFormat)out_sample_fmt, 0);
	if (ret < 0)
	{
		printf("av_samples_alloc_array_and_samples error \n");
		return -1;
	}


	dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_dec_ctx->sample_rate) +
		src_nb_samples, out_sample_rate, audio_dec_ctx->sample_rate,AV_ROUND_UP);
	if (dst_nb_samples <= 0)
	{
		printf("av_rescale_rnd error \n");
		return -1;
	}
	if (dst_nb_samples > max_dst_nb_samples)
	{
		av_free(dst_data[0]);
		ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
			dst_nb_samples, (AVSampleFormat)out_sample_fmt, 1);
		max_dst_nb_samples = dst_nb_samples;
	}

	data_size = av_samples_get_buffer_size(NULL, audio_dec_ctx->channels,
		pAudioDecodeFrame->nb_samples,
		audio_dec_ctx->sample_fmt, 1);
	if (data_size <= 0)
	{
		printf("av_samples_get_buffer_size error \n");
		return -1;
	}
	resampled_data_size = data_size;

	if (swr_ctx)
	{
		ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, 
			(const uint8_t **)pAudioDecodeFrame->data, pAudioDecodeFrame->nb_samples);
		if (ret <= 0)
		{
			printf("swr_convert error \n");
			return -1;
		}

		resampled_data_size = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
			ret, (AVSampleFormat)out_sample_fmt, 1);
		if (resampled_data_size <= 0)
		{
			printf("av_samples_get_buffer_size error \n");
			return -1;
		}
	}
	else 
	{
		printf("swr_ctx null error \n");
		return -1;
	}

	//将值返回去
	memcpy(out_buf,dst_data[0],resampled_data_size);

	if (dst_data)
	{
		av_freep(&dst_data[0]);
	}
	av_freep(&dst_data);
	dst_data = NULL;

	if (swr_ctx)
	{
		swr_free(&swr_ctx);
	}
	return resampled_data_size;
}

int _tmain(int argc, _TCHAR* argv[])
{

	return 0;
}

