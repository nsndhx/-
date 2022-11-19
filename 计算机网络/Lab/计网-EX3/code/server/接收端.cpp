#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#include <cstdio>
#include <windows.h>
#include <iostream>
#include <thread>
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
#define MAX_FILE_SIZE 100000000
#define MAX_DATA_SIZE 2048

char fileBuffer[MAX_FILE_SIZE];
double MAX_TIME = CLOCKS_PER_SEC;
int seqsize = 2;
int curseq = 1;
int curack = 0;

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

bool acceptClient(SOCKET& socket, SOCKADDR_IN& addr) {

    char* buffer = new char[sizeof(packetHead)];
    int len = sizeof(addr);
    recvfrom(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, &len);

    if ((((packetHead*)buffer)->flag & SYN) && (checkPacketSum((u_short*)buffer, sizeof(packetHead)) == 0))
        cout << "��һ�����ֳɹ�" << endl;//[SYN_RECV]
    else {
        cout << "���ǵ�һ�����ֵ����ݰ�" << endl;
        return false;
    }

    packetHead head;
    head.flag |= ACK;
    head.flag |= SYN;
    head.checkSum = checkPacketSum((u_short*)&head, sizeof(packetHead));
    memcpy(buffer, &head, sizeof(packetHead));
    if (sendto(socket, buffer, sizeof(packetHead), 0, (sockaddr*)&addr, len) == -1) {
        cout << "�ڶ����������ݰ�����ʧ��" << endl;
        return false;
    }
    cout << "�ڶ������ֳɹ�" << endl;//[SYN_ACK_SEND]

    u_long mode = 1;//imode=0Ϊ������imode=1Ϊ������
    ioctlsocket(socket, FIONBIO, &mode);//������

    clock_t start = clock(); //��ʼ��ʱ
    while (recvfrom(socket, buffer, sizeof(head), 0, (sockaddr*)&addr, &len) <= 0) {
        if (clock() - start >= MAX_TIME) {
            cout << "δ���ܵ�������������Ϣ����ʱ�ش�" << endl;
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr*)&addr, len);
            start = clock();
        }
    }

    if ((((packetHead*)buffer)->flag & ACK) && (checkPacketSum((u_short*)buffer, sizeof(packetHead)) == 0)) {
        cout << "���������ֳɹ�" << endl;//[ACK_RECV]
    }
    else {
        return false;
    }
    mode = 0;
    ioctlsocket(socket, FIONBIO, &mode);//����

    cout << "���û��˳ɹ��������ӣ�׼�������ļ�" << endl;//[CONNECTED]
    return true;
}

bool disConnect(SOCKET& socket, SOCKADDR_IN& addr) {
    int addrLen = sizeof(addr);
    char* buffer = new char[sizeof(packetHead)];

    recvfrom(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, &addrLen);
    if ((((packetHead*)buffer)->flag & FIN) && (checkPacketSum((u_short*)buffer, sizeof(packetHead) == 0))) {
        cout << "�û��˶Ͽ�" << endl;
    }
    else {
        cout << "�������������ж�" << endl;
        return false;
    }

    packetHead closeHead;
    closeHead.flag = 0;
    closeHead.flag |= ACK;
    closeHead.checkSum = checkPacketSum((u_short*)&closeHead, sizeof(packetHead));
    memcpy(buffer, &closeHead, sizeof(packetHead));
    sendto(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);

    closeHead.flag |= FIN;
    closeHead.checkSum = checkPacketSum((u_short*)&closeHead, sizeof(packetHead));
    memcpy(buffer, &closeHead, sizeof(packetHead));
    sendto(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);

    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);
    clock_t start = clock();
    while (recvfrom(socket, buffer, sizeof(packetHead), 0, (sockaddr*)&addr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIME) {
            memcpy(buffer, &closeHead, sizeof(packetHead));
            sendto(socket, buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
            start = clock();
        }
    }

    if ((((packetHead*)buffer)->flag & ACK) && (checkPacketSum((u_short*)buffer, sizeof(packetHead) == 0))) {
        cout << "���ӹر�" << endl;
    }
    else {
        cout << "�������������ж�" << endl;
        return false;
    }
    closesocket(socket);
    return true;
}

packet makePacket(int ack) {
    packet pkt;
    pkt.head.ack = ack;
    pkt.head.flag |= ACK;
    pkt.head.checkSum = checkPacketSum((u_short*)&pkt, sizeof(packet));

    return pkt;
}

