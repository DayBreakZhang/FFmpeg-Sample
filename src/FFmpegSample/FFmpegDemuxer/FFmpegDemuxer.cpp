// FFmpegDemuxer.cpp : Defines the entry point for the console application.
//��򵥵Ļ���FFmpeg������Ƶ������ 



#include "stdafx.h"
//fuz
std::string in_filename  = "";//Input file URL
std::string out_filename_v = "";//Output file URL
std::string out_filename_a = "";

int Parsng_cmd(int argc,_TCHAR* argv[])
{
	string tmpPara = "";  
	for(int i=1;i <argc; i++)  
	{  
		if(strlen(util::UnicodeToAnsi(argv[i])) == 0) //������ַ���  
		{  
			tmpPara += char(31);  
		}  
		else  
		{  
			tmpPara += util::UnicodeToAnsi(argv[i]);  
		}  
		tmpPara += " ";  
	}  
	std::map<std::string, std::vector<std::string> > result;  
	DayBreak::ParsingArgs pa;  
	pa.AddArgType('i',"InputFile", DayBreak::ParsingArgs::MUST_VALUE);  
	pa.AddArgType('v',"OutFileVideo", DayBreak::ParsingArgs::MUST_VALUE);  
	pa.AddArgType('a',"OutFileAudio", DayBreak::ParsingArgs::MUST_VALUE);  
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
						out_filename_v = it->second[i];
					}  
				}
				if(it->first == "a")
				{
					for(int i=0; i<it->second.size(); ++i)  
					{  
						out_filename_a = it->second[i];
					}  
				}				
			}  
		}  
		
	}//while(!bExit);  
	return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	//�������õı���
	AVOutputFormat *ofmt_a = NULL,*ofmt_v = NULL;//�����format
	//��Input AVFormatContext and Output AVFormatContext��
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_a = NULL, *ofmt_ctx_v = NULL;//��������������
	AVPacket pkt;//д��İ�����
	int ret, i;//���ִ���¼���
	int videoindex=-1,audioindex=-1;//��Ƶ����Ƶ��Ӧ��id
	int frame_index=0;//֡��

	if(Parsng_cmd(argc,argv) !=0)
	{
		return -1;
	}

	cout<<in_filename.c_str()<<endl;
	cout<<out_filename_v.c_str()<<endl;
	cout<<out_filename_a.c_str()<<endl;

	//ִ��ffmpeg����
	av_register_all();//ע��ffmpeg����
	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename.c_str(), 0, 0)) < 0) {
		printf( "Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf( "Failed to retrieve input stream information");
		goto end;
	}
	//Output
	avformat_alloc_output_context2(&ofmt_ctx_v, NULL, NULL, out_filename_v.c_str());
	if (!ofmt_ctx_v) {
		printf( "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt_v = ofmt_ctx_v->oformat;

	avformat_alloc_output_context2(&ofmt_ctx_a, NULL, NULL, out_filename_a.c_str());
	if (!ofmt_ctx_a) {
		printf( "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt_a = ofmt_ctx_a->oformat;

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVFormatContext *ofmt_ctx;
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = NULL;

		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			out_stream=avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);
			ofmt_ctx=ofmt_ctx_v;
		}else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
			audioindex=i;
			out_stream=avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);
			ofmt_ctx=ofmt_ctx_a;
		}else{
			break;
		}

		if (!out_stream) {
			printf( "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//Copy the settings of AVCodecContext
		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
			printf( "Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;

		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format------------------
	printf("\n==============Input Video=============\n");
	av_dump_format(ifmt_ctx, 0, in_filename.c_str(), 0);
	printf("\n==============Output Video============\n");
	av_dump_format(ofmt_ctx_v, 0, out_filename_v.c_str(), 1);
	printf("\n==============Output Audio============\n");
	av_dump_format(ofmt_ctx_a, 0, out_filename_a.c_str(), 1);
	printf("\n======================================\n");

	//Open output file
	if (!(ofmt_v->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx_v->pb, out_filename_v.c_str(), AVIO_FLAG_WRITE) < 0) {
			printf( "Could not open output file '%s'", out_filename_v.c_str());
			goto end;
		}
	}

	if (!(ofmt_a->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx_a->pb, out_filename_a.c_str(), AVIO_FLAG_WRITE) < 0) {
			printf( "Could not open output file '%s'", out_filename_a.c_str());
			goto end;
		}
	}

	//Write file header
	if (avformat_write_header(ofmt_ctx_v, NULL) < 0) {
		printf( "Error occurred when opening video output file\n");
		goto end;
	}
	if (avformat_write_header(ofmt_ctx_a, NULL) < 0) {
		printf( "Error occurred when opening audio output file\n");
		goto end;
	}

#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
#endif

	while (1) {
		AVFormatContext *ofmt_ctx;
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		if (av_read_frame(ifmt_ctx, &pkt) < 0)
			break;
		in_stream  = ifmt_ctx->streams[pkt.stream_index];


		if(pkt.stream_index==videoindex){
			out_stream = ofmt_ctx_v->streams[0];
			ofmt_ctx=ofmt_ctx_v;
			printf("Write Video Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
#if USE_H264BSF
			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
		}else if(pkt.stream_index==audioindex){
			out_stream = ofmt_ctx_a->streams[0];
			ofmt_ctx=ofmt_ctx_a;
			printf("Write Audio Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
		}else{
			continue;
		}


		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index=0;
		//Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf( "Error muxing packet\n");
			break;
		}
		//printf("Write %8d frames to output file\n",frame_index);
		av_free_packet(&pkt);
		frame_index++;
	}

#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);  
#endif

	//Write file trailer
	av_write_trailer(ofmt_ctx_a);
	av_write_trailer(ofmt_ctx_v);

end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx_a->pb);

	if (ofmt_ctx_v && !(ofmt_v->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx_v->pb);

	avformat_free_context(ofmt_ctx_a);
	avformat_free_context(ofmt_ctx_v);


	if (ret < 0 && ret != AVERROR_EOF) {
		printf( "Error occurred.\n");
		return -1;
	}
	return 0;
}

