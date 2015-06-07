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
��ȡ������ʽ
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
 *ʹ��nb_samples ���dst buffer��ȷ����t��ʼ
 *��440hz����������sample_rate��Ƶ��ȡnb_samples��������洢��dst�У�nb_channelsͨ�����ݶ�һ��
 */
static void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t)
{
    int i, j;
    double tincr = 1.0 / sample_rate, *dstp = dst;//tincr��ʱ����
    const double c = 2 * M_PI * 44100;//Ƶ��440Hz
    /* generate sin tone with 440Hz frequency and duplicated channels */
    for (i = 0; i < nb_samples; i++) {
        *dstp = sin(c * *t);//�õ������ϵĲ�����
        for (j = 1; j < nb_channels; j++)
            dstp[j] = dstp[0];//ÿһ��ͨ�������һ��ͨ��дһ����ֵ
        dstp += nb_channels;//˳��д��ͨ�����ƶ�������
        *t += tincr;//ʱ������ƶ�
    }
}

int main(int argc, char **argv)
{
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_SURROUND;
	//Դ�ļ�����Ϊ��������Ŀ���ļ�����Ϊ���廷����
    int src_rate = 48000, dst_rate = 44100;//Դ�ļ���Ŀ���ļ�������
    uint8_t **src_data = NULL, **dst_data = NULL;//Դ�ļ���Ŀ���ļ����ݳ�ʼ��Ϊ��
    int src_nb_channels = 0, dst_nb_channels = 0;//Դ�ļ���Ŀ���ļ�ͨ������ʼ��Ϊ0
    int src_linesize, dst_linesize;//Դ�ļ���Ŀ���ļ�ͨ�����ݴ�С
    int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;//Դ�ļ���Ŀ���ļ���Ʒ��
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_DBL, dst_sample_fmt = AV_SAMPLE_FMT_S16;
	//����Դ�ļ���Ŀ���ļ�����Ʒ��ʽ,ͨ��������ʽ��֪һ����Ʒ��ռ���ֽ���
    const char *src_filename = NULL,*dst_filename = NULL;//Ŀ���ļ�����
    FILE *src_file,*dst_file;//Ŀ���ļ�ָ��
    int dst_bufsize;//Ŀ���ļ������С
    const char *fmt;
    struct SwrContext *swr_ctx;//�����Ա��������ֱ�Ӳ�������ʹ��avoption api����
    double t;
    int ret;

	/*
	�ز�����Ƶ֡���ض��ĸ�ʽ�������Ŀ���ļ���
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
    dst_filename = argv[2];//��ֵĿ���ļ�����

//     src_file = fopen(src_filename, "wb");//�Զ�����д��ʽ��Ŀ���ļ�
//     if (!src_file) {
//         fprintf(stderr, "Could not open src file %s\n", src_filename);
//         exit(1);//��ʧ���˳�
//     }
    dst_file = fopen(dst_filename, "wb");//�Զ�����д��ʽ��Ŀ���ļ�
    if (!dst_file) {
        fprintf(stderr, "Could not open destination file %s\n", dst_filename);
        exit(1);//��ʧ���˳�
    }

    /* create resampler context �����ز���������*/
    swr_ctx = swr_alloc();//Ϊ�ز�������������ռ�
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        ret = AVERROR(ENOMEM);//�����ز���������ʧ�ܷ���
        goto end;
    }

    /* set options �����ز�����������avoption api��ʽ(���)*/
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);//��������Դ��ͨ������
    av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);//��������Դ�Ĳ�����
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);//��������Դ�Ĳ�����ʽ

    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);//�������Դ��ͨ������
    av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);//�������Դ�Ĳ�����
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);//�������Դ�Ĳ�����ʽ

    /* initialize the resampling context ����swr_init��Ч*/
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        goto end;//��ʼ��ʧ���˳�
    }

    /* allocate source and destination samples buffers ��������Դ�����Դ��Ʒ����*/

    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);//�õ�����Դ��ͨ����,��ֵΪ2
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,
                                             src_nb_samples, src_sample_fmt, 0);
    printf("src_linesize:%d\n",src_linesize);//16384:src_linesize=src_nb_samples*src_sample_fmt(size)*src_nb_channels=1024*8*2
	//Ϊ����Դ��������ռ�
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        goto end;
    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples �������Դ����Ʒ����Ҫ�������*/
    max_dst_nb_samples = dst_nb_samples =
        av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);//���Դ������Դʲô����һ���Ĺ�ϵʽ��ʱ��һ��
    printf("max_dst_nb_samples:%d\n",max_dst_nb_samples);
    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);//�õ����Դ��ͨ������3
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    //Ϊ���Դ����ռ�dst_linesize=dst_nb_samples*2*3
    printf("dst_linesize:%d\n",dst_linesize);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        goto end;
    }

    t = 0;
    do {
        /* generate synthetic audio ���ɺϳ���Ƶ��Ϊ����Դ*/
        fill_samples((double *)src_data[0], src_nb_samples, src_nb_channels, src_rate, &t);

        /* compute destination number of samples �������Դ�Ĳ�����*/
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +
                                        src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        //printf("dst_nb_samples:%d\n",dst_nb_samples);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);//����������õĿռ�С��֮ǰ������Ŀռ� ?
            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                                   dst_nb_samples, dst_sample_fmt, 1);//�����������Դ�ռ�
            if (ret < 0)
                break;
            max_dst_nb_samples = dst_nb_samples;//����max_dst_nb_samples
        }

        /* convert to destination format ת����Ŀ���ʽ*/
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            goto end;//ת��ʧ�ܣ��˳�
        }
        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                 ret, dst_sample_fmt, 1);
        //�õ����Դʵ������Ҫ�Ŀռ��С
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            goto end;
        }
        printf("t:%f in:%d out:%d\n", t, src_nb_samples, ret);
        //fwrite((double *)src_data[0], 1, src_linesize, src_file);
        fwrite(dst_data[0], 1, dst_bufsize, dst_file);//д���ļ���СΪdst_bufsize
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
        fclose(dst_file);//�ر��ļ�

    if (src_data)
        av_freep(&src_data[0]);//�ͷ���Դ
    av_freep(&src_data);//�ͷ���Դ

    if (dst_data)
        av_freep(&dst_data[0]);//�ͷ���Դ
    av_freep(&dst_data);//�ͷ���Դ

    swr_free(&swr_ctx);//�ͷ��ز�����������Դ
    return ret < 0;
}
