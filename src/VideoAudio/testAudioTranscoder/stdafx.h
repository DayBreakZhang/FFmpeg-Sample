// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <stdint.h>
using namespace std;

extern "C"
{
#include  	<libavutil/avutil.h>
#include  	<libavutil/attributes.h>
#include    <libavutil/opt.h>
#include    <libavutil/mathematics.h>
#include    <libavutil/imgutils.h>
#include    <libavutil/samplefmt.h>
#include    <libavformat/avformat.h>
#include    <libavcodec/avcodec.h>
#include    <libswscale/swscale.h>
#include    <libavutil/mathematics.h>
#include    <libswresample/swresample.h>
#include    <libavutil/channel_layout.h>
#include    <libavutil/common.h>
#include    <libavformat/avio.h>
#include    <libavutil/file.h>
#include    <libswresample/swresample.h>
};



// TODO: reference additional headers your program requires here
