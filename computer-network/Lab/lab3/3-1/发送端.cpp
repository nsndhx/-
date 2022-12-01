#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#include <cstdio>
#include <windows.h>
#include <iostream>
#include <thread>
#include <math.h>
#pragma comment(lib, "ws2_32.lib")  //���� ws2_32.dll
#pragma comment(lib, "winmm.lib")
#pragma warning(disable : 4996)

using namespace std;

#define SYN 0x1
#define ACK 0x2
#define FIN 0x4
#define END 0x8
#define PORT 7879
#define ADDRSRV "127.0.0.1"
#define MAX_DATA_SIZE 2048

double MAX_TIME = CLOCKS_PER_SEC;
int seqsize = 2;
int curseq = 0;
int curack = 1;

struct packetHead {
    u_int seq;
    u_int ack;
    u_short checkSum;
    u_short bufSize;
    char flag;

    packetHead() {
        seq = ack = 0;
        checkSum = bufSize = 0;
        flag = 0;
    }
};

struct packet {
    packetHead head;
    char data[MAX_DATA_SIZE];
};

u_short checkPacketSum(u_short* packet, int packetLen) {

    u_long sum = 0;
    int count = (packetLen + 1) / 2;

    u_short* temp = new u_short[count];
    memset(temp, 0, 2 * count);
    memcpy(temp, packet, packetLen);

    while (count--) {
        sum += *temp++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF);
}

bool connectToServer(SOCKET& socket, SOCKADDR_IN& addr) {
    int len = sizeof(addr);

    packetHead head;
    head.flag |= SYN;
    head.seq = 0;
    head.checkSum = checkPacketSum((u_short*)&head, sizeof(head));

    char* buffer = new char[sizeof(head)];
    memcpy(buffer, &head, sizeof(head));
    sendto(socket, buffer, sizeof(head), 0, (sockaddr*)&addr, len);
    cout << "��һ�����ֳɹ�" << endl;

    clock_t start = clock(); //��ʼ��ʱ
    while (recvfrom(socket, buffer, sizeof(head), 0, (sockaddr*)&addr, &len) <= 0) {
        if (clock() - start >= MAX_TIME) {
            memcpy(buffer, &head, sizeof(head));
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr*)&addr, len);
            start = clock();
        }
    }

    memcpy(&head, buffer, sizeof(head));
    if ((head.flag & ACK) && (checkPacketSum((u_short*)&head, sizeof(head)) == 0)) {
        cout << "�ڶ������ֳɹ�" << endl;
    }
    else {
        return false;
    }

    //��������������
    if (head.flag & SYN) {
        head.flag = 0;
        head.flag |= ACK;
        head.checkSum = 0;
        head.checkSum = (checkPacketSum((u_short*)&head, sizeof(head)));
    }
    else {
        return false;
    }
    memcpy(buffer, &head, sizeof(head));
    sendto(socket, buffer, sizeof(head), 0, (sockaddr*)&addr, len);

    //�ȴ�����MAX_TIME������յ���Ϣ˵��ACKû�ж���
    start = clock();
    while (clock() - start <= 2 * MAX_TIME) {
        if (recvfrom(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, &len) <= 0)
            continue;
        //˵�����ACK����
        memcpy(buffer, &head, sizeof(head));
        sendto(socket, buffer, sizeof(head), 0, (sockaddr*)&addr, len);
        start = clock();
    }
    cout << "�������ֳɹ�" << endl;
    cout << "�ɹ���������������ӣ�׼����������" << endl;
    return true;
}

