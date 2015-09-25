// testTranscodertobmp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
}

#ifndef _WINGDI_    
#define _WINGDI_   
typedef struct tagBITMAPFILEHEADER {    
	WORD    bfType;    
	DWORD   bfSize;    
	WORD    bfReserved1;    
	WORD    bfReserved2;    
	DWORD   bfOffBits;    
} BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;    

typedef struct tagBITMAPINFOHEADER{    
	DWORD      biSize;    
	LONG       biWidth;    
	LONG       biHeight;    
	WORD       biPlanes;    
	WORD       biBitCount;    
	DWORD      biCompression;    
	DWORD      biSizeImage;    
	LONG       biXPelsPerMeter;    
	LONG       biYPelsPerMeter;    
	DWORD      biClrUsed;    
	DWORD      biClrImportant;    
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;    

#endif  
//保存BMP文件的函数
void SaveAsBMP (AVFrame *pFrameRGB, int width, int height, int index, int bpp) 
{ 
	char buf[5] = {0}; 
	BITMAPFILEHEADER bmpheader; 
	BITMAPINFOHEADER bmpinfo; 
	FILE *fp; 

	char *filename = new char[255];
	//文件存放路径，根据自己的修改
	sprintf_s(filename,255,"%s%d.bmp","e:\\mp4\\",index);
	if ( (fp=fopen(filename,"wb+")) == NULL ) 
	{ 
		printf ("open file failed!\n"); 
		return; 
	} 

	bmpheader.bfType = 0x4d42; 
	bmpheader.bfReserved1 = 0; 
	bmpheader.bfReserved2 = 0; 
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); 
	bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp/8; 

	bmpinfo.biSize = sizeof(BITMAPINFOHEADER); 
	bmpinfo.biWidth = width; 
	bmpinfo.biHeight = height; 
	bmpinfo.biPlanes = 1; 
	bmpinfo.biBitCount = bpp; 
	bmpinfo.biCompression = BI_RGB; 
	bmpinfo.biSizeImage = (width*bpp+31)/32*4*height; 
	bmpinfo.biXPelsPerMeter = 100; 
	bmpinfo.biYPelsPerMeter = 100; 
	bmpinfo.biClrUsed = 0; 
	bmpinfo.biClrImportant = 0; 

	fwrite (&bmpheader, sizeof(bmpheader), 1, fp); 
	fwrite (&bmpinfo, sizeof(bmpinfo), 1, fp); 
	fwrite (pFrameRGB->data[0], width*height*bpp/8, 1, fp); 

	fclose(fp); 
} 

//主函数
int main (void) 
{ 
	unsigned int i = 0, videoStream = -1; 
	AVCodecContext *pCodecCtx; 
	AVFormatContext *pFormatCtx=NULL; 
	AVCodec *pCodec; 
	AVFrame *pFrame, *pFrameRGB; 
	struct SwsContext *pSwsCtx; 
	const char *filename = "e:\\mp4\\61922.mp4"; 
	AVPacket packet; 
	int frameFinished; 
	int PictureSize; 
	uint8_t *buf; 
	//注册编解码器
	av_register_all(); 
	//打开视频文件
	if ( avformat_open_input(&pFormatCtx, filename, 0, 0) < 0 ) 
	{ 
		printf ("av open input file failed!\n"); 
		exit (1); 
	} 
	//获取流信息
	if ( avformat_find_stream_info(pFormatCtx,0) < 0 ) 
	{ 
		printf ("av find stream info failed!\n"); 
		exit (1); 
	} 
	//获取视频流
	for ( i=0; i<pFormatCtx->nb_streams; i++ ) 
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) 
		{ 
			videoStream = i; 
			break; 
		} 

		if (videoStream == -1) 
		{ 
			printf ("find video stream failed!\n"); 
			exit (1); 
		} 

		pCodecCtx = pFormatCtx->streams[videoStream]->codec; 

		pCodec = avcodec_find_decoder (pCodecCtx->codec_id); 

		if (pCodec == NULL) 
		{ 
			printf ("avcode find decoder failed!\n"); 
			exit (1); 
		} 
		//打开解码器
		if ( avcodec_open2(pCodecCtx, pCodec,0)<0 ) 
		{ 
			printf ("avcode open failed!\n"); 
			exit (1); 
		} 

		//为每帧图像分配内存
		pFrame = avcodec_alloc_frame(); 
		pFrameRGB = avcodec_alloc_frame(); 

		if ( (pFrame==NULL)||(pFrameRGB==NULL) ) 
		{ 
			printf("avcodec alloc frame failed!\n"); 
			exit (1); 
		} 

		PictureSize = avpicture_get_size (PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height); 
		buf = (uint8_t*)av_malloc(PictureSize); 

		if ( buf == NULL ) 
		{ 
			printf( "av malloc failed!\n"); 
			exit(1); 
		} 
		avpicture_fill ( (AVPicture *)pFrameRGB, buf, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height); 

		//设置图像转换上下文
		pSwsCtx = sws_getContext (pCodecCtx->width, 
			pCodecCtx->height, 
			pCodecCtx->pix_fmt, 
			pCodecCtx->width, 
			pCodecCtx->height, 
			PIX_FMT_BGR24, 
			SWS_BICUBIC, 
			NULL, NULL, NULL); 
		i = 0; 
		while(av_read_frame(pFormatCtx, &packet) >= 0) 
		{ 
			if(packet.stream_index==videoStream) 
			{ 
				avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
					&packet); 

				if(frameFinished) 
				{    
					//反转图像 ，否则生成的图像是上下调到的
					pFrame->data[0] += pFrame->linesize[0] * (pCodecCtx->height - 1); 
					pFrame->linesize[0] *= -1; 
					pFrame->data[1] += pFrame->linesize[1] * (pCodecCtx->height / 2 - 1); 
					pFrame->linesize[1] *= -1; 
					pFrame->data[2] += pFrame->linesize[2] * (pCodecCtx->height / 2 - 1); 
					pFrame->linesize[2] *= -1; 
					//转换图像格式，将解压出来的YUV420P的图像转换为BRG24的图像
					sws_scale (pSwsCtx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize); 
					SaveAsBMP (pFrameRGB, pCodecCtx->width, pCodecCtx->height, i++, 24); 
				}     
			} 
			av_free_packet(&packet); 
		} 

		sws_freeContext (pSwsCtx); 
		av_free (pFrame); 
		av_free (pFrameRGB); 
		avcodec_close (pCodecCtx); 
		avformat_close_input(&pFormatCtx); 

		return 0; 
} 
