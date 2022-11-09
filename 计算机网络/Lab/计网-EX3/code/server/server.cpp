#include"define.h"

#pragma warning(disable : 6031)
#pragma warning(disable : 4996)

#define TIMEOUT 5  //��ʱ����λs������һ���е�����ack����ȷ����
#define WINDOWSIZE 20 //�������ڴ�С

SOCKADDR_IN addrServer;   //��������ַ
SOCKADDR_IN addrClient;   //�ͻ��˵�ַ

SOCKET sockServer;//�������׽���
SOCKET sockClient;//�ͻ����׽���

auto ack = vector<int>(WINDOWSIZE);
int totalack = 0;//��ȷȷ�ϵ����ݰ�����
int curseq = 0;//��ǰ���͵����ݰ������к�
int curack = 0;//��ǰ�ȴ���ȷ�ϵ����ݰ������кţ���С��

//��ʼ������
void inithandler()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	//�׽��ּ���ʱ������ʾ 
	int err;
	//�汾 2.2 
	wVersionRequested = MAKEWORD(2, 2);
	//���� dll �ļ� Scoket ��   
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//�Ҳ��� winsock.dll 
		cout << "WSAStartup failed with error: " << err << endl;
		return;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		cout << "Could not find a usable version of Winsock.dll" << endl;
		WSACleanup();
	}
	sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//�����׽���Ϊ������ģʽ 
	int iMode = 1; //1����������0������ 
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) & iMode);//���������� 

	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(8080);
	err = bind(sockServer, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	if (err) {
		err = GetLastError();
		cout << "Could  not  bind  the  port" << 8080 << "for  socket. Error  code is" << err << endl;
		WSACleanup();
		return;
	}
	else
	{
		cout << "�����������ɹ�" << endl;
	}
	for (int i = 0; i < WINDOWSIZE; i++)
	{
		ack[i] = 1;//��ʼ�����Ϊ1
	}
}

//��ʱ�ش�
/*
void timeouthandler()
{
	packet* pkt1 = new packet;
	pkt1->init_packet();
	for (int i = curack; i != curseq; i = (i++) % seqnumber)
	{
		memcpy(pkt1, &buffer[i % WINDOWSIZE], BUFFER);
		sendto(sockServer, (char*)pkt1, BUFFER, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
		cout << "�ش��� " << i << " �����ݰ�" << endl;
	}
	ssthresh = cwnd / 2;
	cwnd = 1;
	STATE = SLOWSTART;//��⵽��ʱ���ͻص�������״̬
	cout << "==========================��⵽��ʱ���ص��������׶�============================" << endl;
	cout << "cwnd=  " << cwnd << "     sstresh= " << ssthresh << endl << endl;
}
*/


//�����
void ackhandler(unsigned int a) {

}


//�������ƣ�ͣ�Ȼ���

int main() {
	//��ʼ��
	inithandler();
	char filepath[20];//�ļ�·��
	int totalpacket = 0;
	//��ȡ�ļ�
	cout << "������Ҫ���͵��ļ�·����";
	cin >> filepath;
	ifstream is(filepath, ifstream::in | ios::binary);//�Զ����Ʒ�ʽ���ļ�
	if (!is.is_open()) {
		cout << "�ļ��޷���!" << endl;
		exit(1);
	}
	is.seekg(0, std::ios_base::end);  //���ļ���ָ�붨λ������ĩβ
	int length1 = is.tellg();
	totalpacket = length1 / 1024 + 1;
	cout << "�ļ���СΪ" << length1 << "Bytes,�ܹ���" << totalpacket << "�����ݰ�" << endl;
	is.seekg(0, std::ios_base::beg);  //���ļ���ָ�����¶�λ�����Ŀ�ʼ
	//��������
	packet* pkt = new packet;
	char t[1024] = { 0 };
	int recv_result = 0;
	int send_result = 0;
	
	while (true) {
		 memset(pkt, '\0', sizeof(*pkt));
		 recv_result = recv(sockServer, (char*)pkt, sizeof(packet), 0);
		 int count = 0;
		 int waitcount = 0;
		 while (recv_result < 0)
		 {
			 count++;
			 Sleep(100);
			 if (count > 20)
			 {
				 cout << "��ǰû�пͻ����������ӣ�" << endl;
				 count = 0;
				 break;
			 }
		 }
		 //�������յ��ͻ��˷�����TAG=0�����ݱ�����ʶ��������
		 if (pkt->tag == 0) {
			 clock_t st = clock();
			 cout << "��ʼ��������" << endl;
			 int stage = 0;
			 //bool runFlag = true;
			 //int waitCount = 0;
			 while (1) {
				 if (stage == 0) {
					 //����100(�ڶ�������)
					 pkt = connecthandler(100, totalpacket);
					 send(sockServer, (char*)pkt, sizeof(packet), 0);
					 Sleep(100);
					 stage = 1;
				 }
				 if (stage == 1) {
					 //�ȴ�����200�׶�(��ʼ����)
					 memset(pkt, '\0', sizeof(*pkt));
					 recv_result = recv(sockServer, (char*)pkt, sizeof(packet), 0);
					 if (recv_result < 0) {
						 //++waitCount;
						 //runFlag = false;
						 cout << "connected false" << endl;
						 break;
					 }
					 else {
						 if (pkt->tag == 200) {
							 //�����ļ�
							 pkt->init_packet();
							 cout << "sending file" << endl;
							 memcpy(pkt->data, filepath, strlen(filepath));
							 pkt->len = strlen(filepath);
							 send(sockServer, (char*)pkt, sizeof(packet), 0);
							 stage = 2;
						 }
					 }
				 }
				 if (stage == 2) {
					 //ȷ�϶Է����ܳɹ�
					 if (totalack == totalpacket) {
						 pkt->init_packet();
						 pkt->tag = 88;
						 cout << "���ݴ������" << endl;
						 send(sockServer, (char*)pkt, sizeof(packet), 0);
						 exit(0);
						 break;
					 }
					 //�ȴ�ȷ�Ͻ��ܵ����ݰ�
					 pkt->init_packet();
					 recv_result = recv(sockServer, (char*)pkt, sizeof(packet), 0);
					 if (recv_result < 0) {
						 waitcount++;
						 Sleep(200);
						 if (waitcount > 20)
						 {
							 pkt->init_packet();
							 cout << "sending file" << endl;
							 memcpy(pkt->data, filepath, strlen(filepath));
							 pkt->len = strlen(filepath);
							 send(sockServer, (char*)pkt, sizeof(packet), 0);

							 waitcount = 0;
						 }
					 }
					 else {
						 //ȷ��Ӧ��
						 if () {

						 }
					 }
				 }

			 }
		 }
		 
	}
	system("pause");
	return 0;
}