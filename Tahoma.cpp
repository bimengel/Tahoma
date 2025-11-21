#include "Tahoma.h"

char *pProgramPath;

int main(int argc, char *argv[])
{   
    int len, j;
    int action = 0;

    std::filesystem::path cwd = std::filesystem::current_path();
    string str = cwd.string() + "/";
    j = str.length();
    pProgramPath = new char[j+2];
    strcpy(pProgramPath, str.c_str());  
    len = argc;
    if(len == 1) // kein Argument
        action = 2; // Get Devices
    else
    {
        len = strlen(argv[1]);
        if(len == 1)
            action = atoi(argv[1]);
        if(action < 1 || action > 2)
        {   cout << "Falscher Parameter\n";
            exit(1); 
        }
    }       
    CTahoma *pTahoma = new CTahoma;
    pTahoma->Start(action);
    return 01;
}