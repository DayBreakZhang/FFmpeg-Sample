// testDemuxer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main(int argc, char **argv)
{
	//判断输入参数
	if(argc !=4)
	{
		fprintf(stderr, "Usage: %s input_file output_VideoFile output_AudioFile\n",argv[0]);
		exit(1);
	}
	//输出 视频 音频 格式 容器
	AVOutputFormat *ofmt_a=NULL,*ofmt_v=NULL;
	//一个输入 两个输出 的格式上下文
	AVFormatContext *ifmt_ctx=NULL,*ofmt_ctx_a=NULL,*ofmt_ctx_v=NULL;
	//压缩数据
	AVPacket pkt;
	int ret,i;
	//音频视频索引
	int videoindex =-1,audioindex=-1;
	int frame_index=0;

	//赋值输入，输出 文件名
	const char* in_filename = argv[1];
	const char* out_filename_v = argv[2];
	const char* out_filename_a = argv[3];

	//注册解码器
	av_register_all();
	//获取输入文件的上下文
	ret = avformat_open_input(&ifmt_ctx,in_filename,0,0);
	if(ret < 0)
	{
		printf("Could not open input file.");
		goto end;
	}
	//查找输入媒体流信息
	ret = avformat_find_stream_info(ifmt_ctx,0);
	if(ret <0)
	{
		printf( "Failed to retrieve input stream information");  
		goto end;
	}
	//打开输出视频文件上下文
	avformat_alloc_output_context2(&ofmt_ctx_v,NULL,NULL,out_filename_v);
	if(!ofmt_ctx_v)
	{
		printf( "Could not create output context\n");  
		ret = AVERROR_UNKNOWN;  
		goto end; 
	}
	//将输入视频格式赋值给输出容器
	ofmt_v = ofmt_ctx_v->oformat;

	//打开输出音频格式
	avformat_alloc_output_context2(&ofmt_ctx_a,NULL,NULL,out_filename_a);
	if(!ofmt_ctx_a)
	{
		printf( "Could not create output context\n");  
		ret = AVERROR_UNKNOWN;  
		goto end;  
	}
	//将音频格式赋值给输出音频容器
	ofmt_a = ofmt_ctx_a->oformat;
	//查找视频音频流的索引
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {  
		//创建输入流，输出流
		//声明输出的上下文（音频视频合在一起的）
		AVFormatContext *ofmt_ctx; 
		//创建输入文件的文件流，并赋值
		AVStream *in_stream = ifmt_ctx->streams[i]; 
		//创建输出流，并赋值
		AVStream *out_stream = NULL;  
		//判断输入流是不是视频
		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
			videoindex=i;  //将视频流索引赋值给索引变量
			out_stream=avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);//以输入视频编码方式创建一个视频流  
			ofmt_ctx=ofmt_ctx_v;  //将视频的上下文复制给函数中临时变量做其它操作
		}else if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){  //判断是不是音频
			audioindex=i;  //将音频索引流复制给索引变量
			out_stream=avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);  //以输入音频编码方式创建一个音频流
			ofmt_ctx=ofmt_ctx_a;  //将音频的上下文复制给函数中临时变量做其它推行
		}else{  
			break;  
		}  
		//判断视频流或者 音频流有没有被初始化
		if (!out_stream) {  
			printf( "Failed allocating output stream\n");  
			ret = AVERROR_UNKNOWN;  
			goto end;  
		}  
		//Copy the settings of AVCodecContext  拷贝输入流到输出流，配置信息
		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {  
			printf( "Failed to copy context from input to output stream codec context\n");  
			goto end;  
		}  
		out_stream->codec->codec_tag = 0;  
		//设置表示
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)  
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;  
	}  
	//在界面中输出视频信息
	//Dump Format------------------  
	printf("\n==============Input Video=============\n");  
	av_dump_format(ifmt_ctx, 0, in_filename, 0);  
	printf("\n==============Output Video============\n");  
	av_dump_format(ofmt_ctx_v, 0, out_filename_v, 1);  
	printf("\n==============Output Audio============\n");  
	av_dump_format(ofmt_ctx_a, 0, out_filename_a, 1);  
	printf("\n======================================\n");  
	//Open output file 打开输出文件 
	if (!(ofmt_v->flags & AVFMT_NOFILE)) {  
		if (avio_open(&ofmt_ctx_v->pb, out_filename_v, AVIO_FLAG_WRITE) < 0) {  
			printf( "Could not open output file '%s'", out_filename_v);  
			goto end;  
		}  
	}  

	if (!(ofmt_a->flags & AVFMT_NOFILE)) {  
		if (avio_open(&ofmt_ctx_a->pb, out_filename_a, AVIO_FLAG_WRITE) < 0) {  
			printf( "Could not open output file '%s'", out_filename_a);  
			goto end;  
		}  
	}  

	//Write file header  写输出文件头
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
	//将输入视频的每一帧解码 编码 分别写入指定文件
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


//错误处理和释放资源
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

