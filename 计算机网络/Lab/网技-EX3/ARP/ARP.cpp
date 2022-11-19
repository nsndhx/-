#include "hhh.h"
#include <IPHlpApi.h>
#pragma comment(lib,"wpcap.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"IPHlpApi.lib")    // ����ARP����Ҫ�õľ�̬��,ȡMAC��
#define ARP_REQUEST  0x0001        // ARP����
#define ARP_REPLY    0x0002           // ARPӦ��
#define IPTOSBUFFERS 12
#define HOSTNUM      255      // ��������

#pragma warning(disable:4996)
//#pragma warning(disable:4700)

#pragma pack(1)
typedef struct Ethernet_head {	//֡�ײ�
	BYTE	DesMAC[6];	// Ŀ�ĵ�ַ+
	BYTE 	SrcMAC[6];	// Դ��ַ+
	WORD	EthType;	// ֡����
};
typedef struct ArpPacket {	//����֡�ײ���ARP�ײ������ݰ�
	Ethernet_head	ed;
	WORD HardwareType; //Ӳ������
	WORD ProtocolType; //Э������
	BYTE HardwareAddLen; //Ӳ����ַ����
	BYTE ProtocolAddLen; //Э���ַ����
	WORD OperationField; //�������ͣ�ARP����1����ARPӦ��2����RARP����3����RARPӦ��4����
	BYTE SourceMacAdd[6]; //Դmac��ַ
	DWORD SourceIpAdd; //Դip��ַ
	BYTE DestMacAdd[6]; //Ŀ��mac��ַ
	DWORD DestIpAdd; //Ŀ��ip��ַ
};
#pragma pack()


char* myBroad;
unsigned char* m_MAC = new unsigned char[6];
char* m_IP = (char*)"10.130.86.122";
char* m_mask;
char d_IP[20];
//char* d_IP = (char*)"192.168.137.1";
unsigned char* d_MAC = new unsigned char[6];
bool flag;

char* iptos(u_long in) {
	static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
	static short which;
	u_char* p;

	p = (u_char*)&in;
	which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
	sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return output[which];
}

// ��ӡ���п�����Ϣ
void ifprint(pcap_if_t* d) {
	pcap_addr_t* a;

	/* IP addresses */
	for (a = d->addresses; a; a = a->next)
	{
		printf("\tAddress Family: #%d\n", a->addr->sa_family);
		switch (a->addr->sa_family)
		{
		case AF_INET:
			printf("\tAddress Family Name: AF_INET\n");
			if (a->addr)
			{
				m_IP = iptos(((struct sockaddr_in*)a->addr)->sin_addr.s_addr);
				printf("\tIP Address: %s\n", m_IP);
			}
			if (a->netmask)
			{
				m_mask = iptos(((struct sockaddr_in*)a->netmask)->sin_addr.s_addr);
				printf("\tNetmask: %s\n", m_mask);
			}

			if (a->broadaddr)
			{
				myBroad = iptos(((struct sockaddr_in*)a->broadaddr)->sin_addr.s_addr);
				printf("\tBroadcast Address: %s\n", myBroad);
			}
			if (a->dstaddr)
				printf("\tDestination Address: %s\n", iptos(((struct sockaddr_in*)a->dstaddr)->sin_addr.s_addr));
			break;
		default:
			//printf("\tAddress Family Name: Unknown\n");
			break;
		}
	}
	printf("\n");
}

char* GetSelfMac() {
	ULONG MacAddr[2] = { 0 };    // Mac��ַ����6�ֽ�
	ULONG uMacSize = 6;
	DWORD dwRet = SendARP(inet_addr(m_IP), 0, &MacAddr, &uMacSize);
	if (dwRet == NO_ERROR)
	{
		BYTE* bPhyAddr = (BYTE*)MacAddr;

		if (uMacSize)
		{
			char* sMac = (char*)malloc(sizeof(char) * 18);
			int n = 0;

			memset(sMac, 0, 18);
			sprintf_s(sMac, (size_t)18, "%.2X-%.2X-%.2X-%.2X-%.2X-%.2X", (int)bPhyAddr[0], (int)bPhyAddr[1], (int)bPhyAddr[2], (int)bPhyAddr[3], (int)bPhyAddr[4], (int)bPhyAddr[5]);
			return sMac;
		}
		else
		{
			printf("Mac��ַ��ȡʧ�ܣ�\n");
		}
	}
	else
	{
		printf("ARP���ķ���ʧ��:%d\n", dwRet);
	}
	return NULL;
}

