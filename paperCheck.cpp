#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <ctype.h>
#include <list>
#include <algorithm>

/**
 * 论文查重demo
 *
 * @author xfduanc
 * @date 2021/7/31
 */
using namespace std;
typedef ostringstream fstring;
typedef vector<string> vec_str;
typedef  ::std::string string;

vec_str split(const string & s, const string& delimiters);
bool is_blank(const string & str);


class ReadDir
{
    const int MAX_PATH;
    bool recur;
    vec_str ret;
    public:
    ReadDir(bool recur = false): MAX_PATH(1024){this->recur = recur;}
    vec_str getAllFile(const string &dir =".")
    {
        fsize(dir);
        return ret;
    }
    void fsize(const string& name)
    {
        struct stat stbuf;
        if (stat(name.data(), &stbuf) == -1) {
            fprintf(stderr, "fsize: can't access %s\n", name.data());
            return;
        }
        if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
            dirwalk(name);
    }
    void dirwalk(const string& dir)
    {
        char name[MAX_PATH];
        struct dirent *dp;
        DIR *dfd;

        if ((dfd = opendir(dir.data())) == NULL) {
            fprintf(stderr, "dirwalk: can't open %s\n", dir.data());
            return;
        }
        while ((dp = readdir(dfd)) != NULL) {
            if (strcmp(dp->d_name, ".") == 0
                || strcmp(dp->d_name, "..") == 0)
                continue;    /* skip self and parent */
            if ( dir.size()+strlen(dp->d_name)+2 > sizeof(name))
                fprintf(stderr, "dirwalk: name %s %s too long\n",
                    dir.data(), dp->d_name);
            else {
                fstring fstr;
                fstr<<dir<<"/"<<dp->d_name;
                ret.emplace_back(fstr.str());
                if( this->recur)
                    fsize(name);
            }
        }
        closedir(dfd);
    }
};
class Demo {
    private:
    typedef struct Sentence
    {
        string text;
        string title;
    }Sentence;

    public:
    list<Sentence> preload(const string& dir) {
        ReadDir readdir;
        vec_str ret =readdir.getAllFile(dir);
        vec_str all_sentence;
        map<string, vec_str> file2sentence;
        //对于每一个文件
        for(auto a : ret)
        {
            ifstream in(a);
            string readline;
            while(std::getline(in, readline))
                if(!is_blank(readline))
                    all_sentence.emplace_back( readline );
            in.close();
            vec_str sentences,line_sentence;

            for( auto a: all_sentence)
            {
                line_sentence = split(a, "，。、？,.");
                for(auto b : line_sentence)
                {
                    sentences.emplace_back(b);
                }
            }
            //get the map<fileName, sentences>
            file2sentence.emplace(a.substr(0, a.find_last_of(".")),  sentences);
        }

        list<Sentence> result;

        ::std::for_each( file2sentence.begin(), file2sentence.end(), [&result](std::pair<string, vec_str> a)mutable
        {
            ::std::for_each( a.second.begin(), a.second.end(), [=,&result](string str)mutable{
                result.emplace_back(Sentence{str, a.first});
            });
        });
        return result;
    }

    map<string, list<Sentence>> buildInvertIndex(list<Sentence> sentences){
        map<string, list<Sentence>> result;
        for (Sentence sentence : sentences) {
            if(sentence.text.size() < 4){
                continue;
            }
            for (int i=0; i<=sentence.text.length()-4; i++){
                result[sentence.text.substr(i, i+4)].emplace_back(sentence);
            }
        }
        return result;
    }


    list<set<Sentence, bool(*)(const Sentence& lhs, const Sentence& rhs)>> compare(list<string> subSentence, map<string, list<Sentence>> invertIndex){
        list<set<Sentence, bool(*)(const Sentence& lhs, const Sentence& rhs)>> result;
        for (auto s : subSentence) {
            if(s.length() < 7){
                result.emplace_back();//保证顺序
                continue;
            }
            auto cmp = [](const Sentence& lhs, const Sentence& rhs)->bool{return lhs.text<rhs.text;};
            set<Sentence, bool(*)(const Sentence& lhs, const Sentence& rhs)> subset(cmp);
            for(int i=0; i<=s.length()-4;i++){
                for (Sentence sentence : invertIndex[s.substr(i, i + 4)]) {
                    if(lcs(sentence.text, s)>=7){
                        subset.emplace(sentence);
                    }
                }
            }
            result.emplace_back(subset);
        }
        return result;
    }

    int lcs(string &s1, string &s2){
        
        vector< vector<int> >dp(s2.size()+1, vector<int>(s1.size()+1));
        for(int i=1; i<=s2.length(); i++){
            for(int j=1; j<=s1.length(); j++){
                if(s2.at(i-1) == s1.at(j-1)){
                    dp[i][j] = dp[i-1][j-1]+1;
                }else{
                    dp[i][j] = ::std::max(dp[i-1][j], dp[i][j-1]);
                }
            }
        }
        return dp[s2.size()][s1.size()];
    }

    list<string> readInput(const string &filename){
        list<string> ret, subSentence;
        ifstream in(filename);
        string readline;
        while(std::getline(in, readline))
            if(!is_blank(readline))
            subSentence.emplace_back( readline );
        in.close();

        for( auto a: subSentence)
        {
            auto line_sentence = split(a, "，。、？,.");
            for(auto b : line_sentence)
            {
                ret.emplace_back(b);
            }
        }
        return ret;
    }

    public:
    void run(const string&filename, const string &dir){
        list<Sentence> sentences = preload(dir);
        map<string, list<Sentence>> invertIndex = buildInvertIndex(sentences);


        list<string> input = readInput(filename);

        auto result = compare(input, invertIndex);
        auto iter =input.begin();
        for (auto i : result) {
            if(!i.empty()){
                for (Sentence sentence : i) {
                    std::cout<<"文中【{"<<*iter<<"}】与库中《{"<<sentence.title<<"}》的【{"<<sentence.text<<"}】相似"<<std::endl;
                }
            }
            iter++;
        }


    }
};

int main()
{
    Demo demo;
    // auto file2sentence = demo.preload("GP");
    demo.run("test.cpp", "MP");
    return 0;
}

vec_str split(const string & s, const string& delimiters)
{
    vec_str ret;
    size_t current;
    size_t next = -1;
    do
    {
        current = next + 1;
        next = s.find_first_of( delimiters, current );
        string tmp =s.substr( current, next - current );
        if(!tmp.empty() )
            ret.emplace_back( tmp );
    }
    while (next != string::npos);
    return ret;
}

bool is_blank(const string & str)
{
    for(auto a: str)
        if(!::isblank(a))
        return false;
    return true;
}