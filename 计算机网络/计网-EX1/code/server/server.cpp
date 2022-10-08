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

	//���������ͻ���
	SOCKADDR_IN addrClient1;
	SOCKADDR_IN addrClient2;
	int len1 = sizeof(addrClient1);
	int len2 = sizeof(addrClient2);
	SOCKET sockConn1 = accept(sockSrv, (SOCKADDR*)&addrClient1, &len1);
	if (sockConn1 == INVALID_SOCKET)
	{
		cout << "�ͻ���1�������󣬷�������������ʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockConn1);
		WSACleanup();
		return 1;
	}
	else {
		cout << "���������տͻ���1������" << endl;
	}
	SOCKET sockConn2 = accept(sockSrv, (SOCKADDR*)&addrClient2, &len2);
	if (sockConn2 == INVALID_SOCKET)
	{
		cout << "�ͻ���2�������󣬷�������������ʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockConn2);
		WSACleanup();
		return 1;
	}
	else {
		cout << "���������տͻ���2������" << endl;
	}

	
	char recvBuf[1024] = { 0 };
	int recv1_result = 0;
	int send1_result = 0;
	int recv2_result = 0;
	int send2_result = 0;
	
	while (1) {
		while (1) {//�ӿͻ���1������Ϣ�����͸��ͻ���2
			memset(recvBuf, '\0', sizeof(recvBuf));
			recv1_result = recv(sockConn1, recvBuf, 1024, 0);
			if (recv1_result == SOCKET_ERROR) {
				cout << "������û���յ��ͻ���1����Ϣ��" << WSAGetLastError() << endl;
				closesocket(sockConn1);
				WSACleanup();
				return 1;
			}
			else {
				cout << "�������յ��ͻ���1����Ϣ��" << recvBuf << endl;
			}
			send2_result = send(sockConn2, recvBuf, 1024, 0);
			if (send2_result == SOCKET_ERROR) {
				cout << "��������ͻ���2������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
				closesocket(sockConn2);
				WSACleanup();
				return 1;
			}
			else {
				cout << "��������ͻ���2������Ϣ��" << recvBuf << endl;
			}
			if (strcmp(recvBuf, "exit") == 0) {
				closesocket(sockConn1);
				closesocket(sockConn2);
				WSACleanup();
				return 1;
			}
			if (strcmp(recvBuf, "over") == 0) {
				break;
			}
		}
		while (1) {//�ӿͻ���2������Ϣ�����͸��ͻ���1
			memset(recvBuf, '\0', sizeof(recvBuf));
			recv2_result = recv(sockConn2, recvBuf, 1024, 0);
			if (recv2_result == SOCKET_ERROR) {
				cout << "������û���յ��ͻ���2����Ϣ��" << WSAGetLastError() << endl;
				closesocket(sockConn2);
				WSACleanup();
				return 1;
			}
			else {
				cout << "�������յ��ͻ���2����Ϣ��" << recvBuf << endl;
			}
			send1_result = send(sockConn1, recvBuf, 1024, 0);
			if (send1_result == SOCKET_ERROR) {
				cout << "��������ͻ���1������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
				closesocket(sockConn1);
				WSACleanup();
				return 1;
			}
			else {
				cout << "��������ͻ���1������Ϣ��" << recvBuf << endl;
			}
			if (strcmp(recvBuf, "exit") == 0) {
				closesocket(sockConn1);
				closesocket(sockConn2);
				WSACleanup();
				return 1;
			}
			if (strcmp(recvBuf, "over") == 0) {
				break;
			}
		}
	}
	
	closesocket(sockSrv);
	WSACleanup();
	return 0;
}