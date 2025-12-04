#include "Tahoma.h"

CTahoma::CTahoma()
{
    m_strPORT = "8443";
}

CTahoma::~CTahoma()
{
    curl_easy_cleanup(m_pCurl);
}

int CTahoma::Start(int action)
{
    int iDef, i, iRet = 0;

    iDef = ReadConfig();
    switch(action) {
    case 1: // Generate and activate Token
        i = iDef & 0b1101;  
        if(i == 0b1101)
            iRet = GetSessionID();
        else
        {   if(!(i & 0b1))
                cout << "SOMFYPIN nicht definiert in Tahoma.config" << endl;
            if(!(i & 0b100))
                cout << "SOMYUSER nicht definiert in Tahoma.config" << endl;
            if(!(i & 0b1000))
                cout << "SOMYPWD nicht definiert in Tahoma.config" << endl;                  
            return iRet;
        }
        if(iRet)
        {
            iRet = GenerateToken();
            if(iRet)
            {   iRet = ActivateToken();
                if(iRet)
                {
                    WriteToTahomaConfig();
                    iRet = GetDevices();
                }
            }
        }
        break;
    case 2: // Get devices
        i = iDef & 0b11;
        if (i == 0b11) // PIN, PORT und TOKEN sind definiert
            iRet = GetDevices();
        else
        {   if(!(i & 0b1))
                cout << "SOMFYPIN nicht definiert in Tahoma.config" << endl;
            if(!(i & 0b10))
                cout << "SOMYTOKEN nicht definiert in Tahoma.config" << endl;
        }
        break;
    default:
        break;
    }
    return iRet;
}

//
//  ReadConfig
//  iRet 0b00000000
//                | SOMFYPIN
//               | SOMFYTOKEN
//              | SOMFYUSER
//             | SOMFYPWD
int CTahoma::ReadConfig()
{
    int iRet = 0;
    char buf[256];

    m_pReadFile = new CReadFile;
    m_pReadFile->OpenRead(pProgramPath, 1); // Tahoma.config

    for (;;)
    {
        if (m_pReadFile->ReadLine())
        {
            m_pReadFile->ReadBuf(buf, ':');
            if (strncmp(buf, "SOMFYPIN", 8) == 0 && strlen(buf) == 8)
            {
                m_strPIN = m_pReadFile->ReadText();
                iRet |= 1;
            }
            else if (strncmp(buf, "SOMFYTOKEN", 10) == 0 && strlen(buf) == 10)
            {
                m_strTOKEN = m_pReadFile->ReadText();
                iRet |= 2;
            }
            else if (strncmp(buf, "SOMFYUSER", 9) == 0 && strlen(buf) == 9)
            {
                m_strUSER = m_pReadFile->ReadText();
                iRet |= 4;
            }  
            else if (strncmp(buf, "SOMFYPWD", 8) == 0 && strlen(buf) == 8)
            {
                m_strPWD = m_pReadFile->ReadText();
                iRet |= 8;
            }                  
        }
        else
            break;
    }
    m_pReadFile->Close();
    delete m_pReadFile;
    m_pReadFile = NULL;

    return iRet;
}
int CTahoma::ActivateToken()
{
    int iRet = 0, res, iErr;
    string strUrl, strBody, strReadBuffer, strHeader;
    struct curl_slist* headers = NULL;
    CJson *pJson = new CJson;
    CJsonNodeValue JsonNodeValue;
    bool bSuccess;

    m_pCurl = curl_easy_init();
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer); 
    // URL verification
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    // URL
    strUrl = m_strConnect + "/config/" + m_strPIN + "/local/tokens";
    curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl.c_str());
    // Headers
    headers = curl_slist_append(headers, "Content-Type:application/json;charset=utf-8");
    strHeader = "Cookie:JSESSIONID=" + m_strSessionID;
    headers = curl_slist_append(headers, strHeader.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers); 
    // POST       
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
    strBody = "{\"label\":\"Toto token\", \"token\":\"" + m_strTOKEN
                + "\",\"scope\":\"devmode\"}";
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, strBody.c_str());                 
    // Empfang
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CTahoma::write_data);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    
    res = curl_easy_perform(m_pCurl);
    if(res != CURLE_OK)
        cout << strReadBuffer << endl;
    else
    {    
        bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
        if(bSuccess)
        {
            JsonNodeValue.m_iTyp = -1;
            pJson->get((char *)"requestId", &JsonNodeValue);
            if(JsonNodeValue.m_iTyp != -1)
            {
                strBody = JsonNodeValue.asString();
                if(strBody.length() > 0)
                    iRet = 1;
                else
                    cout << "token nicht aktiviert" << endl;
            } 
            else
                cout << strReadBuffer << endl;             
        }
        else
            cout << strReadBuffer << endl; 
    }
    return iRet;
}

