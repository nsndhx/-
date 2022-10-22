#include<iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <time.h>

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

	//��������ڽ�������ͨ�ŵĵ�ַ�Ͷ˿ڰ󶨵� socket ��
	int bind_redult = bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (bind_redult == SOCKET_ERROR) {
		cout << "��ʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockSrv);
		return 1;
	}

	//��ʼ����
	int lis = listen(sockSrv, 5);
	if (lis == SOCKET_ERROR) {
		cout << "���ִ���" << WSAGetLastError() << endl;
		closesocket(sockSrv);
		WSACleanup();
		return 1;
	}

	//ʵ��Ⱥ��
	//���ӿͻ��ˣ����100����
	int amount = 0;
	cin >> amount;
	SOCKADDR_IN addrClient[100];
	SOCKET sockConn[100];

	for (int i = 1; i <= amount; i++) {
		int len = sizeof(addrClient[i]);
		sockConn[i] = accept(sockSrv, (SOCKADDR*)&addrClient[i], &len);
		if (sockConn[i] == INVALID_SOCKET)
		{
			cout << "�ͻ���" << i << "�������󣬷�������������ʧ�ܣ�" << WSAGetLastError() << endl;
			closesocket(sockConn[i]);
			WSACleanup();
			return 1;
		}
		else {
			cout << "���������տͻ���" << i << "������" << endl;
		}
	}
	//�����пͻ��˴����ж����˲���Ⱥ���Լ����Ե����
	for (int i = 1; i <= amount; i++) {
		char buf[2];
		buf[0] = (char)i;
		buf[1] = (char)amount;
		int send_r = send(sockConn[i], buf, 1024, 0);
		if (send_r == SOCKET_ERROR) {
			cout << "��������ͻ���" << i << "������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
			closesocket(sockConn[i]);
			WSACleanup();
			return 1;
		}
	}

	//������Ϣ
	char recvBuf[1024] = { 0 };
	char t[1024] = { 0 };
	int recv_result = 0;
	int send_result = 0;
	int i = 1;
	while (1) {
		memset(recvBuf, '\0', sizeof(recvBuf));
		memset(t, '\0', sizeof(t));
		recv_result = recv(sockConn[i], recvBuf, 1024, 0);
		if (recv_result == SOCKET_ERROR) {
			cout << "������û���յ��ͻ���" << i << "����Ϣ��" << WSAGetLastError() << endl;
			closesocket(sockConn[i]);
			WSACleanup();
			return 1;
		}
		else {
			cout << "�������յ��ͻ���" << i << "����Ϣ��" << recvBuf << endl;
			string buf_dosoming(recvBuf);
			strcpy(t, (buf_dosoming.substr(buf_dosoming.length() - 4, buf_dosoming.length() - 1).c_str()));
		}
		for (int j = 1; j <= amount; j++) {
			if (j != i) {
				send_result = send(sockConn[j], recvBuf, 1024, 0);
				if (send_result == SOCKET_ERROR) {
					cout << "��������ͻ���" << j << "������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
					closesocket(sockConn[j]);
					WSACleanup();
					return 1;
				}
				else {
					cout << "��������ͻ���" << j << "������Ϣ��" << recvBuf << endl;
				}
			}
		}
		if (strcmp(t, "over") == 0) {
			if (i != amount) {
				i++;
			}
			else {
				i = 1;
			}
		}
		else if (strcmp(t, "exit") == 0) {
			for (int j = 1; j <= amount; j++) {
				closesocket(sockConn[j]);
			}
			WSACleanup();
			return 1;
		}
	}
	closesocket(sockSrv);
	WSACleanup();
	return 0;
}