bool disConnect(SOCKET& socket, SOCKADDR_IN& addr) {

    int addrLen = sizeof(addr);
    char* buffer = new char[sizeof(packetHead)];
    packetHead closeHead;
    closeHead.flag |= FIN;
    closeHead.checkSum = checkPacketSum((u_short*)&closeHead, sizeof(packetHead));

    memcpy(buffer, &closeHead, sizeof(packetHead));
    if (sendto(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen) != SOCKET_ERROR)
        cout << "��һ�λ��ֳɹ�" << endl;
    else
        return false;

    clock_t start = clock();
    while (recvfrom(socket, buffer, sizeof(packetHead), 0, (sockaddr*)&addr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIME) {
            memcpy(buffer, &closeHead, sizeof(packetHead));
            sendto(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
            start = clock();
        }
    }

    if ((((packetHead*)buffer)->flag & ACK) && (checkPacketSum((u_short*)buffer, sizeof(packetHead) == 0))) {
        cout << "�ڶ��λ��ֳɹ����ͻ����Ѿ��Ͽ�" << endl;
    }
    else {
        return false;
    }

    u_long mode = 0;
    ioctlsocket(socket, FIONBIO, &mode);//����

    recvfrom(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, &addrLen);

    if ((((packetHead*)buffer)->flag & FIN) && (checkPacketSum((u_short*)buffer, sizeof(packetHead) == 0))) {
        cout << "�������Ͽ�" << endl;
    }
    else {
        return false;
    }

    mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);

    closeHead.flag = 0;
    closeHead.flag |= ACK;
    closeHead.checkSum = checkPacketSum((u_short*)&closeHead, sizeof(packetHead));

    memcpy(buffer, &closeHead, sizeof(packetHead));
    sendto(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
    start = clock();
    while (clock() - start <= 2 * MAX_TIME) {
        if (recvfrom(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, &addrLen) <= 0)
            continue;
        //˵�����ACK����
        memcpy(buffer, &closeHead, sizeof(packetHead));
        sendto(socket, buffer, sizeof(packetHead), 0, (sockaddr*)&addr, addrLen);
        start = clock();
    }

    cout << "���Ĵλ��ֳɹ��������ѹر�" << endl;
    closesocket(socket);
    return true;
}

packet makePacket(int seq, char* data, int len) {
    packet pkt;
    pkt.head.seq = seq;
    pkt.head.bufSize = len;
    memcpy(pkt.data, data, len);
    pkt.head.checkSum = checkPacketSum((u_short*)&pkt, sizeof(packet));
    return pkt;
}

#define min(a, b) a>b?b:a

void sendFSM(u_long len, char* fileBuffer, SOCKET& socket, SOCKADDR_IN& addr) {

    int packetNum = int(len / MAX_DATA_SIZE) + (len % MAX_DATA_SIZE ? 1 : 0);
    int index = 0;
    int packetDataLen = min(MAX_DATA_SIZE, len - index * MAX_DATA_SIZE);
    int stage = 0;

    int addrLen = sizeof(addr);
    clock_t start;

    char* data_buffer = new char[packetDataLen], * pkt_buffer = new char[sizeof(packet)];
    packet sendPkt, pkt;

    cout << "�����ļ����ݳ���Ϊ" << len << "Bytes,��Ҫ����" << packetNum << "�����ݰ�" << endl;

    while (true) {
        if (index == packetNum) {
            packetHead endPacket;
            endPacket.flag |= END;
            endPacket.checkSum = checkPacketSum((u_short*)&endPacket, sizeof(packetHead));
            memcpy(pkt_buffer, &endPacket, sizeof(packetHead));
            sendto(socket, pkt_buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);

            while (recvfrom(socket, pkt_buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, &addrLen) <= 0) {
                if (clock() - start >= MAX_TIME) {
                    memcpy(pkt_buffer, &endPacket, sizeof(packetHead));
                    sendto(socket, pkt_buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
                    start = clock();
                }
            }

            if (((packetHead*)(pkt_buffer))->flag & ACK) {
                cout << "�ļ��������" << endl;
            }

            return;
        }
        packetDataLen = min(MAX_DATA_SIZE, len - index * MAX_DATA_SIZE);
        
        switch (stage) {
        case 0:
            memcpy(data_buffer, fileBuffer + index * MAX_DATA_SIZE, packetDataLen);
            sendPkt = makePacket(curseq, data_buffer, packetDataLen);

            memcpy(pkt_buffer, &sendPkt, sizeof(packet));
            sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);

            start = clock();//��ʱ
            stage = 1;
            curseq = (curseq + 1) % seqsize;
            break;
        case 1:
            //time_out
            while (recvfrom(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, &addrLen) <= 0) {
                if (clock() - start >= MAX_TIME) {
                    sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);
                    cout << "��" << index << "�����ݰ���ʱ�ش�" << endl;
                    start = clock();
                }
            }

            memcpy(&pkt, pkt_buffer, sizeof(packet));
            if (pkt.head.ack == curack || checkPacketSum((u_short*)&pkt, sizeof(packet)) != 0) {
                stage = 1;
                break;
            }
            //cout << "�ɹ����͵�" << index << "�����ݰ�" << endl;
            stage = 0;
            curack = (curack + 1) % seqsize;
            index++;
            break;
        
        default:
            cout << "error" << endl;
            return;
        }
    }
}

int main() {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //����ʧ��
        cout << "����DLLʧ��" << endl;
        return -1;
    }
    SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0);

    u_long imode = 1;
    ioctlsocket(sockClient, FIONBIO, &imode);//������

    SOCKADDR_IN addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(PORT);
    addrSrv.sin_addr.S_un.S_addr = inet_addr(ADDRSRV);

    if (!connectToServer(sockClient, addrSrv)) {
        cout << "����ʧ��" << endl;
        return 0;
    }

    string filename;
    cout << "��������Ҫ������ļ���" << endl;
    cin >> filename;

    sendFSM(filename.length(), (char *)filename.c_str(), sockClient, addrSrv);

    ifstream infile(filename, ifstream::binary);

    if (!infile.is_open()) {
        cout << "�޷����ļ�" << endl;
        return 0;
    }

    infile.seekg(0, infile.end);
    u_long fileLen = infile.tellg();
    infile.seekg(0, infile.beg);
    //cout << fileLen << endl;

    char* fileBuffer = new char[fileLen];
    infile.read(fileBuffer, fileLen);
    infile.close();
    //cout.write(fileBuffer,fileLen);
    cout << "��ʼ����" << endl;

    clock_t start = clock();
    sendFSM(fileLen, fileBuffer, sockClient, addrSrv);
    clock_t end = clock();
    cout << "������ʱ��Ϊ:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
    cout << "������Ϊ:" << ((float)fileLen) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;

    if (!disConnect(sockClient, addrSrv)) {
        cout << "�Ͽ�ʧ��" << endl;
        return 0;
    }
    cout << "�ļ��������" << endl;
    return 1;
}