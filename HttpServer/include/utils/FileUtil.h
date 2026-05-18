#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <fstream>
#include <string>
#include <vector>

#include <muduo/base/Logging.h>

class FileUtil
{
public:
    FileUtil(std::string filePath):filePath_(filePath)
    ,file_(filePath,std::ios::binary){}

    ~FileUtil()
    {
        file_.close();
    }

    //判断是否是有效路径
    bool isValid() const{
        return file_.is_open();
    }

    //重置打开默认文件
    void resetDefaultFile()
    {
        file_.close();
        file_.open("/Gomoku/GomokuServer/resource/NotFound.html", std::ios::binary);
    }

    uint64_t size()
    {
        file_.seekg(0, std::ios::end); //定位到文件末尾
        uint64_t filesize = file_.tellg(); //获取文件大小
        file_.seekg(0, std::ios::beg); //定位到文件开头
        return filesize;
    }
    
    void readFile(std::vector<char>& buffer)
    {
        if (file_.read(buffer.data(), size()))
        {
            LOG_INFO << "File content load into memory (" << size() << " bytes)";
        }    
        else
        {
            LOG_ERROR << "File read failed";
        }
    }
private:
    std::string filePath_;
    std::ifstream file_;
};

#endif // FILEUTIL_H

