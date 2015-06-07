// testResamplingAudio.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
};

/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @example resampling_audio.c
 * libswresample API use example.
 */

#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>

/*
获取采样格式
*/
static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "Sample format %s not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return AVERROR(EINVAL);
}

/**
 * Fill dst buffer with nb_samples, generated starting from t.
 *使用nb_samples 填充dst buffer，确保从t开始
 *在440hz的曲线上以sample_rate的频率取nb_samples个样本点存储在dst中，nb_channels通道数据都一样
 */
static void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t)
{
    int i, j;
    double tincr = 1.0 / sample_rate, *dstp = dst;//tincr是时间间隔
    const double c = 2 * M_PI * 44100;//频率440Hz
    /* generate sin tone with 440Hz frequency and duplicated channels */
    for (i = 0; i < nb_samples; i++) {
        *dstp = sin(c * *t);//得到曲线上的采样点
        for (j = 1; j < nb_channels; j++)
            dstp[j] = dstp[0];//每一个通道都与第一个通道写一样的值
        dstp += nb_channels;//顺序写满通道后移动采样点
        *t += tincr;//时间向后移动
    }
}

int main(int argc, char **argv)
{
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_SURROUND;
	//源文件布局为立体声，目的文件布局为立体环绕声
    int src_rate = 48000, dst_rate = 44100;//源文件及目的文件的码率
    uint8_t **src_data = NULL, **dst_data = NULL;//源文件及目的文件数据初始化为空
    int src_nb_channels = 0, dst_nb_channels = 0;//源文件及目的文件通道数初始化为0
    int src_linesize, dst_linesize;//源文件及目的文件通道数据大小
    int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;//源文件及目的文件样品数
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_DBL, dst_sample_fmt = AV_SAMPLE_FMT_S16;
	//设置源文件及目的文件的样品格式,通过采样格式可知一个样品所占的字节数
    const char *src_filename = NULL,*dst_filename = NULL;//目的文件名字
    FILE *src_file,*dst_file;//目的文件指针
    int dst_bufsize;//目的文件缓存大小
    const char *fmt;
    struct SwrContext *swr_ctx;//对其成员变量不能直接操作，需使用avoption api操作
    double t;
    int ret;

	/*
	重采样音频帧以特定的格式并输出到目的文件中
	*/
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input_file output_file\n"
                "API example program to show how to resample an audio stream with libswresample.\n"
                "This program generates a series of audio frames, resamples them to a specified "
                "output format and rate and saves them to a input file named input_file and an output file named output_file.\n",
            argv[0]);
        exit(1);
    }
    src_filename = argv[1];
    dst_filename = argv[2];//赋值目的文件名字

//     src_file = fopen(src_filename, "wb");//以二进制写方式打开目的文件
//     if (!src_file) {
//         fprintf(stderr, "Could not open src file %s\n", src_filename);
//         exit(1);//打开失败退出
//     }
    dst_file = fopen(dst_filename, "wb");//以二进制写方式打开目的文件
    if (!dst_file) {
        fprintf(stderr, "Could not open destination file %s\n", dst_filename);
        exit(1);//打开失败退出
    }

    /* create resampler context 创建重采样上下文*/
    swr_ctx = swr_alloc();//为重采样上下文申请空间
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        ret = AVERROR(ENOMEM);//创建重采样上下文失败返回
        goto end;
    }

    /* set options 设置重采样上下文以avoption api方式(间接)*/
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);//设置输入源的通道布局
    av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);//设置输入源的采样率
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);//设置输入源的采样格式

    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);//设置输出源的通道布局
    av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);//设置输出源的采样率
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);//设置输出源的采样格式

    /* initialize the resampling context 调用swr_init生效*/
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        goto end;//初始化失败退出
    }

    /* allocate source and destination samples buffers 申请输入源、输出源样品缓存*/

    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);//得到输入源的通道数,数值为2
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,
                                             src_nb_samples, src_sample_fmt, 0);
    printf("src_linesize:%d\n",src_linesize);//16384:src_linesize=src_nb_samples*src_sample_fmt(size)*src_nb_channels=1024*8*2
	//为输入源申请采样空间
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        goto end;
    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples 计算输出源的样品数，要避免溢出*/
    max_dst_nb_samples = dst_nb_samples =
        av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);//输出源与输入源什么量是一定的关系式：时间一定
    printf("max_dst_nb_samples:%d\n",max_dst_nb_samples);
    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);//得到输出源的通道数：3
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    //为输出源申请空间dst_linesize=dst_nb_samples*2*3
    printf("dst_linesize:%d\n",dst_linesize);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        goto end;
    }

    t = 0;
    do {
        /* generate synthetic audio 生成合成音频作为输入源*/
        fill_samples((double *)src_data[0], src_nb_samples, src_nb_channels, src_rate, &t);

        /* compute destination number of samples 计算输出源的采样数*/
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +
                                        src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        //printf("dst_nb_samples:%d\n",dst_nb_samples);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);//如果计算所得的空间小于之前所申请的空间 ?
            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                                   dst_nb_samples, dst_sample_fmt, 1);//重新申请输出源空间
            if (ret < 0)
                break;
            max_dst_nb_samples = dst_nb_samples;//更新max_dst_nb_samples
        }

        /* convert to destination format 转换成目标格式*/
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            goto end;//转换失败，退出
        }
        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                 ret, dst_sample_fmt, 1);
        //得到输出源实际所需要的空间大小
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            goto end;
        }
        printf("t:%f in:%d out:%d\n", t, src_nb_samples, ret);
        //fwrite((double *)src_data[0], 1, src_linesize, src_file);
        fwrite(dst_data[0], 1, dst_bufsize, dst_file);//写入文件大小为dst_bufsize
    } while (t < 1);

    if ((ret = get_format_from_sample_fmt(&fmt, dst_sample_fmt)) < 0)
        goto end;
    fprintf(stderr, "Resampling succeeded. Play the output file with the command:\n"
            "ffplay -f %s -channel_layout %"PRId64" -channels %d -ar %d %s\n",
            fmt, dst_ch_layout, dst_nb_channels, dst_rate, dst_filename);

end:
//     if (src_file)
//         fclose(src_file);
    if (dst_file)
        fclose(dst_file);//关闭文件

    if (src_data)
        av_freep(&src_data[0]);//释放资源
    av_freep(&src_data);//释放资源

    if (dst_data)
        av_freep(&dst_data[0]);//释放资源
    av_freep(&dst_data);//释放资源

    swr_free(&swr_ctx);//释放重采样上下文资源
    return ret < 0;
}
