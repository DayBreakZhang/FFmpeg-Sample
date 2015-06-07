// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <iostream>

using namespace std;

extern "C"
{
#include "libavformat/avformat.h"
};

#define  USE_H264BSF 0

// TODO: reference additional headers your program requires here
