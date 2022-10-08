#include<iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable : 6031)
#pragma warning(disable : 4996)
using namespace std;

int main() {

	//��ʼ��Socket DLL��ʹ��2.2�汾��socket
	WORD wVersionRequested;
	WSADATA wsaData;
	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);

	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
	if (sockSrv == INVALID_SOCKET)
	{
		cout << "�׽��ִ���ʧ��" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	//addrStv
	SOCKADDR_IN addrSrv;
	//���õ�ַ��ΪIPv4
	addrSrv.sin_family = AF_INET;
	//���õ�ַ�Ķ˿ں���Ϣ
	addrSrv.sin_port = htons(8888);                                             
	addrSrv.sin_addr.S_un.S_addr = INADDR_ANY;

	int bind_redult = bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (bind_redult == SOCKET_ERROR) {
		cout << "��ʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockSrv);
		return 1;
	}

	//��ʼ����
	int lis = listen(sockSrv, 100);
	if (lis == SOCKET_ERROR) {
		cout << "���ִ���" << WSAGetLastError() << endl;
		closesocket(sockSrv);
		WSACleanup();
		return 1;
	}

	//�ȴ�����
	SOCKADDR_IN addrClient;
	int len = sizeof(addrClient);
	SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
	if (sockConn == INVALID_SOCKET)
	{
		cout << "�ͻ��˷������󣬷�������������ʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockConn);
		WSACleanup();
		return 1;
	}
	else {
		cout << "��������������" << endl;
	}

	char recvBuf[512] = { 0 };
	int relen = recv(sockConn, recvBuf, 512, 0);
	if (relen == SOCKET_ERROR) {
		cout << "������û���յ���Ϣ��" << WSAGetLastError() << endl;
		closesocket(sockSrv);
		WSACleanup();
		return 1;
	}
	else {
		cout << "�������յ���Ϣ��" << recvBuf << endl;
	}
	int slen = send(sockConn, recvBuf, relen, 0);
	if (slen == SOCKET_ERROR) {
		cout << "������������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockSrv);
		WSACleanup();
		return 1;
	}
	else {
		cout << "������������Ϣ��" << recvBuf << endl;
	}
	/*
	while (1)
	{
		while (1) {

		}
		send(sockConn, sendBuf, strlen, 0);
		int recv_result;
		do {
			recv_result = recv(sockSrv, recvbuf, 512, 0);
			if (recv_result > 0)
				cout << "�յ���Ϣ��" << recv_result;
			else if (recv_result == 0)
				cout << "���ӹر�";
			else
				cout << "�յ�ʧ�ܣ�" << WSAGetLastError();
		} while (recv_result > 0);
		closesocket(sockConn);
	}*/
	
	closesocket(sockSrv);
	WSACleanup();

}