int CTahoma::GenerateToken()
{
    int iRet = 0, res, iErr;
    string strUrl, strBody, strReadBuffer, strHeader;
    struct curl_slist* headers = NULL;
    CJson *pJson = new CJson;
    CJsonNodeValue JsonNodeValue;
    bool bSuccess;

    m_pCurl = curl_easy_init();
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer); 
    // URL verification
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    // URL
    strUrl = m_strConnect + "/config/" + m_strPIN + "/local/tokens/generate";
    curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl.c_str());
    // Headers
    headers = curl_slist_append(headers, "Content-Type:application/json;charset=utf-8");
    strHeader = "Cookie:JSESSIONID=" + m_strSessionID;
    headers = curl_slist_append(headers, strHeader.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers); 
    // POST       
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);
    // Empfang
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CTahoma::write_data);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);    
    res = curl_easy_perform(m_pCurl);
    if(res != CURLE_OK)
        cout << strReadBuffer << endl;
    else
    {    
        bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
        if(bSuccess)
        {
            JsonNodeValue.m_iTyp = -1;
            pJson->get((char *)"token", &JsonNodeValue);
            if(JsonNodeValue.m_iTyp != -1)
            {
                m_strTOKEN = JsonNodeValue.asString();
                if(m_strTOKEN.length() > 0)
                    iRet = 1;
                else
                    cout << "token nicht erstellt" << endl;
            }
            else
                cout << strReadBuffer << endl;           
        }  
        else
            cout << strReadBuffer << endl; 
    }
    return iRet;
}
int CTahoma::GetSessionID()
{
    int res = 0, iErr, iPos;
    string strReadBuffer, strAuthentication, strCookie, strUrl;
    struct curl_slist* headers = NULL;
    bool bSuccess;
    CJson *pJson = new CJson;
    CJsonNodeValue JsonNodeValue; 

    m_pCurl = curl_easy_init();
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);   
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer); 
    // URL verification
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    // URL
    m_strConnect = "https://ha101-1.overkiz.com/enduser-mobile-web/enduserAPI";
    strUrl = m_strConnect + "/login";
    curl_easy_setopt(m_pCurl, CURLOPT_URL, strUrl.c_str()); 
    // Headers
    headers = curl_slist_append(headers, "application/x-www-form-urlencoded;charset=utf-8");
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);    
    // POST
    strAuthentication = "userId=" + m_strUSER + "&userPassword=" + m_strPWD;        
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, strAuthentication.c_str()); 
    // Empfang
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CTahoma::write_data);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer); 
    // export cookies to this file when closing the handle 
    strCookie = CReadFile::GetName(pProgramPath, 2, 0);
    curl_easy_setopt(m_pCurl, CURLOPT_COOKIEJAR, strCookie.c_str());
    // Perform the request, res will get the return code 
    res = curl_easy_perform(m_pCurl);
    if(res != CURLE_OK)
        cout << strReadBuffer << endl;
    else
    {
        curl_easy_cleanup(m_pCurl); // speichert cookies ab
        bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
        if(bSuccess)
        {
            pJson->get((char *)"success", &JsonNodeValue);
            if(JsonNodeValue.asBool())
            {
                m_pReadFile = new CReadFile();
                m_pReadFile->OpenRead(pProgramPath, 2);
                for (;;)
                {
                    if (m_pReadFile->ReadLine())
                    {   
                        iPos = m_pReadFile->FindText((char *)"JSESSIONID");
                        if(iPos)
                        {
                            m_strSessionID = m_pReadFile->ReadText();
                            res = 1;
                            break;
                        }
                    }
                    else
                    {   
                        cout << "JSESSONID nicht in cookie gefunden!" << endl;
                        return 0;
                    }
                }
                m_pReadFile->Close();
                delete m_pReadFile;
                m_pReadFile = NULL;
            }
            else
                cout << strReadBuffer << endl;
        }
        else
            cout << "login nicht erfolgreich!" << endl;
    }   
    return res;
}

