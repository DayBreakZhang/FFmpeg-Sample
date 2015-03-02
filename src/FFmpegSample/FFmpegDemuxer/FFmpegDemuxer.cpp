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

	return 0;
}