void SendArpPacket(pcap_t* adhandle) {
	char* ip = m_IP;
	unsigned char* mac = m_MAC;
	char* netmask = m_mask;
	printf("ip_mac:%02x-%02x-%02x-%02x-%02x-%02x\n", mac[0], mac[1], mac[2],
		mac[3], mac[4], mac[5]);
	printf("�����IP��ַΪ:%s\n", ip);
	printf("��ַ����NETMASKΪ:%s\n", netmask);
	printf("\n");
	unsigned char* sendbuf; //arp���ṹ��С
	ArpPacket arp;
	
	//��ֵMAC��ַ
	memset(arp.ed.DesMAC, 0xff, 6);       //Ŀ�ĵ�ַΪȫΪ�㲥��ַ
	printf("Des_MAC:%02x-%02x-%02x-%02x-%02x-%02x\n", arp.ed.DesMAC[0], arp.ed.DesMAC[1], arp.ed.DesMAC[2],
		arp.ed.DesMAC[3], arp.ed.DesMAC[4], arp.ed.DesMAC[5]);
	memcpy(arp.ed.SrcMAC, mac, 6);
	printf("Src_MAC:%02x-%02x-%02x-%02x-%02x-%02x\n", arp.ed.SrcMAC[0], arp.ed.SrcMAC[1], arp.ed.SrcMAC[2],
		arp.ed.SrcMAC[3], arp.ed.SrcMAC[4], arp.ed.SrcMAC[5]);
	memcpy(arp.SourceMacAdd, mac, 6);
	printf("SourceMacAdd:%02x-%02x-%02x-%02x-%02x-%02x\n", arp.SourceMacAdd[0], arp.SourceMacAdd[1], arp.SourceMacAdd[2],
		arp.SourceMacAdd[3], arp.SourceMacAdd[4], arp.SourceMacAdd[5]);
	memset(arp.DestMacAdd, 0x00, 6);
	printf("DestMacAdd:%02x-%02x-%02x-%02x-%02x-%02x\n", arp.DestMacAdd[0], arp.DestMacAdd[1], arp.DestMacAdd[2],
		arp.DestMacAdd[3], arp.DestMacAdd[4], arp.DestMacAdd[5]);
	arp.ed.EthType = htons(0x0806);//֡����ΪARP3
	cout << "EthTyte:" << arp.ed.EthType << endl;
	arp.HardwareType = htons(0x0001);
	cout << "HardwareType:" << arp.HardwareType << endl;
	arp.ProtocolType = htons(0x0800);
	cout << "ProtocolType:" << arp.ProtocolType << endl;
	arp.HardwareAddLen = 6;
	arp.ProtocolAddLen = 4;
	arp.SourceIpAdd = inet_addr(ip); //���󷽵�IP��ַΪ�����IP��ַ
	cout << "SourceIpAss:" << arp.SourceIpAdd << endl;
	arp.OperationField = htons(0x0001);
	cout << "OperationField:" << arp.OperationField << endl;
	arp.DestIpAdd = inet_addr(d_IP);
	cout << "DestIpAdd:" << arp.DestIpAdd << endl;
	u_char* a = (u_char*)&arp;
	//������ͳɹ�
	cout << "a:";
	for (int i = 0; i < 48; i++) {
		printf("% 02x ", a[i]);
	}
	cout << endl;
	if (pcap_sendpacket(adhandle, (u_char*)&arp, sizeof(ArpPacket)) == 0) {
		printf("\nPacketSend succeed\n");
	}
	else {
		printf("PacketSendPacket in getmine Error: %d\n", GetLastError());
	}
	flag = TRUE;
	return;
}

