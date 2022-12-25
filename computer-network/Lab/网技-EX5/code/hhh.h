#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <conio.h>
#include <iomanip>
#include <WS2tcpip.h>
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream> 

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
#pragma pack(1)
typedef struct FrameHeader_t {//֡�ײ�
	BYTE DesMAC[6];//Ŀ�ĵ�ַ
	BYTE SrcMAC[6];//Դ��ַ
	WORD FrameType;//֡����
}FrameHeader_t;

typedef struct ARPFrame_t {
	FrameHeader_t FrameHeader;//֡�ײ�
	WORD HardwareType;//Ӳ������
	WORD ProtocolType;//Э������
	BYTE HLen;//Ӳ����ַ����
	BYTE PLen;//Э���ַ
	WORD Operation;//����
	BYTE SendHa[6];//���ͷ�MAC
	DWORD SendIP;//���ͷ�IP
	BYTE RecvHa[6];//���շ�MAC
	DWORD RecvIP;//���շ�IP
}ARPFrame_t;

typedef struct IPHeader_t {//IP�ײ�
	BYTE Ver_HLen;
	BYTE TOS;
	WORD TotalLen;
	WORD ID;
	WORD Flag_Segment;
	BYTE TTL;//��������
	BYTE Protocol;
	WORD Checksum;//У���
	ULONG SrcIP;//ԴIP
	ULONG DstIP;//Ŀ��IP
}IPHeader_t;

typedef struct Data_t {//����֡�ײ���IP�ײ������ݰ�
	FrameHeader_t FrameHeader;//֡�ײ�
	IPHeader_t IPHeader;//IP�ײ�
}Data_t;

typedef struct ICMP_t {//����֡�ײ���IP�ײ������ݰ�
	FrameHeader_t FrameHeader;
	IPHeader_t IPHeader;
	char buf[0x80];
}ICMP_t;
#pragma pack()
