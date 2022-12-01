#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <conio.h>
#include<iomanip>
#include<WS2tcpip.h>
#include<cstdlib>

#define HAVE_REMOTE
#include <pcap.h>

#include <WS2tcpip.h>
//#include <remote-ext.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
#define _AFXDLL
#define LINE_LEN 16
#define MAX_ADDR_LEN 16

using namespace std;

#pragma pack(1)
typedef struct FrameHeader_t {	//֡�ײ�
	BYTE	DesMAC[6];	// Ŀ�ĵ�ַ+
	BYTE 	SrcMAC[6];	// Դ��ַ+
	WORD	FrameType;	// ֡����
} FrameHeader_t;
typedef struct ArpPacket {	//����֡�ײ���ARP�ײ������ݰ�
	FrameHeader_t	ed;
	WORD HardwareType; //Ӳ������
	WORD ProtocolType; //Э������
	BYTE HardwareAddLen; //Ӳ����ַ����
	BYTE ProtocolAddLen; //Э���ַ����
	WORD OperationField; //�������ͣ�ARP����1����ARPӦ��2����RARP����3����RARPӦ��4����
	BYTE SourceMacAdd[6]; //Դmac��ַ
	DWORD SourceIpAdd; //Դip��ַ
	BYTE DestMacAdd[6]; //Ŀ��mac��ַ
	DWORD DestIpAdd; //Ŀ��ip��ַ
} ArpPacket;
typedef struct IPHeader_t {		//IP�ײ�
	BYTE Ver_HLen; //IPЭ��汾��IP�ײ����ȡ���4λΪ�汾����4λΪ�ײ��ĳ���(��λΪ4bytes)
	BYTE TOS;//��������+
	WORD TotalLen;//�ܳ���+
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
#pragma pack()