int CTahoma::GetDevices()
{
    int iRet = 0, res, j, i, iErr, iMax;
    struct curl_slist *slist = NULL;
    string strReadBuffer;
    string strLabel, strIO, strClass, strWrite;
    bool bSuccess;
    CJson *pJson = new CJson;
    CJsonNodeValue pJsonNodeValue;  
    CJsonNode *pJsonNode;
    
    m_pCurl = curl_easy_init();
    m_strConnect = "https://gateway-" + m_strPIN + ".local:" + m_strPORT + "/enduser-mobile-web/1/enduserAPI";
    m_strAuthorization = "Authorization: Bearer " + m_strTOKEN;
    // Timeout auf 10 Sekunden
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);
    // Error string
    curl_easy_setopt(m_pCurl, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);
    // URL verification disabled
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 0L);
    // URL
    m_strConnect += "/setup/devices";
    curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strConnect.c_str());
    // GET
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);

    //slist = curl_slist_append(slist, "Content-Type:application/json");    
    // Authorization
    slist = curl_slist_append(slist, m_strAuthorization.c_str());
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, slist);
    // Empfang
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &CTahoma::write_data);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void *)&strReadBuffer);

    // Perform the request, res will get the return code
    res = curl_easy_perform(m_pCurl);
    if(res != CURLE_OK) 
        cout << m_curlErrorBuffer << endl;
    else
    {
        bSuccess = pJson->parse((char *)strReadBuffer.c_str(), strReadBuffer.length(), &iErr);
        if(bSuccess)
        {   
            pJson->WriteToFile("Tahoma", 4, true);
            m_pReadFile = new CReadFile;
            m_pReadFile->OpenWrite(pProgramPath, 3); // prowo.config
            strWrite = "SOMFYPIN:" + m_strPIN + "\n";
            m_pReadFile->WriteLine(strWrite.c_str());
            strWrite = "SOMFYPORT:" + m_strPORT + "\n";
            m_pReadFile->WriteLine(strWrite.c_str());
            strWrite = "SOMFYTOKEN:" + m_strTOKEN + "\n";
            m_pReadFile->WriteLine(strWrite.c_str());       
            pJsonNode = pJson->get((char *)"", &pJsonNodeValue);
            j = pJsonNode->size();
            for(i=0; i < j; i++)
            {
                pJsonNode = pJson->get((char *)"", &pJsonNodeValue)
                        ->get(i, &pJsonNodeValue);
                if(pJsonNodeValue.m_iTyp == 0)
                {
                    cout << strReadBuffer << endl;
                    bSuccess = false;
                    break;
                }
                pJsonNode->get((char *)"label", &pJsonNodeValue);
                strLabel = pJsonNodeValue.asString();
                pJsonNode->get((char *)"deviceURL", &pJsonNodeValue);
                strIO = pJsonNodeValue.asString();
                pJsonNode->get((char *)"definition", &pJsonNodeValue)->get((char *)"uiClass", &pJsonNodeValue);
                strClass = pJsonNodeValue.asString(); 
                iMax = 0;
                if(strClass.compare("Awning") == 0) 
                {   strClass = "SOMFYAWNING";
                    iMax = 100;
                }
                else if(strClass.compare("Light") == 0)
                {   strClass = "SOMFYLEDLIGHT";
                    iMax = 100;
                }
                else if(strClass.compare("Window") == 0)
                {   strClass = "SOMFYWINDOW";
                    iMax = 100;
                }
                else if(strClass.compare("ExteriorScreen") == 0)
                {   strClass = "SOMFYAWNING";
                    iMax = 100;
                }
                else if(strClass.compare("RollerShutter") == 0)
                {   strClass = "SOMFYAWNING";
                    iMax = 100;
                }
                strIO = ReplaceBackSlash(strIO);
                strWrite = "";
                if(!iMax)
                    strWrite = "\'";
                strWrite += strClass + ":" + strIO + ";" + to_string(iMax) + "\' " +strLabel + "\n";
                m_pReadFile->WriteLine(strWrite.c_str());                          
            }  
            m_pReadFile->Close();
            delete m_pReadFile;
            m_pReadFile = NULL; 
            if(bSuccess)  
                cout << "Sie finden das Resultat in " + CReadFile::GetName(pProgramPath, 3) << endl;        
        }
    }
    curl_easy_cleanup(m_pCurl);
    return iRet;
}

size_t CTahoma::write_data(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string CTahoma::ReplaceBackSlash(string str)
{
    string newStr;
    int i, len;
    char ch;

    len = str.length();
    for(i=0; i < len; i++)
    {
        ch = str.at(i);
        if(ch != 0x5c)
            newStr += ch;
    }
    return newStr;
}
int CTahoma::WriteToTahomaConfig()
{
    int iRet = 0;
    string strWrite;

    m_pReadFile = new CReadFile;
    m_pReadFile->OpenWrite(pProgramPath, 1); // Tahoma.config
    strWrite = "' Definition Somfy erstellt von dem Tahoma-Program\n";
    m_pReadFile->WriteLine(strWrite.c_str());    
    strWrite = "SOMFYPIN:" + m_strPIN + "\n";
    m_pReadFile->WriteLine(strWrite.c_str());
    strWrite = "SOMFYUSER:" + m_strUSER + "\n";
    m_pReadFile->WriteLine(strWrite.c_str());
    strWrite = "SOMFYPWD:" + m_strPWD + "\n";
    m_pReadFile->WriteLine(strWrite.c_str());
    strWrite = "SOMFYTOKEN:" + m_strTOKEN + "\n";
    m_pReadFile->WriteLine(strWrite.c_str());   
    m_pReadFile->Close();
    delete m_pReadFile;
    m_pReadFile = NULL; 
    return iRet;
}
/*  m_strUrl = "";
    for(pos = 0; pos < strUrl.length(); pos++)
    {
        if(strUrl.at(pos) == ':')
            m_strUrl += "%3A";
        else if(strUrl.at(pos) == '/')
            m_strUrl += "%2F";
        else
            m_strUrl += strUrl.at(pos);
    }*/