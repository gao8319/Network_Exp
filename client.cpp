#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT 8000
#define FTP_PORT 8006
#define IP "192.168.1.113"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "pthreadVC2.lib")

using namespace std;

void login(SOCKET *soc);
void ftp_send(SOCKET *soc);
void ftp_recv(SOCKET *soc);
void *send_func(void *arg);
void *recv_func(void *arg);
void main_UI(SOCKET *soc);

void login(SOCKET *soc)
{
    system("cls");

    char username[20] = {0};
    char passwd[20] = {0};
    char uname_pwd[50] = {0};
    char space[] = " ";
    char rec_log[10] = {0};

    cout << "------------------------------------------------------------------------" << endl;

    while (1)
    {
        printf("Please input your UserName: ");
        cin.sync();
        gets(username);
        printf("Please input your PassWord: ");
        cin.sync();
        gets(passwd);

        strcat(username, space);
        strcat(username, passwd);
        strcpy(uname_pwd, username);

        send(*soc, uname_pwd, (int)strlen(uname_pwd), 0);
        recv(*soc, rec_log, 10, 0);

        if (strncmp(rec_log, "1", 1) == 0)
        {
            break;
        }
    }
    printf("Authentication succeeded, linked to ChatRoom.\n");
}

void ftp_send(SOCKET *soc)
{
    Sleep(50);
    char ftpreq[50];

    // cout << "1.Offline" << endl;
    // cout << "2.Online" << endl;

    // switch (getchar())
    // {
    //     case ('1'):
    //         strcpy(ftpreq, "FTPoffline");
    //         send(*soc, ftpreq, (int)strlen(ftpreq), 0);
    //         break;
    //     case ('2'):
    //         strcpy(ftpreq, "FTPonline");
    //         send(*soc, ftpreq, (int)strlen(ftpreq), 0);
    //         break;
    // }

    //cout << "Location: " << endl;
    FILE *fp = fopen("D:\\Algorithm\\Network\\ChatRoom\\paper.txt", "rb");

    if (fp==NULL)
    {
        printf("ERROR: The file was not opened.\n");
        return;
    }
    else
    {
        printf("Opening file...\n");
    }

    int nCount;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    int send_ret;
    int sum = 0;

    while ((nCount = fread(sendbuf, 1, DEFAULT_BUFLEN, fp)) > 0)
    {
        send_ret = send(*soc, sendbuf, nCount, 0);
        sum += send_ret;
        cout << "send signal:" << send_ret << endl;
        if(nCount<DEFAULT_BUFLEN)
            break;
    }

    cout << "sum bytes send: " << sum << endl;
    fclose(fp);

    char fin[] = "FTPfin";
    Sleep(50);
    send_ret = send(*soc, fin, (int)strlen(fin), 0);
    cout << "send signal:" << send_ret << endl;

    cout << "Done." << endl;
}

void ftp_recv(SOCKET *soc)
{
    cout << "ftp recv" << endl;

    char filename[10] = "recv.txt"; //文件名
    FILE *fp = fopen(filename, "wb");  //以二进制方式打开（创建）文件
    char buffer[DEFAULT_BUFLEN] = {0}; //文件缓冲区
    int nCount;
    int sum = 0;

    while (1)
    {
        nCount = recv(*soc, buffer, DEFAULT_BUFLEN, 0);
        cout << "recv signal: " << nCount << endl;
        if (strncmp(buffer, "FTPfin", 6) == 0)
            break;
        if (nCount > 0)
        {
            fwrite(buffer, nCount, 1, fp);
        }
        sum += nCount;
    }

    cout << "sum bytes recevice: " << sum << endl;
    fclose(fp);
    
    cout << "File transfer success!";
}