u_long recvFSM(char* fileBuffer, SOCKET& socket, SOCKADDR_IN& addr) {
    u_long fileLen = 0;
    //int stage = 0;

    int addrLen = sizeof(addr);
    char* pkt_buffer = new char[sizeof(packet)];
    packet pkt, sendPkt;
    int index = 0;
    int dataLen;
    while (true) {
        memset(pkt_buffer, '0', sizeof(packet));
        recvfrom(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, &addrLen);

        memcpy(&pkt, pkt_buffer, sizeof(packetHead));

        if (pkt.head.flag & END) {
            cout << "�ļ��������" << endl;
            packetHead endPacket;
            endPacket.flag |= ACK;
            endPacket.checkSum = checkPacketSum((u_short*)&endPacket, sizeof(packetHead));
            memcpy(pkt_buffer, &endPacket, sizeof(packetHead));
            sendto(socket, pkt_buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
            return fileLen;
        }

        memcpy(&pkt, pkt_buffer, sizeof(packet));

        if (pkt.head.seq == curseq || checkPacketSum((u_short*)&pkt, sizeof(packet)) != 0) {
            sendPkt = makePacket(1);
            memcpy(pkt_buffer, &sendPkt, sizeof(packet));
            sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);
            cout << "�յ��ظ���" << index - 1 << "�����ݰ�����������" << endl;
            break;
        }

        //correctly receive the seq
        dataLen = pkt.head.bufSize;
        memcpy(fileBuffer + fileLen, pkt.data, dataLen);
        fileLen += dataLen;

        //give back ack
        sendPkt = makePacket(curack);
        memcpy(pkt_buffer, &sendPkt, sizeof(packet));
        sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);

        //cout<<"�ɹ��յ�"<<index<<"�����ݰ����䳤����"<<dataLen<<endl;
        curseq = (curseq + 1) % seqsize;
        curack = (curack + 1) % seqsize;
        index++;
        //break;
        /*
        switch (stage) {
        case 0:
            recvfrom(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, &addrLen);

            memcpy(&pkt, pkt_buffer, sizeof(packetHead));

            if (pkt.head.flag & END) {
                cout << "�ļ��������" << endl;
                packetHead endPacket;
                endPacket.flag |= ACK;
                endPacket.checkSum = checkPacketSum((u_short*)&endPacket, sizeof(packetHead));
                memcpy(pkt_buffer, &endPacket, sizeof(packetHead));
                sendto(socket, pkt_buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
                return fileLen;
            }

            memcpy(&pkt, pkt_buffer, sizeof(packet));

            if (pkt.head.seq == 1 || checkPacketSum((u_short*)&pkt, sizeof(packet)) != 0) {
                sendPkt = makePacket(1);
                memcpy(pkt_buffer, &sendPkt, sizeof(packet));
                sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);
                stage = 0;
                cout << "[SYSTEM]�յ��ظ���" << index - 1 << "�����ݰ�����������" << endl;
                break;
            }

            //correctly receive the seq0
            dataLen = pkt.head.bufSize;
            memcpy(fileBuffer + fileLen, pkt.data, dataLen);
            fileLen += dataLen;

            //give back ack0
            sendPkt = makePacket(0);
            memcpy(pkt_buffer, &sendPkt, sizeof(packet));
            sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);
            stage = 1;

            //cout<<"�ɹ��յ�"<<index<<"�����ݰ����䳤����"<<dataLen<<endl;
            index++;
            break;
        case 1:
            recvfrom(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, &addrLen);

            memcpy(&pkt, pkt_buffer, sizeof(packetHead));

            if (pkt.head.flag & END) {
                cout << "[SYSTEM]�������" << endl;
                packetHead endPacket;
                endPacket.flag |= ACK;
                endPacket.checkSum = checkPacketSum((u_short*)&endPacket, sizeof(packetHead));
                memcpy(pkt_buffer, &endPacket, sizeof(packetHead));
                sendto(socket, pkt_buffer, sizeof(packetHead), 0, (SOCKADDR*)&addr, addrLen);
                return fileLen;
            }

            memcpy(&pkt, pkt_buffer, sizeof(packet));

            if (pkt.head.seq == 0 || checkPacketSum((u_short*)&pkt, sizeof(packet)) != 0) {
                sendPkt = makePacket(0);
                memcpy(pkt_buffer, &sendPkt, sizeof(packet));
                sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);
                stage = 1;
                cout << "[SYSTEM]�յ��ظ���" << index - 1 << "�����ݰ�����������" << endl;
                break;
            }

            //correctly receive the seq1
            dataLen = pkt.head.bufSize;
            memcpy(fileBuffer + fileLen, pkt.data, dataLen);
            fileLen += dataLen;

            //give back ack1
            sendPkt = makePacket(1);
            memcpy(pkt_buffer, &sendPkt, sizeof(packet));
            sendto(socket, pkt_buffer, sizeof(packet), 0, (SOCKADDR*)&addr, addrLen);
            stage = 0;

            //cout<<"�ɹ��յ�"<<index<<"�����ݰ����䳤����"<<dataLen<<endl;
            index++;

            break;
        }
        */
    }
}

int main() {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //����ʧ��
        cout << "[ERROR]����DLLʧ��" << endl;
        return -1;
    }
    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);

    SOCKADDR_IN addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(PORT);
    addrSrv.sin_addr.S_un.S_addr = inet_addr(ADDRSRV);
    bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

    SOCKADDR_IN addrClient;

    //�������ֽ�������
    if (!acceptClient(sockSrv, addrClient)) {
        cout << "[ERROR]����ʧ��" << endl;
        return 0;
    }

    char* filename = new char;
    u_long r = recvFSM(filename, sockSrv, addrSrv);
    if (r > 0) {
        printf("�ļ�·��Ϊ:%s\n", filename);
    }
    else {
        cout << "û�н��յ��ļ�·��" << endl;
    }

    //char fileBuffer[MAX_FILE_SIZE];
    //�ɿ����ݴ������
    u_long fileLen = recvFSM(fileBuffer, sockSrv, addrClient);
    //�Ĵλ��ֶϿ�����
    if (!disConnect(sockSrv, addrClient)) {
        cout << "[ERROR]�Ͽ�ʧ��" << endl;
        return 0;
    }

    //д�븴���ļ�
    ofstream outfile(filename, ofstream::binary);
    if (!outfile.is_open()) {
        cout << "[ERROR]���ļ�����" << endl;
        return 0;
    }
    cout << fileLen << endl;
    outfile.write(fileBuffer, fileLen);
    outfile.close();

    cout << "�ļ��������" << endl;
    return 1;
}