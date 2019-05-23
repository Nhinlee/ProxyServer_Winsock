#include "Proxy_Parse.h"
#include<fstream>	
#include<time.h>
#include <sstream>
#include <set>
//data:
map<string, int> blacklist;
string fbd403;
struct HEADER_IN_CACHE {
	bool Exist;
	string filename;
	string date;
	int size;
};
set<char> invalid_char_set = { 92,47,58,42,63,34,60,62,124 };
//Function:
bool GetDomainName(char *request, char *dname)
{
	int n = strlen(request), pos = 0, i = 0;
	n -= 6;
	for (; i < n; i++)
	{
		if (request[i] == 'H' && request[i + 1] == 'o' && request[i + 2] == 's' && request[i + 3] == 't'
			&& request[i + 4] == ':')
		{
			pos = i + 6;
			break;
		}
	}
	if (i == n)
		return false;
	i = 0;
	while (pos < n && request[pos] != '\r'/*&& request[pos] != ':'*/)// Nếu là cổng 443 (HTTPs) thì phải chạy tới : và dừng lại.
	{
		dname[i] = request[pos];
		pos++;
		i++;
	}
	dname[i] = '\0';
	return true;

}

bool IsGETMethod(char *request)
{
	if (strlen(request) < 3)
		return false;
	if (request[0] == 'G' && request[1] == 'E' && request[2] == 'T')
		return true;
	return false;
}

bool IsPOSTMethod(char *request)
{
	if (strlen(request) < 4)
		return false;
	if (request[0] == 'P' && request[1] == 'O' && request[2] == 'S' && request[3] == 'T')
		return true;
	return false;
}

int GetContent_Length(string head)
{
	int pos = head.find("Content-Length: ");
	int sum = 0;
	if (pos == -1)
		return -1;
	else {
		pos += 16;
		while (head[pos] != '\r')
		{
			sum += head[pos] - '0';
			sum *= 10;
			pos++;
		}
		sum /= 10;
		return sum;
	}
}

bool UpdateBlacklist(string input)
{
	ifstream inp(input);
	if (inp.fail())
		return false;
	string host;
	while (!inp.eof())
	{
		inp >> host;
		blacklist[host]++;
		if (host.find("www.") == -1)
		{
			host = "www." + host;
			blacklist[host]++;
		}
	}
	return true;
}

char* strtochar(string s)
{
	char *a = new char[s.length() + 1];
	int n = s.length();
	for (int i = 0; i < n; i++)
		a[i] = s[i];
	a[n] = '\0';
	return a;
}

string chartostr(char *st, int len)
{
	int sz = len;
	string tmp;
	for (int i = 0; i < sz; i++)
		tmp += *(st + i);
	return tmp;
}

void UpdateDate(string &res)
{
	time_t t;
	tm *pt;
	time(&t);
	pt = gmtime(&t);
	int pos = res.find("Date: ");
	char* temp = asctime(pt);
	temp[strlen(temp) - 1] = '\0';
	res.insert(pos + 6, " GMT");
	res.insert(pos + 6, temp);
}

bool Update403(string input)
{
	ifstream inp(input);
	if (inp.fail())
		return false;
	char c;
	while (!inp.eof())
	{
		c = inp.get();
		if (c == -1)
			break;
		if (c == '\n')
			fbd403.push_back('\r');
		fbd403.push_back(c);
	}
	UpdateDate(fbd403);
	return true;
}

string GetFileName(char *str, int len)
{
	string tmp = chartostr(str, len), res;
	int pos = tmp.find(" ");
	for (int i = pos + 1; tmp[i] != ' '; i++)
	{
		if (invalid_char_set.find(tmp[i]) != invalid_char_set.end())
			tmp[i] = '_';
		res += tmp[i];
	}
	return res;
}

HEADER_IN_CACHE Find_In_Cache(string filename)
{
	HEADER_IN_CACHE tmp;
	//initiate value for tmp
	tmp.Exist = false;
	tmp.date = "";
	tmp.filename = "";
	tmp.size = 0;
	ifstream is("header_file.conf");
	if (is.fail())
		return tmp;
	else
	{
		//find date and size of this filename
		//look in header file	
		filename += ".conf";
		string st;
		while (getline(is, st))
			if (st.find(filename) != -1)
			{
				tmp.filename = filename;
				tmp.Exist = true;
				int start = st.find("\t");
				start += 1;
				for (; st[start] != '\t'; start++)
					tmp.date += st[start];
				start += 1;
				string num;
				for (; start < st.size(); start++)
					num += st[start];
				stringstream ss;
				ss << num;
				ss >> tmp.size;
				break;
			}
	}
	is.close();
	return tmp;
}

string Get_Last_Modified(char *header_res, int len)
{
	string h_res = chartostr(header_res, len), res;
	int pos = h_res.find("Last-Modified: ");
	if (pos == -1)
		pos = h_res.find("Date: ");
	if (pos == -1)
		return NULL;
	int i;
	for (i = pos; h_res[i] != ' '; i++)
		;//do nothing
	i += 1;
	for (; (h_res[i] != '\r') && (h_res[i] != '\n'); i++)
		res += h_res[i];
	return res;
}

