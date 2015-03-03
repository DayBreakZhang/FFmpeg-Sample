#pragma once
namespace DayBreak
{
	class util
	{
	public:
		// 	static bool writeInit(QString path, QString root_key,QString user_key, QString user_value,QString def_value);//读ini文件
		// 	static bool readInit(QString path, QString root_key,QString user_key, QString &user_value,QString def_value);//写ini文件

		static void UTF_8ToUnicode(wchar_t* pOut,char *pText);  // 把UTF-8转换成Unicode
		static void UnicodeToUTF_8(char* pOut,wchar_t* pText);  //Unicode 转换成UTF-8
		static void UnicodeToGB2312(char* pOut,wchar_t uData);  // 把Unicode 转换成 GB2312
		static void Gb2312ToUnicode(wchar_t* pOut,char *gbBuffer);// GB2312 转换成　Unicode
		static void GB2312ToUTF_8(std::string& pOut,char *pText, int pLen);//GB2312 转为 UTF-8
		static void UTF_8ToGB2312(std::string &pOut, char *pText, int pLen);//UTF-8 转为 GB2312
		static WCHAR* AnsiToUnicode( const char* szStr );//char to wchar
		static char* UnicodeToAnsi( const WCHAR* szStr );//wchar to char
		static char* UnicodeToUTF8( const WCHAR* szStr );//wchar to utf8
		static wchar_t* Utf8ToUnicode(const char* utf);//utf8 to wchar
	};
}

