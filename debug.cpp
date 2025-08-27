#include "debug.h"
#include <fstream> //file stream
#include <string>
void writeInfo(string str){
    static int flag = 0;
    //写到一个txt文件中，out方式打开，没有就自动创建，重名是直接覆盖！
	fstream file;
	if(flag == 0){
        file.open("test.txt", ios::out);
    }
    else{
        file.open("test.txt", ios::app);
    }
    flag++;
        
	file << str << endl;
	file.close();
}