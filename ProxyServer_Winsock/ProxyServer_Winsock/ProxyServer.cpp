#include"stdafx.h"
#include"Proxy_Parse.h"

//#pragma comment(lib,"Ws2_32.lib")

using namespace std;

//map<string, int> blacklist;

int main()
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	//SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult;
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError();
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	else cout << "\t\t\t\t\tListenSocket has been Created \n";

	// Setup the TCP listening socket:
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError();
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else cout << "\t\t\t\t\tWaiting... ... ... ... ... ... ... ...\n";

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError();
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	cout << "\t\t\t\t\tlistenning-------------------- ^ ^\n";

	//Update blacklist:
	UpdateBlacklist("blacklist.conf");

	//proxy process:
	while (true)
	{
		SOCKET ClientSocket = INVALID_SOCKET;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			cout << "accept failed with error: " << WSAGetLastError();
			/*closesocket(ListenSocket);
			WSACleanup();
			return 1;*/
			closesocket(ClientSocket);
			continue;
		}
		// Thread:
		AfxBeginThread(Proxy, (LPVOID)ClientSocket);
		//Proxy((LPVOID)ClientSocket);
	}

	closesocket(ListenSocket);
	WSACleanup();
	system("pause");
	return iResult;
}