//(pcap_t *adhandle)
int GetLivePC(pcap_t* adhandle) {
	//gparam *gpara = (gparam *)lpParameter;
	//pcap_t *adhandle = gpara->adhandle;
	int res;
	unsigned char Mac[6];
	struct pcap_pkthdr* pkt_header;
	const u_char* pkt_data;
	while ((res = pcap_next_ex(adhandle, &pkt_header, &pkt_data)) >= 0) {
		//cout << "ETH_ARP:" << *(unsigned short*)(pkt_data + 12) << "        " << htons(0x0806) << endl;
		if (*(unsigned short*)(pkt_data + 12) == htons(0x0806)) {
			ArpPacket* recv = (ArpPacket*)pkt_data;
			//cout << "ARP_REPLY:" << *(unsigned short*)(pkt_data + 20) << "        " << htons(ARP_REPLY) << endl;
			if (*(unsigned short*)(pkt_data + 20) == htons(ARP_REPLY)) {
				printf("sourceIP��ַ:%d.%d.%d.%d   MAC��ַ:",
					*(unsigned char*)(pkt_data + 28) & 255,
					*(unsigned char*)(pkt_data + 28 + 1) & 255,
					*(unsigned char*)(pkt_data + 28 + 2) & 255,
					*(unsigned char*)(pkt_data + 28 + 3) & 255);
				for (int i = 0; i < 6; i++) {
					Mac[i] = *(unsigned char*)(pkt_data + 22 + i);
					printf("%02x ", Mac[i]);
				}
				printf("\n");
				printf("destinationIP��ַ:%d.%d.%d.%d   MAC��ַ:",
					*(unsigned char*)(pkt_data + 38) & 255,
					*(unsigned char*)(pkt_data + 38 + 1) & 255,
					*(unsigned char*)(pkt_data + 38 + 2) & 255,
					*(unsigned char*)(pkt_data + 38 + 3) & 255);
				for (int i = 0; i < 6; i++) {
					Mac[i] = *(unsigned char*)(pkt_data + 32 + i);
					printf("%02x ", Mac[i]);
				}
				printf("\n");
				return 0;
			}
		}
		Sleep(10);
	}
	return 0;
}


int main() {
	//�����ӿں�IP��ַ�Ļ�ȡ
	pcap_if_t* alldevs; 	               //ָ���豸�����ײ���ָ��
	pcap_if_t* d;
	pcap_addr_t* a;
	int i = 0;
	int inum = 0;
	pcap_t* adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];	//������Ϣ������
	//��ñ������豸�б�
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, 	//��ȡ�����Ľӿ��豸
		NULL,			       //������֤
		&alldevs, 		       //ָ���豸�б��ײ�
		errbuf			      //������Ϣ���滺����
	) == -1)
	{
		//������
		cout << "��ȡ�����豸����:" << errbuf << endl;
		pcap_freealldevs(alldevs);
		return 0;
	}
	//��ʾ�ӿ��б�
	for (d = alldevs; d != NULL; d = d->next)
	{
		cout << dec << ++i << ": " << d->name; //����d->name��ȡ������ӿ��豸������
		if (d->description) { //����d->description��ȡ������ӿ��豸��������Ϣ
			cout << d->description << endl;
		}
		else {
			cout << "�����������Ϣ" << endl;
			return -1;
		}
	}
	if (i == 0)
	{
		cout << "wrong!" << endl;
		return -1;
	}

	cout << "������Ҫ�򿪵����ںţ�1-" << i << "����";
	cin >> inum;

	//����û��Ƿ�ָ������Ч���豸
	if (inum < 1 || inum > i)
	{
		cout << "����������������Χ" << endl;

		pcap_freealldevs(alldevs);
		return -1;
	}

	//��ת��ѡ�����豸
	for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);
	ifprint(d);

	//������
	if ((adhandle = pcap_open(d->name,          // �豸��
		65536,            // Ҫ��׽�����ݰ��Ĳ���
						  // 65535��֤�ܲ��񵽲�ͬ������·���ϵ�ÿ�����ݰ���ȫ������
		PCAP_OPENFLAG_PROMISCUOUS,    // ����ģʽ
		1000,             // ��ȡ��ʱʱ��
		NULL,             // Զ�̻�����֤
		errbuf            // ���󻺳��
	)) == NULL)
	{
		cout << "�޷����豸������Ƿ���֧�ֵ�NPcap" << endl;
		pcap_freealldevs(alldevs);
		return -1;
	}
	/* �ͷ��豸�б� */
	//pcap_freealldevs(alldevs);

	printf("\nlistening on %s...\n", d->description);
	m_MAC = (unsigned char*)GetSelfMac();
	printf("����Ŀ��IP:");
	scanf("%s", &d_IP);
	//HANDLE sendthread;      //����ARP���߳�
	//HANDLE recvthread;       //����ARP���߳�

	//sendthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendArpPacket, adhandle, 0, NULL);
	//recvthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetLivePC, adhandle, 0, NULL);

	SendArpPacket(adhandle);
	GetLivePC(adhandle);

	pcap_freealldevs(alldevs);
	//CloseHandle(sendthread);
	//CloseHandle(recvthread);
	return 0;
}