void *ftp_func(void *arg)
{
    int cmd = *(int *)arg;
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;

    if (WSAStartup(sockVersion, &data) != 0)
    {
        pthread_exit(0);
    }

    SOCKET ftpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(ftpsock==INVALID_SOCKET)
    {
        cout << ("invalid socket!");
        pthread_exit(0);
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(FTP_PORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(IP);

    cout << "Connecting to ftp service." << endl;

    if (connect(ftpsock, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        cout << "ftp_func Connect error." << endl;
        closesocket(ftpsock);
        pthread_exit(0);
    }

    if(cmd==0)
    {
        char signal[] = "I am sender.";
        send(ftpsock, signal, (int)strlen(signal), 0);
        ftp_send(&ftpsock);
    }
    else if (cmd==1)
    {
        char signal[]="I am receiver.";
        send(ftpsock, signal, (int)strlen(signal), 0);
        ftp_recv(&ftpsock);
    }
    
    closesocket(ftpsock);
    WSACleanup();
    pthread_exit(0);
}

void *send_func(void *arg)
{
    char sendbuf[DEFAULT_BUFLEN];
    SOCKET send_sock = (SOCKET)arg;

    while (1)
    {
        if (fgets(sendbuf, DEFAULT_BUFLEN, stdin) == NULL)
        {
            continue;
        }
        else
        {
            if (strncmp(sendbuf, "QUIT", 4) == 0)
                break;
            if (strncmp(sendbuf, "FTP", 3) == 0)
            {
                char ftpreq[] = "FTPonline";
                send(send_sock, ftpreq, (int)strlen(ftpreq), 0);
                pthread_t ftp_thread;
                int ftp_result;
                int cmd = 0;
                ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, (void *)&cmd);
                ftp_result = pthread_join(ftp_thread, NULL);
                cout << "Back to main thread." << endl;
                continue;
            }
            if (send(send_sock, sendbuf, (int)strlen(sendbuf), 0) == -1)
            {
                printf("Send failed with error: %d\n", WSAGetLastError());
                continue;
            }
        }
    }

    pthread_exit(0);
}

void *recv_func(void *arg)
{
    SOCKET recv_sock = (SOCKET)arg;
    int recv_ret;
    char recvbuf[DEFAULT_BUFLEN] = {0};
    int recvbuflen = DEFAULT_BUFLEN;

    do
    {
        recv_ret= recv(recv_sock, recvbuf, recvbuflen, 0);

        if (strncmp(recvbuf, "FTPonline", 6) == 0)
        {
            pthread_t ftp_thread;
            int ftp_result;
            int cmd = 1;
            ftp_result = pthread_create(&ftp_thread, NULL, ftp_func, (void *)&cmd);
            ftp_result = pthread_join(ftp_thread, NULL);
            continue;
        }

        if (recv_ret > 0)
        {
            cout << "RECV MESSAGE: " << recvbuf;
        }
        else if (recv_ret == 0)
            printf("Connection closed.\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (recv_ret > 0);

    pthread_exit(0);
}

void main_UI(SOCKET *soc)
{
    //pirnt main UI

    system("cls");
    cout << "------------------------------------------------------------------------" << endl;
    cout << "Input the operation: " << endl;
    cout << endl;
    cout << "1. Sign in." << endl;
    cout << endl;
    cout << "2. Sign up." << endl;
    cout << "------------------------------------------------------------------------" << endl;

    while (1)
    {
        switch (getchar())
        {
        case ('1'):
            login(soc);
            break;
        case ('2'):
            break;
        default:
            system("cls");
            cout << "------------------------------------------------------------------------" << endl;
            cout << "Undefined opreation." << endl;
            cout << "------------------------------------------------------------------------" << endl;
            system("pause");
        }
        break;
    }
}

int main()
{
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0)
    {
        return 0;
    }

    SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sclient==INVALID_SOCKET)
    {
        cout << ("invalid socket!");
        return 0;
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(DEFAULT_PORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(IP);
    if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        cout << "Main Connect error." << endl;
        closesocket(sclient);
        return 0;
    }

    main_UI(&sclient);

    pthread_t recv_p, send_p;
    int send_result, recv_result;

    send_result = pthread_create(&send_p, NULL, send_func, (void *)sclient);
    recv_result = pthread_create(&recv_p, NULL, recv_func, (void *)sclient);

    if (send_result != 0)
    {
        printf("send_thread create fail\n");
        exit(1);
    }
    if (recv_result != 0)
    {
        printf("recv_thread create fail\n");
        exit(1);
    }

    send_result = pthread_join(send_p, NULL);
    recv_result = pthread_join(recv_p, NULL);

    printf("SYSTEM: Chat over.\n");

    // cleanup
    closesocket(sclient);
    WSACleanup();
    return 0;
}
