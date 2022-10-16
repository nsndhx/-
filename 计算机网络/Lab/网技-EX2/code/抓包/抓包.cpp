#include<iostream>
#include"pcap.h"
#include<iomanip>
#include<WS2tcpip.h>
#include<windows.h>
#include<cstdlib>
#pragma comment(lib,"wpcap.lib")
#pragma comment(lib,"packet.lib")
#pragma comment(lib,"wsock32.lib")
#pragma comment(lib,"ws2_32.lib")
#define LINE_LEN 16
#define MAX_ADDR_LEN 16
using namespace std;

#pragma pack(1)		//�����ֽڶ��뷽ʽ
typedef struct FrameHeader_t {	//֡�ײ�
	BYTE	DesMAC[6];	// Ŀ�ĵ�ַ
	BYTE 	SrcMAC[6];	// Դ��ַ
	WORD	FrameType;	// ֡����
} FrameHeader_t;
typedef struct IPHeader_t {		//IP�ײ�
	BYTE Ver_HLen;//�汾
	BYTE TOS;//��������
	WORD TotalLen;//�ܳ���
	WORD ID;//��ʶ
	WORD Flag_Segment;//��־ Ƭƫ��
	BYTE TTL;//��������
	BYTE Protocol;//Э��
	WORD Checksum;//ͷ��У���
	u_int SrcIP;//ԴIP
	u_int DstIP;//Ŀ��IP
} IPHeader_t;
typedef struct Data_t {	//����֡�ײ���IP�ײ������ݰ�
	FrameHeader_t	FrameHeader;
	IPHeader_t		IPHeader;
} Data_t;
#pragma pack()	//�ָ�ȱʡ���뷽ʽ

void ip_protocol_packet_handle(const struct pcap_pkthdr* pkt_header, const u_char* pkt_data)
{
	IPHeader_t* IPHeader;
	IPHeader = (IPHeader_t*)(pkt_data + 14);
	sockaddr_in source, dest;
	char sourceIP[MAX_ADDR_LEN], destIP[MAX_ADDR_LEN];
	char str[16];
	source.sin_addr.s_addr = IPHeader->SrcIP;
	dest.sin_addr.s_addr = IPHeader->DstIP;
	strncpy_s(sourceIP, inet_ntop(AF_INET,&source.sin_addr,str,16), MAX_ADDR_LEN);
	strncpy_s(destIP, inet_ntop(AF_INET,&dest.sin_addr,str,16), MAX_ADDR_LEN);

	//��ʼ���
	cout << dec << "Version��" << (int)(IPHeader->Ver_HLen >> 4) << endl;
	cout << "Header Length��";
	cout << (int)((IPHeader->Ver_HLen & 0x0f) * 4) << " Bytes" << endl;
	cout << "Tos��" << (int)IPHeader->TOS << endl;
	cout << "Total Length��" << (int)ntohs(IPHeader->TotalLen) << endl;
	cout << "Identification��0x" << hex << setw(4) << setfill('0') << ntohs(IPHeader->ID) << endl;
	cout << "Flags��" << dec << (int)(ntohs(IPHeader->Flag_Segment)) << endl;
	cout << "Time to live��" << (int)IPHeader->TTL << endl;
	cout << "Protocol Type�� ";
	switch (IPHeader->Protocol)
	{
	case 1:
		cout << "ICMP";
		break;
	case 6:
		cout << "TCP";
		break;
	case 17:
		cout << "UDP";
		break;
	default:
		break;
	}
	cout << "(" << (int)IPHeader->Protocol << ")" << endl;
	cout << "Header checkSum��0x" << hex << setw(4) << setfill('0') << ntohs(IPHeader->Checksum) << endl;
	cout << "Source��" << sourceIP << endl;
	cout << "Destination��" << destIP << endl;

}
void ethernet_protocol_packet_handle(u_char* param, const struct pcap_pkthdr* pkt_header, const u_char* pkt_data)
{
	FrameHeader_t* ethernet_protocol;//��̫��Э��
	u_short ethernet_type;			//��̫������
	u_char* mac_string;				//��̫����ַ

	//��ȡ��̫����������
	ethernet_protocol = (FrameHeader_t*)pkt_data;
	ethernet_type = ntohs(ethernet_protocol->FrameType);

	cout << "==============Ethernet Protocol=================" << endl;

	//��̫��Ŀ���ַ
	mac_string = ethernet_protocol->DesMAC;

	cout << "Destination Mac Address�� ";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[0] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[1] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[2] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[3] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[4] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[5] << endl;

	//��̫��Դ��ַ
	mac_string = ethernet_protocol->SrcMAC;

	cout << "Source Mac Address�� ";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[0] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[1] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[2] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[3] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[4] << ":";
	cout << hex << setw(2) << setfill('0') << (u_int)mac_string[5] << endl;

	cout<<"Ethernet type�� ";
	switch (ethernet_type)
	{
	case 0x0800:
		cout << "IP";
		break;
	case 0x0806:
		cout << "ARP";
		break;
	case 0x0835:
		cout << "RARP";
		break;
	default:
		cout << "Unknown Protocol";
		break;
	}
	cout << " 0x" << setw(4) << setfill('0') << ethernet_type << endl;

	//����IPHeader������
	if (ethernet_type == 0x0800)
	{
		ip_protocol_packet_handle(pkt_header, pkt_data);
	}
}

int main() {
	//�����ӿں�IP��ַ�Ļ�ȡ
	pcap_if_t* alldevs; 	               //ָ���豸�����ײ���ָ��
	pcap_if_t* d;
	//pcap_addr_t* a;
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
			
	for (d = alldevs; d != NULL; d = d->next) //��ʾ�ӿ��б�
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
	
	cout << "������" << d->description << endl;
	pcap_freealldevs(alldevs);
	int cnt = -1;
	cout << "��Ҫ�������ݰ��ĸ�����";
	cin >> cnt;
	pcap_loop(adhandle, cnt, ethernet_protocol_packet_handle, NULL);
	pcap_close(adhandle);

	system("pause");
	return 0;
}