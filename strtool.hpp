#include<string.h>
#include<string>
#include<vector>
int strseekpos(const char* src, int pos = 0,const char* finded) { // 失败返回-1
    /*
        在src串中寻找finded 出现的位置
    */
    std::string str(src);
    int idx = str.find(finded, pos);
    if(idx == std::string::npos)
        return -1;
    return idx;    
}

void lstrip(char* & str) {
    /*消除字符串的左空格, 传入引用,指向位置已经改变*/
    int idx = 0;
    while(str[idx] != '\0') {
        if(str[idx] != ' ') break;
        str[idx] = '\0';
        idx++;
    }
    str = str + idx;
}
void rstrip(char* & str) {
    int n = strlen(str);
    while(n) {
        if(str[n] != ' ') break;
        str[n] = '\0';
        n--; 
    }
}
void strip(char* & str) {
    /*
    消除两边空格
    */
    lstrip(str);
    rstrip(str);
}

bool startWith(const char * src, const char * Start) {
    int srclen = strlen(src);
    int stlen = strlen(Start);
    if(srclen < stlen) return false;
    int i = 0;
    while(i < stlen) {
        if(src[i] != Start[i]) return false;
        i++;
    }
    return true;
}
bool endWith(const char* src, const char* End) {
    int srclen = strlen(src);
    int endlen = strlen(End);
    if(srclen < endlen) return false;
    while(endlen) {
        if(src[endlen] != End[endlen]) return false;
        endlen--;
    }
    return true;
}
std:: vector<std::string> strsplit(const char* str, char sign) {
    std::vector<std::string> ret;
    int len = strlen(str);
    int last = -1;
    for(int i = 0; i < len; i++) {
        if(str[i] == sign) {
            std:: string tmp;
            for(int j = last;  j < i; j++) {
                tmp.push_back(str[j]);
            }
            if(tmp.size() > 0)
                ret.push_back(tmp);
            last = i;
        }
    }
    return ret;
}


