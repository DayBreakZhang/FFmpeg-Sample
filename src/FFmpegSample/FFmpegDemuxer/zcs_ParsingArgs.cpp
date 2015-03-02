#include "StdAfx.h"
#include "zcs_ParsingArgs.h"

bool DayBreak::ParsingArgs::AddArgType(const char shortName, const char * longName /*= NULL*/, KeyFlag flag/*=NO_VALUE*/)
{
	if(NULL == longName && 0 == shortName)  
	{  
		return false;  
	}  
	Option tmp;  
	tmp.m_longName = longName;  
	tmp.m_shortName = shortName;  
	tmp.m_flag = flag;  
	m_args.push_back(tmp);  
	return true;  
}

int DayBreak::ParsingArgs::Parse(const std::string & ps, std::map<std::string, std::vector<std::string> > & result, std::string &errPos)
{
	std::string tmpString = ps;  
	KeyFlag keyFlag = INVALID_KEY; //������ʶ  
	std::string sKey = ""; //key�ؼ���  
	bool bFindValue = false; //�Ƿ��Ѿ���value  
	while(!tmpString.empty())  
	{  
		std::string word = "";  

		bool bRet = GetWord(tmpString, word);  

		if(bRet == false)  
		{  
			errPos = tmpString;  
			return -4;//������������  
		}  
		else  
		{  
			KeyFlag tmpFlag = GetKeyFlag(word);  
			if(IsDuplicateKey(word, result))  
			{  
				errPos = tmpString;  
				return -5;  
			}  
			if(tmpFlag != INVALID_KEY)  
			{  
				if(tmpFlag == MUST_VALUE && keyFlag == MUST_VALUE && !bFindValue)  
				{  
					errPos = tmpString;  
					return -3;  
				}  
				keyFlag = tmpFlag;  
				std::vector<std::string> tmp;  
				result[word] = tmp;  
				sKey = word;  
				bFindValue = false;  
			}  
			else  
			{  
				switch(keyFlag)  
				{  
				case MAYBE_VALUE:  
				case MUST_VALUE:  
					{  
						std::map<std::string, std::vector<std::string> >::iterator it = result.find(sKey);  
						if(it != result.end())  
						{  
							it->second.push_back(word);  
							bFindValue = true;  
						}  
						else  
						{  
							errPos = tmpString;  
							return -1;// û�з�����ص�key  
						}  
					}  
					break;  
				case NO_VALUE:  
					errPos = tmpString;  
					return -2; //�����в�����ѡ����в���  
				default:  
					errPos = tmpString;  
					return -1;//��������  
				}//switch end  
			}  
		}  
	}//while end  
	return 0;  
}

DayBreak::ParsingArgs::KeyFlag DayBreak::ParsingArgs::GetKeyFlag(std::string &key)
{
	for(int i=0; i<m_args.size(); ++i)  
	{  
		std::string shortName = "-";  
		std::string longName = "--";  
		shortName += m_args[i].m_shortName;  
		longName += m_args[i].m_longName;  
		if( 0 == key.compare(shortName) ||  
			(0==key.compare(longName))  
			)  
		{  
			RemoveKeyFlag(key);  
			return m_args[i].m_flag;  
		}  
	}  
	return INVALID_KEY;  
}

void DayBreak::ParsingArgs::RemoveKeyFlag(std::string & word)
{
	if(word.size()>=2)  
	{  
		if(word[1] == '-')  
		{  
			word.erase(1,1);  
		}  
		if(word[0] == '-')  
		{  
			word.erase(0,1);  
		}  
	}  
}
/* pur @ ��Paras�л�ȡһ�����ʣ��Զ����˵�����ǰ�����ţ���ʵ��\�Կո�����ŵ�ת�� 
 * para @ Paras ���ص�һ�����ʺ���������� 
 * para @ word ���ص�һ���� 
 * return @ �ɹ�����true��falseʧ�� 
*/ 
bool DayBreak::ParsingArgs::GetWord(std::string & Paras, std::string & word)
{
	size_t iNotSpacePos = Paras.find_first_not_of(' ',0);//���ҵ�һ���ǿո��ַ�λ��  
	if(iNotSpacePos == std::string::npos)  
	{  
		Paras.clear();  
		word.clear();  
		return true;  
	}  
	int length = Paras.size();  
	std::list<char> specialChar;  
	int islashPos = -1;  
	for(int i=iNotSpacePos; i<length; i++)  
	{  
		char cur=Paras[i];  
		bool bOk = false;  
		switch(cur)  
		{  
		case ' ':  
			if(specialChar.empty())  
			{  
				if(i!=(length-1))  
				{  
					Paras = std::string(Paras, i+1, length-i-1);  
				}  
				else  
				{//���һ���ǿո�  
					Paras.clear();  
				}  
				bOk = true;  
			}  
			else  
			{                      
				if(specialChar.back() == '\\')  
				{  
					specialChar.pop_back();  
				}  
				word.append(1,cur);  
			}  
			break;  
		case '"':  
			if(specialChar.empty())  
			{  
				specialChar.push_back(cur);  
			}  
			else if(specialChar.back() == cur)  
			{ //�ҵ�ƥ�������  
				specialChar.pop_back();  
			}  
			else if(specialChar.back() == '\\')  
			{  
				word.append(1,cur);  
				specialChar.pop_back();  
			}  
			else  
			{  
				word.clear();  
				return false;  
			}  
			break;  
		case '\\':  
			if(specialChar.empty())  
			{  
				specialChar.push_back(cur);  
				islashPos = i;  
			}  
			else if(specialChar.back() == '"')  
			{  
				if(i<(length-1))  
				{  
					if('"'==Paras[i+1] || '\\'==Paras[i+1])  
					{  
						specialChar.push_back(cur);  
					}  
					else  
					{  
						word.append(1,cur);  
					}  
				}  
				else  
				{  
					word.clear();  
					return false;  
				}  
			}  
			else if('\\' == specialChar.back())  
			{  
				word.append(1,cur);  
				specialChar.pop_back();  
			}  
			else   
			{  
				word.clear();  
				return false;  
			}  
			break;  
		default:  
			word.append(1,Paras[i]);  
			if(i==(length-1))  
			{  
				bOk = true;  
				Paras.clear();  
			}  
			break;  
		}  
		if(bOk)   
		{  
			return true;  
		}  
	}//for end  
	if(specialChar.empty())  
	{  
		Paras.clear();  
		return true;  
	}  
	else  
	{  
		return false;  
	}  
}

bool DayBreak::ParsingArgs::IsDuplicateKey(const std::string &key, const std::map<std::string, std::vector<std::string> > & result)
{
	if(result.find(key) != result.end())  
	{  
		return true; //�ؼ����ظ�  
	}  

	for(int i=0; i<m_args.size(); ++i)  
	{  
		if( (key.compare(m_args[i].m_longName) == 0 && result.find(std::string(1, m_args[i].m_shortName)) != result.end())  
			|| (key.compare(std::string(1, m_args[i].m_shortName)) == 0 && result.find(m_args[i].m_longName) != result.end())  
			)  
		{  
			return true;  
		}  
	}  
	return false;  
}

DayBreak::ParsingArgs::ParsingArgs()
{

}

DayBreak::ParsingArgs::~ParsingArgs()
{

}