void Find_And_Rep(int line, HEADER_IN_CACHE tmp)
{
	string st;
	//create temp file
	ofstream os("Cache\\temp_file.conf");
	ifstream is("Cache\\header_file.conf");
	for (int i = 1; i < line; i++)
	{
		getline(is, st);
		os << st << "\n";
	}
	//replace the line
	os << tmp.filename + ".conf" << '\t' << tmp.date << '\t' << tmp.size << '\n';
	getline(is, st);
	//read the rest of file
	while (getline(is, st))
		os << st << "\n";
	is.close();
	os.close();
	//delete the original file
	int err = remove("Cache\\header_file.conf");
	//rename the temp file to name of original file
	err = rename("Cache\\temp_file.conf", "Cache\\header_file.conf");
}

void BackUpHeader(HEADER_IN_CACHE tmp)
{
	ifstream is("header_file.conf");
	if (is.fail())
	{
		ofstream os("header_file.conf");
		os << tmp.filename + ".conf" << '\t' << tmp.date << '\t' << tmp.size << '\n';
		os.close();
	}
	else
	{
		int ln = -1, cur = 0;
		string st;
		while (getline(is, st))
		{
			cur += 1;
			if (st.find(tmp.filename + ".conf") != -1)
			{
				ln = cur;
				break;
			}
		}
		if (ln == -1)
		{
			ofstream os("header_file.conf", ios::in | ios::out | ios::app);
			os << tmp.filename + ".conf" << '\t' << tmp.date << '\t' << tmp.size << '\n';
			os.close();
			is.close();
		}
		else
		{
			//open a temp file
			//write whole data from original file to the temp, except date in line ln, replace date in this line with newer data
			//then delete the original file, rename the temp file
			is.close();
			Find_And_Rep(ln, tmp);
		}
	}
}

