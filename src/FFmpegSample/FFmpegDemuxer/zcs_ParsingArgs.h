#pragma once
/* purpose @ ��������Ĳ���������ͨ��AddArgType����������Ϳ�����Ĳ���key���뵽�ж��б��� 
 *          ͨ��Parse�е�result��������أ����н����keyΪ�Ϸ���key��vecotrΪ�����б� 
 *          �����б�֧��ȥ������ǰ������ź�\�����ź�\��ת�� 
 * 
 *          ����Ϸ��ֶΣ� 
 *          ��ʽ               ʵ�ʴ洢ֵ 
 *          \\value\"            \value" 
 *          "\\\value\""         \value" 
 * 
 *          ע����� 
 *              1����������б��в����ָ��Կո����� 
 *              2��- ������ַ��ؼ��֣�--������ַ����ؼ��� 
 *              3���ؼ��ֲ����ظ����֣����̹ؼ��ֲ���ͬʱ�����ڲ����б������Parse��������ʾ�������� 
 * 
 *          �÷��� 
 *              ParsingArgs pa; 
 *              pa.AddArgType('l',"getlist", ParsingArgs::NO_VALUE); //NO_VALUE�ؼ��ֺ����в��� 
 *              pa.AddArgType('p',"getuser", ParsingArgs::MAYBE_VALUE); //MAYBE_VALUE �ؼ��ֺ�����йؼ��� 
 *              pa.AddArgType('o',"outFile", ParsingArgs::MUST_VALUE); // MUST_VALUE �ؼ��ֺ�����в��� 
 *              std::map<std::string, std::vector<std::string> > result; 
 *              int iRet = pa.Parse(tmpPara,result); //result������ؼ���Ϊkey�洢��ص�ֵ���� 
 * 
 * date    @ 2014.02.19 
 * author  @ zhangDayBreak
 * 
 */  
namespace DayBreak
{
	class ParsingArgs  
{  
public:  
    ParsingArgs();  
    ~ParsingArgs();  
    enum KeyFlag{ INVALID_KEY=-1, NO_VALUE, MAYBE_VALUE, MUST_VALUE};  
    /* pur @ ��ӽ��Ͳ�����һ�����������ǳ�������Ҳ��������д�Ķβ������̲���ֻ��Ϊ���ַ���longName��shortName����Ҫ��һ�� 
     * para @ shortName �̲�����,0Ϊ��Ҫ�̲��� 
     * para @ longName �������� ��NULLΪ��Ҫ������ 
     * para @ flag �Ƿ���Ҫ������0����Ҫ��1����Ҫ��2��Ҫ�ɲ�Ҫ 
     * return @ true ��ӳɹ���false���ʧ�� 
    */  
    bool AddArgType(const char shortName, const char * longName = NULL, KeyFlag flag=NO_VALUE);  
  
    /* pur @ ���ݲ������ͽ��ʹ�����ַ��� 
     * para @ paras ��Ҫ���͵��ַ��� 
     * para @ result ���ؽ�����Ľ�� 
     * para @ errPos �������ʱ�򷵻س���Ĵ��λ�� 
     * return @ 0 ���ͳɹ������� ����ʧ�� 
     *          -1 δ֪�������� 
                -2 �����в�����ѡ���в������� 
     *          -3 ���в���ѡ���û�и����� 
     *          -4 �ؼ���û�м��뵽AddArgType�� 
     *          -5 �ؼ����ظ� 
    */  
    int Parse(const std::string & paras, std::map<std::string, std::vector<std::string> > & result, std::string &errPos);  
  
private:  
    /* pur @ �ж�����Ĳ����Ƿ����Ѿ���ӵĲ�������,�������ȥ��-��--,������ 
     * para @ key Ҫ�ж��Ĳ��� 
     * return @ -1 ���ǺϷ��������� ���򷵻�Option�е�flag 
    */  
    KeyFlag GetKeyFlag(std::string &key);  
  
    /* pur @ ɾ���ؼ���ǰ��-��-- 
    */  
    void RemoveKeyFlag(std::string & paras);  
  
    /* pur @ ��Paras�л�ȡһ�����ʣ��Զ����˵�����ǰ�����ţ���ʵ��\�Կո�����ŵ�ת�� 
     * para @ Paras ���ص�һ�����ʺ���������� 
     * para @ word ���ص�һ���� 
     * return @ �ɹ�����true��falseʧ�� 
     */  
    bool GetWord(std::string & Paras, std::string & word);  
  
    /* pur @ ���ؼ����Ƿ��ظ� 
     * para @ key �����Ĺؼ��� 
     * para @  result�Ѵ洢�Ĺؼ������� 
     * return @ true ���ظ��ģ�false���ظ� 
    */  
    bool IsDuplicateKey(const std::string &key, const std::map<std::string, std::vector<std::string> > & result);  
  
    struct Option  
    {  
        std::string m_longName;  
        char m_shortName;  
        KeyFlag m_flag;  
    };  
  
    std::vector<Option> m_args; //������Ϣ�б�  
};  
}