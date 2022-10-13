#include<iostream>
#include "pcap.h"
#pragma comment(lib,"wpcap.lib")
#pragma comment(lib,"packet.lib")
using namespace std;

#pragma pack(1)		//�����ֽڶ��뷽ʽ
typedef struct FrameHeader_t {	//֡�ײ�
	BYTE	DesMAC[6];	// Ŀ�ĵ�ַ
	BYTE 	SrcMAC[6];	// Դ��ַ
	WORD	FrameType;	// ֡����
} FrameHeader_t;
typedef struct IPHeader_t {		//IP�ײ�
	BYTE	Ver_HLen;
	BYTE	TOS;
	WORD	TotalLen;
	WORD	ID;
	WORD	Flag_Segment;
	BYTE	TTL;
	BYTE	Protocol;
	WORD	Checksum;
	ULONG	SrcIP;
	ULONG	DstIP;
} IPHeader_t;
typedef struct Data_t {	//����֡�ײ���IP�ײ������ݰ�
	FrameHeader_t	FrameHeader;
	IPHeader_t		IPHeader;
} Data_t;
#pragma pack()	//�ָ�ȱʡ���뷽ʽ

typedef struct pcap_if pcap_if_t;
struct pcap_if {
	struct pcap_if* next;
	char* name;
	char* description;
	struct pcap_addr* addresses;
	u_int flags;
};
struct pcap_addr {
	struct pcap_addr* next;
	struct sockaddr* addr;
	struct sockaddr* netmask;
	struct sockaddr* broadaddr;
	struct sockaddr* dstaddr;
};
//��ȡ�豸�б�
int pcap_findalldevs_ex(
		char *source,
		struct	pcap_rmtauth auth,
		pcap_if_t **alldevs,
		char *errbuf
);

int main() {
	//�����ӿں�IP��ַ�Ļ�ȡ
	pcap_if_t* alldevs; 	               //ָ���豸�����ײ���ָ��
	pcap_if_t* d;
	pcap_addr_t* a;
	char		errbuf[PCAP_ERRBUF_SIZE];	//������Ϣ������
	//��ñ������豸�б�
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, 	//��ȡ�����Ľӿ��豸
		NULL,			       //������֤
		&alldevs, 		       //ָ���豸�б��ײ�
		errbuf			      //������Ϣ���滺����
	) == -1)
	{
		//������
		cout << "��ȡ�����豸����" << endl;
		exit(1);
	}
			
	for (d = alldevs; d != NULL; d = d->next)      //��ʾ�ӿ��б�
	{
		//����	//����d->name��ȡ������ӿ��豸������
			//����	//����d->description��ȡ������ӿ��豸��������Ϣ
			//��ȡ������ӿ��豸��IP��ַ��Ϣ
			for (a = d->addresses; a != NULL; a = addr->next)
				if (a->addr->sa_family == AF_INET)  //�жϸõ�ַ�Ƿ�IP��ַ
				{
					//����	//����a->addr��ȡIP��ַ
						//����	//����a->netmask��ȡ��������
						//����	//����a->broadaddr��ȡ�㲥��ַ
						//����	//����a->dstaddr)��ȡĿ�ĵ�ַ
				}
	}
	pcap_freealldevs(alldevs); //�ͷ��豸�б�

	system("pause");
	return 0;
}