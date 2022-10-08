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

	//�����ͻ����׽��֣�ʹ��TCPЭ��
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockClient == INVALID_SOCKET)
	{
		cout << "�׽��ִ���ʧ��" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}
	else {
		cout << "�׽��ִ����ɹ�" << endl;
	}

	//addrStv
	SOCKADDR_IN addrSrv;
	//���õ�ַ��ΪIPv4
	addrSrv.sin_family = AF_INET;
	//���õ�ַ�Ķ˿ں���Ϣ
	addrSrv.sin_port = htons(8888);
	//127.0.0.1һ�������IP��ַ����ʾ�Ǳ�����IP��ַ                                               
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//��һ������˵�socket������������
	int conn = connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (conn == SOCKET_ERROR) {
		closesocket(sockClient);
		cout << "���Ӵ���" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}
	else {
		cout << "���ӳɹ�" << endl;
	}

	char mes[1024] = { "Hello" };
	int send_result = send(sockClient, mes, (int)strlen(mes), 0);
	if (send_result == SOCKET_ERROR) {
		cout << "����ʧ�ܣ�" << WSAGetLastError() << endl;
		closesocket(sockClient);
		WSACleanup();
		return 1;
	}
	cout << "������Ϣ��" << mes << endl;

	//
	char recvbuf[512] = { 0 };
	int recv_result;
	do {
		recv_result = recv(sockClient, recvbuf, 512, 0);
		if (recv_result > 0)
			cout << "�յ���Ϣ��" << recvbuf << endl;
		else if (recv_result == 0)
			cout << "���ӹر�" << endl;
		else
			cout << "�յ�ʧ�ܣ�" << WSAGetLastError() << endl;
	} while (recv_result > 0);

	
	closesocket(sockClient);
	WSACleanup();
	return 0;
}