#include "define.h"

#define PORT 7879
#define ADDRSRV "127.0.0.1"

static SOCKADDR_IN addrSrv;
static int addrLen = sizeof(addrSrv);
double MAX_TIME = CLOCKS_PER_SEC;
static int windowSize = 16;
static unsigned long int base = 0;//���ֽ׶�ȷ���ĳ�ʼ���к�
static unsigned long int nextSeqNum = 0;
static Packet *sendPkt;
static unsigned long int sendIndex = 0, recvIndex = 0;
static bool stopTimer = false;
static clock_t start;
static int packetNum;

u_int waitingNum(u_int nextSeq) {
    if (nextSeq >= base)
        return nextSeq - base;
    return nextSeq + MAX_SEQ - base;
}

bool connectToServer(SOCKET &socket, SOCKADDR_IN &addr) {
    int len = sizeof(addr);

    PacketHead head;
    head.flag |= SYN;
    head.seq = base;
    head.checkSum = CheckPacketSum((u_short *) &head, sizeof(head));

    char *buffer = new char[sizeof(head)];
    memcpy(buffer, &head, sizeof(head));
    sendto(socket, buffer, sizeof(head), 0, (sockaddr *) &addr, len);
    //ShowPacket((Packet *) &head);
    cout << "��һ�����ֳɹ�" << endl;

    clock_t start_connect = clock(); //��ʼ��ʱ
    while (recvfrom(socket, buffer, sizeof(head), 0, (sockaddr *) &addr, &len) <= 0) {
        if (clock() - start_connect >= MAX_TIME) {
            memcpy(buffer, &head, sizeof(head));
            sendto(socket, buffer, sizeof(buffer), 0, (sockaddr *) &addr, len);
            start_connect = clock();
        }
    }

    memcpy(&head, buffer, sizeof(head));
    ShowPacket((Packet *) &head);
    if ((head.flag & ACK) && (CheckPacketSum((u_short *) &head, sizeof(head)) == 0) && (head.flag & SYN)) {
        cout << "�ڶ������ֳɹ�" << endl;
    } else {
        return false;
    }

    windowSize = head.windows;
    //��������������
    head.flag = 0;
    head.flag |= ACK;
    head.checkSum = 0;
    head.checkSum = (CheckPacketSum((u_short *) &head, sizeof(head)));
    memcpy(buffer, &head, sizeof(head));
    sendto(socket, buffer, sizeof(head), 0, (sockaddr *) &addr, len);
    ShowPacket((Packet *) &head);

    //�ȴ�����MAX_TIME�����û���յ���Ϣ˵��ACKû�ж���
    start_connect = clock();
    while (clock() - start_connect <= 2 * MAX_TIME) {
        if (recvfrom(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, &len) <= 0)
            continue;
        //˵�����ACK����
        memcpy(buffer, &head, sizeof(head));
        sendto(socket, buffer, sizeof(head), 0, (sockaddr *) &addr, len);
        start_connect = clock();
    }
    cout << "�������ֳɹ�" << endl;
    cout << "�ɹ���������������ӣ�׼����������" << endl;
    return true;
}

