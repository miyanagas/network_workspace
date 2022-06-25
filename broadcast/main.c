#include "mynet.h"
#include "chat.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define USERNAME_LEN 16
#define BUFSIZE 10
#define DEFAULT_PORT 50001 /* ポート番号既定値 */

extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char *argv[])
{
    struct sockaddr_in broadcast_adrs;
    struct sockaddr_in from_adrs;
    socklen_t from_len;

    int sock;
    int broadcast_sw=1;
    fd_set mask, readfds;
    struct timeval timeout;

    char r_buf[BUFSIZE];
    int i, strsize;
    char mode = 'S';

    in_port_t port_number=DEFAULT_PORT;
    char username[USERNAME_LEN];
    int c;

    opterr = 0;
    while(1){
        c = getopt(argc, argv, "n:p:h");
        if( c == -1 ) break;
        
        switch( c ){
        case 'n' :
            snprintf(username, USERNAME_LEN, "%s", optarg);
            break;
        case 'p':  /* ポート番号の指定 */
            port_number = (in_port_t)atoi(optarg);
            break;
        case '?' :
            fprintf(stderr,"Unknown option '%c'\n", optopt);
        case 'h' :
            fprintf(stderr,"Usage: %s -n username -p port_number\n", argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    /* ブロードキャストアドレスの情報をsockaddr_in構造体に格納する */
    set_sockaddr_in_broadcast(&broadcast_adrs, port_number);

    /* ソケットをDGRAMモードで作成する */
    sock = init_udpclient();

    /* ソケットをブロードキャスト可能にする */
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast_sw, sizeof(broadcast_sw)) == -1){
        exit_errmesg("setsockopt()");
    }

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(sock, &mask);

    /* サーバから文字列を受信して表示 */
    for(i = 0;(i < 3) && (mode != 'C');i++){

        /* 文字列をサーバに送信する */
        Sendto(sock, "HELO", strlen("HELO"), 0, (struct sockaddr *)&broadcast_adrs, sizeof(broadcast_adrs) );
        printf("-");

        for(;;){
            /* 受信データの有無をチェック */
            readfds = mask;
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;
            
            if( select( sock+1, &readfds, NULL, NULL, &timeout)==0 ) break;

            from_len = sizeof(from_adrs);
            strsize = Recvfrom(sock, r_buf, BUFSIZE-1, 0, (struct sockaddr *)&from_adrs, &from_len);
            r_buf[strsize] = '\0';
            printf("[%s] %s\n",inet_ntoa(from_adrs.sin_addr), r_buf);

            if(strcmp(r_buf,"HERE") == 0){
                mode = 'C';
                break;
            }
        }
    }

    close(sock);

    switch (mode)
    {
    case 'C':
        chat_client(from_adrs, port_number, username);
        break;
    case 'S':
        printf("Server: start up.\n");
        chat_server(port_number, username);
        break;
    }

    exit(EXIT_SUCCESS);
}