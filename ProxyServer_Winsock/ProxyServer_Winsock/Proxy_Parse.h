#include "stdafx.h"
#include<map>
#define DEFAULT_BUFLEN 1460
#define DEFAULT_PORT "8888"

using namespace std;

//function:
bool GetDomainName(char *request, char *dname);
bool IsGETMethod(char *request);
bool IsPOSTMethod(char *request);
int GetContent_Length(string head);
bool UpdateBlacklist(string input);
bool Update403(string input);
UINT Proxy(LPVOID prams);