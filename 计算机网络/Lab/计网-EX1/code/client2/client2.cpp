#include<iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include<time.h> 
#include <sstream>

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
	int conn = connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (conn == SOCKET_ERROR) {
		closesocket(sockClient);
		cout << "���ӷ���˴���" << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}
	//else {
	//	cout << "���ӷ���˳ɹ�" << endl;
	//}
	cout << "���ӳɹ�" << endl;

	char mes[1024] = { 0 };
	char recvbuf[1024] = { 0 };
	char buf[1024] = { 0 };
	int send_result = 0;
	int recv_result = 0;
	recv_result = recv_result = recv(sockClient, buf, 1024, 0);
	int id = (int)buf[0];
	int amount = (int)buf[1];
	cout << id << endl;
	cout << amount << endl;
	/*���ͻ��˷���overʱ������������Ϣ������exitʱ�������Ի�*/
	int i = 1;//i�����Ŀǰ���ĸ��ͻ��˷���Ϣ
	while (1) {
		if (1 == id) {//���ȷ�����Ϣ
			while (1) {//�ͻ��˷���Ϣ
				cout << "client1��";
				memset(mes, '\0', sizeof(mes));
				time_t  t;
				char  buf[128];
				memset(buf, 0, sizeof(buf));
				struct tm* tmp;
				t = time(NULL);
				tmp = localtime(&t);
				strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmp);
				cin.getline(mes, sizeof(mes));
				strcat(buf, " ");
				strcat(buf, mes);
				send_result = send(sockClient, buf, (int)strlen(buf), 0);
				if (send_result == SOCKET_ERROR) {
					cout << "������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
					closesocket(sockClient);
					WSACleanup();
					return 1;
				}
				//cout << "������Ϣ��" << mes << endl;
				if (strcmp(mes, "exit") == 0)
				{
					cout << "����Ⱥ��" << endl;
					closesocket(sockClient);
					WSACleanup();
					return 1;
				}
				else if (strcmp(mes, "over") == 0) {
					i++;
					break;
				}
			}
			while (1) {//�ͻ��˽�����Ϣ
				memset(recvbuf, '\0', sizeof(recvbuf));
				recv_result = recv(sockClient, recvbuf, 1024, 0);
				if (recv_result > 0) {
					if (strcmp(recvbuf, "exit") == 0) {
						cout << "�ͻ���" << i << "����Ⱥ��" << endl;
						closesocket(sockClient);
						WSACleanup();
						return 1;
					}
					else if (strcmp(recvbuf, "over") == 0) {
						if (i != amount) {
							i++;
						}
						else {
							i = 1;
							break;
						}
					}
					cout << "client" << i << "��" << recvbuf << endl;
				}
				//cout << "�յ���Ϣ��" << recvbuf << endl;
				else if (recv_result == 0)
					cout << "���ӹر�" << endl;
				else {
					cout << "�յ�ʧ�ܣ�" << WSAGetLastError() << endl;
					closesocket(sockClient);
					WSACleanup();
					return 1;
				}
			}
		}
		else {//�������
			while (1) {//�ͻ����Ƚ�����Ϣ
				memset(recvbuf, '\0', sizeof(recvbuf));
				recv_result = recv(sockClient, recvbuf, 1024, 0);
				if (recv_result > 0) {
					if (strcmp(recvbuf, "exit") == 0) {
						cout << "�ͻ���" << i << "����Ⱥ��" << endl;
						closesocket(sockClient);
						WSACleanup();
						return 1;
					}
					else if (strcmp(recvbuf, "over") == 0) {
						if (i == id - 1) {
							i = id;
							break;
						}
						else {
							if (i == amount) {
								i = 1;
							}
							else {
								i++;
							}
						}
					}
					else {
						cout << "client" << i << "��" << recvbuf << endl;
					}
				}
				//cout << "�յ���Ϣ��" << recvbuf << endl;
				else if (recv_result == 0)
					cout << "���ӹر�" << endl;
				else {
					cout << "�յ�ʧ�ܣ�" << WSAGetLastError() << endl;
					closesocket(sockClient);
					WSACleanup();
					return 1;
				}
			}
			while (1) {//�ͻ����ٷ�����Ϣ
				cout << "client" << id << "��";
				memset(mes, '\0', sizeof(mes));
				cin.getline(mes, sizeof(mes));
				send_result = send(sockClient, mes, (int)strlen(mes), 0);
				if (send_result == SOCKET_ERROR) {
					cout << "������Ϣʧ�ܣ�" << WSAGetLastError() << endl;
					closesocket(sockClient);
					WSACleanup();
					return 1;
				}
				//cout << "������Ϣ��" << mes << endl;
				if (strcmp(mes, "exit") == 0)
				{
					cout << "����Ⱥ��" << endl;
					closesocket(sockClient);
					WSACleanup();
					return 1;
				}
				else if (strcmp(mes, "over") == 0) {
					if (i == amount) {
						i = 1;
					}
					else {
						i++;
					}
					break;
				}
			}
		}
	}

	closesocket(sockClient);
	WSACleanup();
	return 0;
}