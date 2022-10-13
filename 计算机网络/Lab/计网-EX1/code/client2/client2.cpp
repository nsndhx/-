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
	SOCKET sockClient2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockClient2 == INVALID_SOCKET)
	{
		cout << "�׽��ִ���ʧ��" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}
	//else {
	//	cout << "�׽��ִ����ɹ�" << endl;
	//}

	//addrStv
	SOCKADDR_IN addrSrv;
	//���õ�ַ��ΪIPv4
	addrSrv.sin_family = AF_INET;
	//���õ�ַ�Ķ˿ں���Ϣ
	addrSrv.sin_port = htons(8888);
	//127.0.0.1һ�������IP��ַ����ʾ�Ǳ�����IP��ַ                                               
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//��һ������˵�socket������������
	int conn = connect(sockClient2, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (conn == SOCKET_ERROR) {
		closesocket(sockClient2);
		cout << "���ӷ���˴���" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}
	//else {
	//	cout << "���ӷ���˳ɹ�" << endl;
	//}
	cout << "���ӳɹ�" << endl;
	//ȷ����˭�����Ի�
	char mes[1024] = { 0 };
	char recvbuf[1024] = { 0 };
	int send_result = 0;
	int recv_result = 0;
	/*���ͻ��˷���overʱ������������Ϣ������exitʱ�������Ի�*/
	while (1) {
		while (1) {//�ͻ���2����1����Ϣ
			memset(recvbuf, '\0', sizeof(recvbuf)); 
			recv_result = recv(sockClient2, recvbuf, 1024, 0);
			if (recv_result > 0){}
				//cout << "�յ���Ϣ��" << recvbuf << endl;
			else if (recv_result == 0)
				cout << "���ӹر�" << endl;
			else {
				cout << "�յ�ʧ�ܣ�" << WSAGetLastError() << endl;
				closesocket(sockClient2);
				WSACleanup();
				return 1;
			}
			if (strcmp(recvbuf, "exit") == 0) {
				cout << "�Է������Ի�" << endl;
				closesocket(sockClient2);
				WSACleanup();
				return 1;
			}
			else if (strcmp(recvbuf, "over") == 0) {
				break;
			}
			cout << "client1��" << recvbuf << endl;
		}
		while (1) {//�ͻ���2��1����Ϣ
			cout << "client2��";
			memset(mes, '\0', sizeof(mes));
			cin.getline(mes, sizeof(mes));
			send_result = send(sockClient2, mes, (int)strlen(mes), 0);
			if (send_result == SOCKET_ERROR) {
				cout << "������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
				closesocket(sockClient2);
				WSACleanup();
				return 1;
			}
			//cout << "������Ϣ��" << mes << endl;
			if (strcmp(mes, "exit") == 0)
			{
				cout << "�����Ի�" << endl;
				closesocket(sockClient2);
				WSACleanup();
				return 1;
			}
			else if (strcmp(mes, "over") == 0) {
				break;
			}
		}
	}

	closesocket(sockClient2);
	WSACleanup();
	return 0;
}