UINT Proxy(LPVOID prams)
{
	cout << "Da co Client ket noi !!! \n\n";
	// Initialize buffer memory and ClientSocket:
	SOCKET ClientSocket = (SOCKET)prams;
	// Buffer:
	char request[5000] = { 0 }, dname[100] = { 0 }, ip[16] = { 0 },
		body_res[DEFAULT_BUFLEN] = { 0 }, header_res[5000] = { 0 };
	//************************************************************************************************************
	// Recieve Request from Client( Browser).
	int bytes = recv(ClientSocket, request, sizeof request, 0);
	if (bytes > 0) {
		cout << "Bytes received:" << bytes << endl << request;
	}
	else if (bytes == 0)
	{
		cout << " No Request !!!\n";
		closesocket(ClientSocket);
		return 0;
	}
	//************************************************************************************************************
	bool client_cache;
	if (chartostr(request, bytes).find("If-Modified-Since: ") == -1 && chartostr(request, bytes).find("If-None-Match: ") == -1)
		client_cache = false;
	else
		client_cache = true;
	//************************************************************************************************************
	//If Request is GET or POST method, It's continue processes
	if (!IsGETMethod(request) && !IsPOSTMethod(request))
	{
		closesocket(ClientSocket);
		return 0;
	}

	//Get File Name:
	string filename = GetFileName(request, bytes);

	//Get Host Name:
	if (GetDomainName(request, dname) == false)
	{
		cout << "Get Domain Name False !!!";
		closesocket(ClientSocket);
		return 0;
	}
	else cout << dname << "\n";


	//Refuse with blacklist:
	if (blacklist[dname])
	{
		//Update 403 request:
		Update403("403.conf");
		char* res = strtochar(fbd403);
		bytes = send(ClientSocket, res, (int)strlen(res), 0);
		fbd403.clear();
		delete []res;
		cout << "-------------Web in black list----------------" << endl;
		closesocket(ClientSocket);
		return 0;
	}

	//GET IP from Host name of web server:
	hostent *remoteHost = gethostbyname(dname);
	in_addr addr;
	addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];

	//Create SOCKET connect to web server with ip and post 80 default:
	SOCKET ConnectSocket = INVALID_SOCKET;
	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	//Create struct sockaddr_in to save infor of web server:
	sockaddr_in AddrIP;
	AddrIP.sin_family = AF_INET;
	AddrIP.sin_port = htons(80);
	AddrIP.sin_addr = addr;
	//Connect web server:
	int iResult = connect(ConnectSocket, (sockaddr *)&AddrIP, sizeof AddrIP);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		cout << "Failed Connect with web server\n";
		return 0;
	}
	else cout << "Connect Success to Web\n\n";
	//************************************************************************************************************
	//TIM KIEM filename TRONG CACHE CUA PROXY SERVER
	HEADER_IN_CACHE headcache;
	headcache = { false,"","",0 };
	headcache = Find_In_Cache(filename);
	//************************************************************************************************************
	//Send request from client to web server
	bytes = send(ConnectSocket, request, bytes, 0);
	//************************************************************************************************************
	//Recieve Header Respones:
	int id = 0, endhead = 0;
	string head;
	while (endhead < 4)
	{
		bytes = recv(ConnectSocket, header_res + id, 1, 0);
		head.push_back(header_res[id]);
		cout << header_res[id];
		if (header_res[id] == '\r' || header_res[id] == '\n')
			endhead++;
		else endhead = 0;
		id++;
	}
	if (head.find(" 206 ") != -1)
		filename = filename;
	//************************************************************************************************************
	string headerdate = Get_Last_Modified(header_res, id);//LAY LAST MODIFIED HOAC DATE HEADER TRONG RESPONSE TU WEB SERVER
	//************************************************************************************************************
	//Send Header Respones for client( Browser) with id (bytes) exactly:
	bytes = send(ClientSocket, header_res, id, 0);
	cout << id << " Byte ---- Xong header roi nhe !\n\n";
	//************************************************************************************************************
	//Get Content-Length of body res:
	int ctlength = GetContent_Length(head);
	cout << "Content-Length: " << ctlength << endl;
	/*if (head.find("304") != -1)
	{
		cout << "\nDu lieu duoc lay tu cache cua browser!!!\n\n";
		closesocket(ConnectSocket);
		closesocket(ClientSocket);
		return 0;
	}*/
	//************************************************************************************************************
	//XY LY HEADER VA BODY TRONG CAC TRUONG HOP CU THE
	bool err200or304 = true;
	if (head.find("304 Not Modified") == -1 && head.find("200 OK") == -1)
		err200or304 = false;
	if (client_cache == false)
	{
		if (headcache.Exist)
			if (headcache.date == headerdate && err200or304)
			{
				//khong nhan body tu web server
				//lay noi dung trong cache de tra ve client
				//LAY DU LIEU TU BO NHO CACHE
				ifstream inp("Cache\\"+headcache.filename, ios::binary | ios::in);
				int cnt;
				cnt = 0;
				cnt = headcache.size;
				while (cnt > 0)
				{
					char a[1460];
					int c = min(1460, cnt);
					for (int i = 0; i < c; i++)
						inp.read(a + i, 1);
					int b = send(ClientSocket, a, c, 0);
					cnt -= c;
				}
				inp.close();
				closesocket(ClientSocket);
				closesocket(ConnectSocket);
				return 0;
			}
	}
	else
	{
		if (head.find("304 Not Modified") != -1)
		{
			//dung chuong trinh
			closesocket(ConnectSocket);
			closesocket(ClientSocket);
			return 0;
		}
		else
		{
			if (head.find("200 OK") != -1)
				if (headcache.date == headerdate && err200or304)
				{
					//khong nhan body tu web server
					//lay noi dung trong cache de tra ve client
					//LAY DU LIEU TU BO NHO CACHE
					ifstream inp("Cache\\"+headcache.filename, ios::binary | ios::in);
					int cnt;
					cnt = 0;
					cnt = headcache.size;
					while (cnt > 0)
					{
						char a[1460];
						int c = min(1460, cnt);
						for (int i = 0; i < c; i++)
							inp.read(a + i, 1);
						int b = send(ClientSocket, a, c, 0);
						cnt -= c;
					}
					inp.close();
					closesocket(ClientSocket);
					closesocket(ConnectSocket);
					return 0;
				}
		}
	}
	//************************************************************************************************************
	//Get body response from web server:
	int bytes_rev = 0, sum_bytes = 0;
	ofstream out("Cache\\"+filename + ".conf", ios::binary | ios::out);
	do
	{
		bytes_rev = recv(ConnectSocket, body_res, DEFAULT_BUFLEN, 0);
		sum_bytes += bytes_rev;
		ctlength -= bytes_rev;
		if (bytes_rev > 0) {
			cout << "Bytes received: " << bytes_rev << endl;
			//backup body to proxy cache
			string body = chartostr(body_res, bytes_rev);
			if (err200or304)
				for (int i = 0; i < bytes_rev; i++)
					out.write(body_res + i, 1);
			// Echo the buffer back to the sender
			int bytes_send = send(ClientSocket, body_res, bytes_rev, 0);
			if (bytes_send == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError();
				break;
				closesocket(ConnectSocket);
				closesocket(ClientSocket);
				return 0;
			}
			if (ctlength > 0)
				continue;
			if (bytes_rev < DEFAULT_BUFLEN)
				break;
		}
		else if (bytes_rev == 0)
		{
			cout << "Connection closing...\n";
			closesocket(ConnectSocket);
			closesocket(ClientSocket);
			return 0;
		}
		else {
			cout << "recv failed with error: " << WSAGetLastError();
			closesocket(ConnectSocket);
			closesocket(ClientSocket);
			//WSACleanup();
			return 0;
		}
	} while (bytes_rev > 0);
	out.close();
	//************************************************************************************************************
	//BACKUP HEADER VAO CACHE
	headcache.date = headerdate;
	headcache.Exist = true;
	headcache.filename = filename;
	headcache.size = sum_bytes;
	if (err200or304)
		BackUpHeader(headcache);
	//************************************************************************************************************
	cout << sum_bytes << endl;
	cout << "Da Thuc Hien Xong !\n\n";
	//Close SOCKET and return function:
	closesocket(ConnectSocket);
	closesocket(ClientSocket);
	return 1;
}