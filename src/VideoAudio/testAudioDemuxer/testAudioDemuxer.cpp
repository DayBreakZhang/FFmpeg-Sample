// testAudioDemuxer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main(int argc, char **argv)
{
	if(argc !=3)
	{
		fprintf(stderr, "Usage: %s input_file output_AudioFile\n",argv[0]);
		exit(1);
	}
	//��� ��Ƶ ��Ƶ ��ʽ ����
	AVOutputFormat *ofmt_a=NULL;
	//һ������ ������� �ĸ�ʽ������
	AVFormatContext *ifmt_ctx=NULL,*ofmt_ctx_a=NULL;
	//ѹ������
	AVPacket pkt;
	int ret,i;
	//��Ƶ��Ƶ����
	int audioindex=-1;
	int frame_index=0;

	//��ֵ���룬��� �ļ���
	const char* in_filename = argv[1];
	const char* out_filename_a = argv[2];

	//ע�������
	av_register_all();
	//��ȡ�����ļ���������
	ret = avformat_open_input(&ifmt_ctx,in_filename,0,0);
	if(ret < 0)
	{
		printf("Could not open input file.");
		goto end;
	}
	//��������ý������Ϣ
	ret = avformat_find_stream_info(ifmt_ctx,0);
	if(ret <0)
	{
		printf( "Failed to retrieve input stream information");  
		goto end;
	}
	//�������Ƶ�ļ�������
	//�������Ƶ��ʽ
	avformat_alloc_output_context2(&ofmt_ctx_a,NULL,NULL,out_filename_a);
	if(!ofmt_ctx_a)
	{
		printf( "Could not create output context\n");  
		ret = AVERROR_UNKNOWN;  
		goto end;  
	}
	//����Ƶ��ʽ��ֵ�������Ƶ����
	ofmt_a = ofmt_ctx_a->oformat;
	//������Ƶ��Ƶ��������
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {  
		//�����������������
		//��������������ģ���Ƶ��Ƶ����һ��ģ�
		AVFormatContext *ofmt_ctx; 
		//���������ļ����ļ���������ֵ
		AVStream *in_stream = ifmt_ctx->streams[i]; 
		//���������������ֵ
		AVStream *out_stream = NULL;  
		//�ж��������ǲ�����Ƶ
		if(ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){  //�ж��ǲ�����Ƶ
			audioindex=i;  //����Ƶ���������Ƹ���������
			out_stream=avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);  //��������Ƶ���뷽ʽ����һ����Ƶ��
			ofmt_ctx=ofmt_ctx_a;  //����Ƶ�������ĸ��Ƹ���������ʱ��������������
		}else{  
			continue;;  
		}  
		//�ж���Ƶ������ ��Ƶ����û�б���ʼ��
		if (!out_stream) {  
			printf( "Failed allocating output stream\n");  
			ret = AVERROR_UNKNOWN;  
			goto end;  
		}  
		//Copy the settings of AVCodecContext  �������������������������Ϣ
		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {  
			printf( "Failed to copy context from input to output stream codec context\n");  
			goto end;  
		}  
		out_stream->codec->codec_tag = 0;  
		//���ñ�ʾ
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)  
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;  
	}  
	//�ڽ����������Ƶ��Ϣ
	//Dump Format------------------  
	printf("\n==============Input Video=============\n");  
	av_dump_format(ifmt_ctx, 0, in_filename, 0);  
	printf("\n==============Output Audio============\n");  
	av_dump_format(ofmt_ctx_a, 0, out_filename_a, 1);  
	printf("\n======================================\n");  
	//Open output file ������ļ� 
	if (!(ofmt_a->flags & AVFMT_NOFILE)) {  
		if (avio_open(&ofmt_ctx_a->pb, out_filename_a, AVIO_FLAG_WRITE) < 0) {  
			printf( "Could not open output file '%s'", out_filename_a);  
			goto end;  
		}  
	}  

	//Write file header  д����ļ�ͷ
	if (avformat_write_header(ofmt_ctx_a, NULL) < 0) {  
		printf( "Error occurred when opening audio output file\n");  
		goto end;  
	}
	//��������Ƶ��ÿһ֡���� ���� �ֱ�д��ָ���ļ�
	while (1) {  
		AVFormatContext *ofmt_ctx;  
		AVStream *in_stream, *out_stream;  
		//Get an AVPacket  
		if (av_read_frame(ifmt_ctx, &pkt) < 0)  
			break;  
		in_stream  = ifmt_ctx->streams[pkt.stream_index];  


		if(pkt.stream_index==audioindex){  
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

	//Write file trailer  
	av_write_trailer(ofmt_ctx_a);  

//��������ͷ���Դ
end:  
	avformat_close_input(&ifmt_ctx);  
	/* close output */  
	if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))  
		avio_close(ofmt_ctx_a->pb);  

	avformat_free_context(ofmt_ctx_a);  

	if (ret < 0 && ret != AVERROR_EOF) {  
		printf( "Error occurred.\n");  
		return -1;  
	}  
	return 0; 
}

