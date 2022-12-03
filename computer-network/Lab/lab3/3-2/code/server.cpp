#include "define.h"

#define PORT 7879
#define ADDRSRV "127.0.0.1"
#define MAX_FILE_SIZE 100000000
double MAX_TIME = CLOCKS_PER_SEC;
static u_long base_stage = 0;
static int windowSize = 16;
char fileBuffer[MAX_FILE_SIZE];

bool acceptClient(SOCKET &socket, SOCKADDR_IN &addr) {

    char *buffer = new char[sizeof(PacketHead)];
    int len = sizeof(addr);
    recvfrom(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, &len);

    //ShowPacket((Packet*)buffer);

    if ((((PacketHead *) buffer)->flag & SYN) && (CheckPacketSum((u_short *) buffer, sizeof(PacketHead)) == 0))
        cout << "��һ�����ֳɹ�" << endl;
    else
        return false;
    base_stage = ((PacketHead *) buffer)->seq;

    PacketHead head;
    head.flag |= ACK;
    head.flag |= SYN;
    head.windows=windowSize;
    head.checkSum = CheckPacketSum((u_short *) &head, sizeof(PacketHead));
    memcpy(buffer, &head, sizeof(PacketHead));
    if (sendto(socket, buffer, sizeof(PacketHead), 0, (sockaddr *) &addr, len) == -1) {
        return false;
    }

    //ShowPacket((Packet*)buffer);

    cout << "�ڶ������ֳɹ�" << endl;
    u_long imode = 1;
    ioctlsocket(socket, FIONBIO, &imode);//������

    clock_t start = clock(); //��ʼ��ʱ
    while (recvfrom(socket, buffer, sizeof(head), 0, (sockaddr *) &addr, &len) <= 0) {
        if (clock() - start >= MAX_TIME) {
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr *) &addr, len);
            start = clock();
        }
    }

    //ShowPacket((Packet*)buffer);

    if ((((PacketHead *) buffer)->flag & ACK) && (CheckPacketSum((u_short *) buffer, sizeof(PacketHead)) == 0)) {
        cout << "���������ֳɹ�" << endl;
    } else {
        return false;
    }
    imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//����

    cout << "���û��˳ɹ��������ӣ�׼�������ļ�" << endl;
    return true;
}

bool disConnect(SOCKET& socket, SOCKADDR_IN& addr) {
    int addrLen = sizeof(addr);
    char* buffer = new char[sizeof(PacketHead)];

    recvfrom(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR*)&addr, &addrLen);
    if ((((PacketHead*)buffer)->flag & FIN) && (CheckPacketSum((u_short*)buffer, sizeof(PacketHead) == 0))) {
        cout << "��һ�λ��֣��û��˶Ͽ�" << endl;
    }
    else {
        cout << "�������������ж�" << endl;
        return false;
    }

    PacketHead closeHead;
    closeHead.flag = 0;
    closeHead.flag |= ACK;
    closeHead.checkSum = CheckPacketSum((u_short*)&closeHead, sizeof(PacketHead));
    memcpy(buffer, &closeHead, sizeof(PacketHead));
    int send_receive=sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR*)&addr, addrLen);
    if(send_receive>0){
        cout<<"�ڶ��λ��ֳɹ�"<<endl;
    }

    closeHead.flag |= FIN;
    closeHead.checkSum = CheckPacketSum((u_short*)&closeHead, sizeof(PacketHead));
    memcpy(buffer, &closeHead, sizeof(PacketHead));
    send_receive=sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR*)&addr, addrLen);
    if(send_receive>0){
        cout<<"�����λ��ֳɹ�"<<endl;
    }

    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);
    clock_t start = clock();
    while (recvfrom(socket, buffer, sizeof(PacketHead), 0, (sockaddr*)&addr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIME) {
            memcpy(buffer, &closeHead, sizeof(PacketHead));
            sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR*)&addr, addrLen);
            start = clock();
        }
    }

    if ((((PacketHead*)buffer)->flag & ACK) && (CheckPacketSum((u_short*)buffer, sizeof(PacketHead) == 0))) {
        cout << "���ӹر�" << endl;
    }
    else {
        cout << "�������������ж�" << endl;
        return false;
    }
    closesocket(socket);
    return true;
}

Packet makePacket(u_int ack) {
    Packet pkt;
    pkt.head.ack = ack;
    pkt.head.flag |= ACK;
    pkt.head.checkSum = CheckPacketSum((u_short *) &pkt, sizeof(Packet));

    return pkt;
}

u_long recvFSM(char *filebuffer, SOCKET &socket, SOCKADDR_IN &addr) {
    u_long fileLen = 0;
    int addrLen = sizeof(addr);
    u_int expectedSeq = base_stage;
    int dataLen;

    char *pkt_buffer = new char[sizeof(Packet)];
    Packet recvPkt, sendPkt= makePacket(base_stage - 1);//�����ACK����

    while (true) {
        memset(pkt_buffer, 0, sizeof(Packet));
        recvfrom(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, &addrLen);
        memcpy(&recvPkt,pkt_buffer, sizeof(Packet));
        //ShowPacket(&recvPkt);

        if ((recvPkt.head.flag & END) && CheckPacketSum((u_short *) &recvPkt, sizeof(PacketHead)) == 0) {
            cout << "�������" << endl;
            PacketHead endPacket;
            endPacket.flag |= ACK;
            endPacket.checkSum = CheckPacketSum((u_short *) &endPacket, sizeof(PacketHead));
            memcpy(pkt_buffer, &endPacket, sizeof(PacketHead));
            sendto(socket, pkt_buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, addrLen);
            return fileLen;
        }

        if((recvPkt.head.seq==expectedSeq) && (CheckPacketSum((u_short *) &recvPkt, sizeof(Packet)) == 0)){
            //correctly receive the expected seq
            dataLen = recvPkt.head.bufSize;
            memcpy(filebuffer + fileLen, recvPkt.data, dataLen);
            fileLen += dataLen;

            //give back ack=seq
            sendPkt = makePacket(expectedSeq);
            memcpy(pkt_buffer, &sendPkt, sizeof(Packet));
            expectedSeq=(expectedSeq+1)%MAX_SEQ;
            continue;
        }
        //����������Seq������̳����ش�ACK
        memcpy(pkt_buffer, &sendPkt, sizeof(Packet));
        sendto(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);
    }
}

int main() {
    WSAData wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //����ʧ��
        cout << "����DLLʧ��" << endl;
        return -1;
    }
    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);

    SOCKADDR_IN addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(PORT);
    addrSrv.sin_addr.S_un.S_addr = inet_addr(ADDRSRV);
    bind(sockSrv, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));

    SOCKADDR_IN addrClient;

    //�������ֽ�������
    if (!acceptClient(sockSrv, addrClient)) {
        cout << "����ʧ��" << endl;
        return 0;
    }

    //�ɿ����ݴ������
    u_long fileLen = recvFSM(fileBuffer, sockSrv, addrClient);

    //д�븴���ļ�
    string filename = R"(D:\wtx\computer-network\computer-network\Lab\lab3\3-2\code\test\out\1_recv.jpg)";
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) {
        cout << "���ļ�����" << endl;
        return 0;
    }
    cout << fileLen << endl;
    outfile.write(fileBuffer, fileLen);
    outfile.close();

    cout << "�ļ��������" << endl;

    //�Ĵλ��ֶϿ�����
    disConnect(sockSrv, addrClient);
    return 1;
}