bool disConnect(SOCKET &socket, SOCKADDR_IN &addr) {
    addrLen = sizeof(addr);
    char *buffer = new char[sizeof(PacketHead)];
    PacketHead closeHead;
    closeHead.flag |= FIN;
    closeHead.checkSum = CheckPacketSum((u_short *) &closeHead, sizeof(PacketHead));

    memcpy(buffer, &closeHead, sizeof(PacketHead));
    if (sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, addrLen) != SOCKET_ERROR)
        cout << "��һ�λ��ֳɹ�" << endl;
    else
        return false;

    start = clock();
    while (recvfrom(socket, buffer, sizeof(PacketHead), 0, (sockaddr *) &addr, &addrLen) <= 0) {
        if (clock() - start >= MAX_TIME) {
            memcpy(buffer, &closeHead, sizeof(PacketHead));
            sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, addrLen);
            start = clock();
        }
    }

    if ((((PacketHead *) buffer)->flag & ACK) && (CheckPacketSum((u_short *) buffer, sizeof(PacketHead) == 0))) {
        cout << "�ڶ��λ��ֳɹ����ͻ����Ѿ��Ͽ�" << endl;
    } else {
        return false;
    }

    u_long mode = 0;
    ioctlsocket(socket, FIONBIO, &mode);//����

    recvfrom(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, &addrLen);

    if ((((PacketHead *) buffer)->flag & FIN) && (CheckPacketSum((u_short *) buffer, sizeof(PacketHead) == 0))) {
        cout << "�����λ��ֳɹ�,�������Ѿ��Ͽ�" << endl;
    } else {
        return false;
    }

    mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);

    closeHead.flag = 0;
    closeHead.flag |= ACK;
    closeHead.checkSum = CheckPacketSum((u_short *) &closeHead, sizeof(PacketHead));

    memcpy(buffer, &closeHead, sizeof(PacketHead));
    sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, addrLen);
    start = clock();
    while (clock() - start <= 2 * MAX_TIME) {
        if (recvfrom(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, &addrLen) <= 0)
            continue;
        //˵�����ACK����
        memcpy(buffer, &closeHead, sizeof(PacketHead));
        sendto(socket, buffer, sizeof(PacketHead), 0, (sockaddr *) &addr, addrLen);
        start = clock();
    }

    cout << "���Ĵλ��ֳɹ��������ѹر�" << endl;
    closesocket(socket);
    return true;
}

Packet makePacket(u_int seq, char *data, int len) {
    Packet pkt;
    pkt.head.seq = seq;
    pkt.head.bufSize = len;
    memcpy(pkt.data, data, len);
    pkt.head.checkSum = CheckPacketSum((u_short *) &pkt, sizeof(Packet));
    return pkt;
}

bool inWindows(u_int seq) {
    if (seq >= base && seq < base + windowSize)
        return true;
    if (seq < base && seq < ((base + windowSize) % MAX_SEQ))
        return true;

    return false;
}

