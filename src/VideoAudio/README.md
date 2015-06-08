# src
工程的源文件

#更新日志

2015/6/7
1. Transcoder1
使用av_samples_alloc和av_samples_get_buffer_size对音频数据进行重新采样
2. Transcoder
使用AVFilter，AVFilterContext，AVFilterInOut，AVFilterGraph对视频音频进行转码
3. ResamplingAudio
使用SwrContext对数据进行重新采样
4. Demuxer
视频转码
5.AudioDemuxer
对音频进行抽取
