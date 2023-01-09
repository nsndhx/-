#include "define.h"
#define ADDRSRV "127.0.0.1"
#define PORT 7879

static int addrLen;
double MAX_TIME = CLOCKS_PER_SEC / 4;
static u_long fileLen;
SOCKADDR_IN addrSrv;
//static const u_long windowSize = 8 * MSS;
static const u_long windowSize = MSS;
static u_int base = 0;//���ֽ׶�ȷ���ĳ�ʼ���к�
static u_int nextSeqNum = base;
static mutex mutexLock;

static clock_t start;
static bool stopTimer = true;

static u_long cwnd = MSS;
static u_long ssthresh = 6 * MSS;
static int dupACKCount = 0;
static u_long window;
//sender������
static u_long lastSendByte = 0, lastAckByte = 0;
static Packet sendPkts[50]{};

//RENO

static int RENO_STAGE = START_UP;

static bool fastResend = false;
static bool timeOutResend = false;

bool connectToServer(SOCKET &socket, SOCKADDR_IN &addr) {
    int len = sizeof(addr);

    PacketHead head;
    head.flag |= SYN;
    head.seq = base;
    head.checkSum = CheckPacketSum((u_short *) &head, sizeof(head));

    char *buffer = new char[sizeof(head)];
    memcpy(buffer, &head, sizeof(head));
    sendto(socket, buffer, sizeof(head), 0, (sockaddr *) &addr, len);
    ShowPacket((Packet *) &head);
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

    ShowPacket((Packet *) &closeHead);
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
    ShowPacket((Packet *) buffer);
    if ((((PacketHead *) buffer)->flag & ACK) && (CheckPacketSum((u_short *) buffer, sizeof(PacketHead) == 0))) {
        cout << "�ڶ��λ��ֳɹ����ͻ����Ѿ��Ͽ�" << endl;
    } else {
        return false;
    }

    u_long imode = 0;
    ioctlsocket(socket, FIONBIO, &imode);//����
    recvfrom(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, &addrLen);
    memcpy(&closeHead, buffer, sizeof(PacketHead));
    ShowPacket((Packet *) buffer);
    if ((((PacketHead *) buffer)->flag & FIN) && (CheckPacketSum((u_short *) buffer, sizeof(PacketHead) == 0))) {
        cout << "�����λ��ֳɹ����������Ͽ�" << endl;
    } else {
        return false;
    }

    imode = 1;
    ioctlsocket(socket, FIONBIO, &imode);

    closeHead.flag = 0;
    closeHead.flag |= ACK;
    closeHead.checkSum = CheckPacketSum((u_short *) &closeHead, sizeof(PacketHead));

    memcpy(buffer, &closeHead, sizeof(PacketHead));
    sendto(socket, buffer, sizeof(PacketHead), 0, (SOCKADDR *) &addr, addrLen);
    ShowPacket((Packet *) &closeHead);
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

u_int waitingNum(u_int nextSeq) {
    if (nextSeq >= base)
        return nextSeq - base;
    return nextSeq + MAX_SEQ - base;
}

DWORD WINAPI ACKHandler(LPVOID param) {
    SOCKET *clientSock = (SOCKET *) param;
    char recvBuffer[sizeof(Packet)];
    Packet recvPacket;

    while (true) {
        if (recvfrom(*clientSock, recvBuffer, sizeof(Packet), 0, (SOCKADDR *) &addrSrv, &addrLen) > 0) {
            memcpy(&recvPacket, recvBuffer, sizeof(Packet));
            mutexLock.lock();
            if (CheckPacketSum((u_short *) &recvPacket, sizeof(Packet)) == 0 && recvPacket.head.flag & ACK) {
                if (base < (recvPacket.head.ack + 1)) {

                    int d = recvPacket.head.ack + 1 - base;
                    for (int i = 0; i < d; i++) {
                        lastAckByte += sendPkts[i].head.bufSize;
                    }
                    for (int i = 0; i < (int) waitingNum(nextSeqNum) - d; i++) {
                        sendPkts[i] = sendPkts[i + d];
                    }
                    string stageName;
                    switch (RENO_STAGE) {
                        case START_UP:
                            cwnd += d * MSS;
                            dupACKCount = 0;
                            if (cwnd >= ssthresh)
                                RENO_STAGE = AVOID;
                            break;
                        case AVOID:
                            cwnd += d * MSS * MSS / cwnd;
                            dupACKCount = 0;
                            break;
                        case RECOVERY:
                            cwnd = ssthresh;
                            dupACKCount = 0;
                            RENO_STAGE = AVOID;
                            break;
                    }
                    window = min(cwnd, windowSize);
                    base = (recvPacket.head.ack + 1) % MAX_SEQ;
                    stageName = getRENOStageName(RENO_STAGE);
#ifdef OUTPUT_LOG
                    cout << "[" << stageName << "]cwnd:" << cwnd << "\twindow:" << window << "\tssthresh:" << ssthresh
                         << endl;
                    cout << "[lastACKByte:" << lastAckByte << "\tlastSendByte:" << lastSendByte
                         << "\tlastWritenByte:"
                         << lastAckByte + window << "]" << endl;
#endif
                } else {
                    //duplicate ACK
                    dupACKCount++;
                    if (RENO_STAGE == START_UP || RENO_STAGE == AVOID) {
                        if (dupACKCount == 3) {
                            ssthresh = cwnd / 2;
                            cwnd = ssthresh + 3 * MSS;
                            RENO_STAGE = RECOVERY;
                            //retransmit missing segment
                            fastResend = true;
#ifdef OUTPUT_LOG
                            cout << "ACK DUP 3 times!Retransmit the missing packet" << endl;
#endif
                        }
                    } else {
                        cwnd += MSS;
                    }

                    string stageName = getRENOStageName(RENO_STAGE);
#ifdef OUTPUT_LOG
                    cout << "[" << stageName << "]cwnd:" << cwnd << "\twindow:" << window << "\tssthresh:" << ssthresh
                         << endl;
                    cout << "[lastACKByte:" << lastAckByte << "\tlastSendByte:" << lastSendByte
                         << "\tlastWritenByte:"
                         << lastAckByte + window << "]" << endl;
#endif
                }

                if (base == nextSeqNum)//���͵�ȫ����ȷ��
                    stopTimer = true;
                else {
                    start = clock();
                    stopTimer = false;
                }
            }
            mutexLock.unlock();
            if (lastAckByte == fileLen)
                return 0;
        }
    }
}

DWORD WINAPI TIMEOUTHandler(LPVOID param) {
    while (true) {

        if (lastAckByte == fileLen)
            return 0;
        if (!stopTimer) {
            if (clock() - start > MAX_TIME) {
                timeOutResend = true;
#ifdef OUTPUT_LOG
                cout << "[time out!]Begin Resend" << endl;
#endif
            }
        }
    }
}

void sendFSM(u_long len, char *fileBuffer, SOCKET &socket, SOCKADDR_IN &addr) {

    int packetDataLen;

    char *data_buffer = new char[sizeof(Packet)], *pkt_buffer = new char[sizeof(Packet)];
    nextSeqNum = base;
    cout << "�����ļ����ݳ���Ϊ" << len << "Bytes" << endl;

    HANDLE ackhandler = CreateThread(nullptr, 0, ACKHandler, LPVOID(&socket), 0, nullptr);
    HANDLE timeouthandler = CreateThread(nullptr, 0, TIMEOUTHandler, nullptr, 0, nullptr);
    string stageName;
    while (true) {

        if (lastAckByte == len) {
            CloseHandle(ackhandler);
            CloseHandle(timeouthandler);
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

            if (((PacketHead *) (pkt_buffer))->flag & ACK &&
                CheckPacketSum((u_short *) pkt_buffer, sizeof(PacketHead)) == 0) {
                cout << "�ļ��������" << endl;
                return;
            }

            resend:
            continue;
        }

        if (fastResend || timeOutResend)
            goto GBN;

        mutexLock.lock();
        window = min(cwnd, windowSize);
        if ((lastSendByte < lastAckByte + window) && (lastSendByte < len)) {

            packetDataLen = min(lastAckByte + window - lastSendByte, MSS);
            packetDataLen = min(packetDataLen, len - lastSendByte);
            memcpy(data_buffer, fileBuffer + lastSendByte, packetDataLen);

            sendPkts[nextSeqNum - base] = makePacket(nextSeqNum, data_buffer, packetDataLen);
            memcpy(pkt_buffer, &sendPkts[nextSeqNum - base], sizeof(Packet));

            sendto(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);

            if (base == nextSeqNum) {
                start = clock();
                stopTimer = false;
            }
            nextSeqNum = (nextSeqNum + 1) % MAX_SEQ;
            lastSendByte += packetDataLen;
        }
        mutexLock.unlock();
        continue;

        GBN:
        mutexLock.lock();
        for (int i = 0; i < nextSeqNum - base; i++) {
            memcpy(pkt_buffer, &sendPkts[i], sizeof(Packet));
            sendto(socket, pkt_buffer, sizeof(Packet), 0, (SOCKADDR *) &addr, addrLen);
        }
        if (timeOutResend) {
            ssthresh = cwnd / 2;
            cwnd = MSS;
            dupACKCount = 0;
            RENO_STAGE = START_UP;
        }
        timeOutResend = fastResend = false;
        mutexLock.unlock();
        start = clock();
        stopTimer = false;
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


    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(PORT);
    addrSrv.sin_addr.S_un.S_addr = inet_addr(ADDRSRV);
    addrLen = sizeof(addrSrv);
    if (!connectToServer(sockClient, addrSrv)) {
        cout << "����ʧ��" << endl;
        return 0;
    }

    string filename = R"(D:\wtx\computer-network\computer-network\Lab\lab3\3-3\code\test\in\1.jpg)";

    ifstream infile(filename, ifstream::binary);

    if (!infile.is_open()) {
        cout << "[ERROR]�޷����ļ�" << endl;
        return 0;
    }

    infile.seekg(0, infile.end);
    fileLen = infile.tellg();
    infile.seekg(0, infile.beg);

    char *fileBuffer = new char[fileLen];
    infile.read(fileBuffer, fileLen);
    infile.close();
    cout << "��ʼ����" << endl;

    auto start_time = chrono::system_clock::now();
    sendFSM(fileLen, fileBuffer, sockClient, addrSrv);
    auto end_time = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end_time-start_time);
    double time=double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den;
    cout << "������ʱ��Ϊ:" << time << "s" << endl;
    cout << "������Ϊ:" << ((float) fileLen) / time << "byte/s" << endl;

    if (!disConnect(sockClient, addrSrv)) {
        cout << "�Ͽ�ʧ��" << endl;
        return 0;
    }
    cout << "�ļ��������" << endl;
    return 1;
}