void sendFSM(u_long len, char *fileBuffer, SOCKET &socket, SOCKADDR_IN &addr) {

    packetNum = int(len / MAX_DATA_SIZE) + (len % MAX_DATA_SIZE ? 1 : 0);
    //sendIndex==packetNumʱ�����ٷ��ͣ�recvIndex==packetNum���յ�ȫ��ACK����������
    sendIndex = 0, recvIndex = 0;
    int packetDataLen;
    addrLen = sizeof(addr);

    stopTimer = false;//�Ƿ�ֹͣ��ʱ
    char *data_buffer = new char[packetDataLen], *pkt_buffer = new char[sizeof(Packet)];
    Packet recvPkt;
    nextSeqNum = base;

    sendPkt[windowSize];
    cout << "�����ļ����ݳ���Ϊ" << len << "Bytes,��Ҫ����" << packetNum << "�����ݰ�" << endl;

    while (true) {
        if (recvIndex == packetNum) {
            //recvȫ��ACK���������䣬����END����
            PacketHead endPacket;
            endPacket.flag |= END;
            endPacket.checkSum = CheckPacketSum((u_short *) &endPacket, sizeof(PacketHead));
            memcpy(pkt_buffer, &endPacket, sizeof(PacketHead));
            sendto(socket, pkt_buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, addrLen);

            while (recvfrom(socket, pkt_buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, &addrLen) <= 0) {
                if (clock() - start >= MAX_TIME) {
                    start = clock();
                    goto resend;
                }
            }

            if ((((PacketHead *) (pkt_buffer))->flag & ACK) &&
                (CheckPacketSum((u_short *) pkt_buffer, sizeof(PacketHead)) == 0)) {
                cout << "�ļ��������" << endl;
                return;
            }

            resend:
            continue;
        }

        packetDataLen = min(MAX_DATA_SIZE, len - sendIndex * MAX_DATA_SIZE);

        //�����һ�����к��ڻ���������
        if (inWindows(nextSeqNum) && sendIndex < packetNum) {

            memcpy(data_buffer, fileBuffer + sendIndex * MAX_DATA_SIZE, packetDataLen);
            //�����������
            sendPkt[(int) waitingNum(nextSeqNum)] = makePacket(nextSeqNum, data_buffer, packetDataLen);
            memcpy(pkt_buffer, &sendPkt[(int) waitingNum(nextSeqNum)], sizeof(Packet));
            //���͸����ն�
            sendto(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);

            //���Ŀǰ������ֻ��һ�����ݱ�����ʼ��ʱ���������ڹ���һ����ʱ����
            if (base == nextSeqNum) {
                start = clock();
                stopTimer = false;
            }
            nextSeqNum = (nextSeqNum + 1) % MAX_SEQ;
            sendIndex++;
            //cout << sendIndex << "�����ݰ��Ѿ�����" << endl;
        }
        //�ж��Ƿ���ACK����
        while (recvfrom(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, &addrLen) > 0) {
            memcpy(&recvPkt, pkt_buffer, sizeof(Packet));
            //corrupt
            if (CheckPacketSum((u_short *) &recvPkt, sizeof(Packet)) != 0 || !(recvPkt.head.flag & ACK))
                goto time_out;
            //not corrupt
            if (base < (recvPkt.head.ack + 1)) {
                //���Ǵ������ACK
                int d = recvPkt.head.ack + 1 - base;
                for (int i = 0; i < (int) waitingNum(nextSeqNum) - d; i++) {
                    sendPkt[i] = sendPkt[i + d];
                }
                recvIndex += d;
                cout << "[window move]base:" << base << "\tnextSeq:" << nextSeqNum << "\tendWindow:"
                     << base + windowSize << endl;
            }
            base = (max((recvPkt.head.ack + 1), base)) % MAX_SEQ;
            //������Ϊ�գ�ֹͣ��ʱ
            if (base == nextSeqNum)
                stopTimer = true;
            else {
                start = clock();
                stopTimer = false;
            }

        }
        //��ʱ�������������л�������ݱ�ȫ���ش�һ�Σ������Go Back N
        time_out:
        if (!stopTimer && clock() - start >= MAX_TIME) {
            //cout << "resend" << endl;
            for (int i = 0; i < (int) waitingNum(nextSeqNum); i++) {
                memcpy(pkt_buffer, &sendPkt[i], sizeof(Packet));
                sendto(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);
            }
            cout << "��" << base << "�����ݰ���ʱ�ش�" << endl;
            start = clock();
            stopTimer = false;
        }
    }
}

int main() {
    WSAData wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        //����ʧ��
        cout << "����DLLʧ��" << endl;
        return -1;
    }
    SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0);

    u_long imode = 1;
    ioctlsocket(sockClient, FIONBIO, &imode);//������

    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(PORT);
    addrSrv.sin_addr.S_un.S_addr = inet_addr(ADDRSRV);

    if (!connectToServer(sockClient, addrSrv)) {
        cout << "����ʧ��" << endl;
        return 0;
    }
    sendPkt = new Packet[windowSize];
    string filename = R"(D:\wtx\computer-network\computer-network\Lab\lab3\3-2\code\test\in\1.jpg)";

    ifstream infile(filename, ifstream::binary);

    if (!infile.is_open()) {
        cout << "�޷����ļ�" << endl;
        return 0;
    }

    infile.seekg(0, infile.end);
    u_long fileLen = infile.tellg();
    infile.seekg(0, infile.beg);
    cout << fileLen << endl;

    char *fileBuffer = new char[fileLen];
    infile.read(fileBuffer, fileLen);
    infile.close();
    cout << "��ʼ����" << endl;

    clock_t start_time = clock();
    sendFSM(fileLen, fileBuffer, sockClient, addrSrv);
    clock_t end_time = clock();
    cout << "������ʱ��Ϊ:" << (end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;
    cout << "������Ϊ:" << ((float) fileLen) / ((float) (end_time - start_time) / CLOCKS_PER_SEC) << "byte/s" << endl;

    if (!disConnect(sockClient, addrSrv)) {
        cout << "�Ͽ�ʧ��" << endl;
        return 0;
    }
    cout << "�ļ��������" << endl;
    return 1;
}