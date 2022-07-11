#include "chat.h"
#include <errno.h>

#define S_BUFSIZE MESG_BUFSIZE /* 送信バッファサイズ */
#define R_BUFSIZE POST_BUFSIZE /* 受信バッファサイズ */

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct client_tag {
  int  sock;
  char name[USERNAME_LEN];
  struct client_tag *next;
} client_info;

/* プライベート変数 */
static client_info *Client_header = NULL;  /* クライアント情報 */
static int N_client = 0; /* クライアント数 */
static int Max_sd = 0; /* ソケットディスクリプタ最大値 */
static fd_set Mask; /* セレクトビットマスク */

/* プライベート関数 */
static void client_login(int sock); /* クライアントの情報を記録する（ログイン）関数 */
static int receive_message(int sock, client_info *post_client); /* クライアントからのメッセージを受信する関数 */
static void client_logout(client_info *before_logout_client, client_info *logout_client); /* クライアントの情報を抹消する（ログアウト）関数 */
static void send_message(char *mesg, char *username, client_info *post_client); /* クライアントにメッセージを送信する関数 */

void chat_server(in_port_t port_number, char *my_username)
{
    struct sockaddr_in from_adrs;
    socklen_t from_len;
    int udp_sock, tcp_sock, sock_accepted;
    fd_set readfds;
    struct timeval timeout;

    char buf[MESG_LEN];
    int large_sd, strsize;
    client_info *before_ptr, *after_ptr;

    /* クライアント情報を保存する線形リストのダミー先頭要素 */
    if( (Client_header=(client_info*)malloc(sizeof(client_info)))==NULL ){
        exit_errmesg("malloc()");
    }
    strncpy(Client_header->name, "", USERNAME_LEN);
    Client_header->sock = 0;
    Client_header->next = NULL;

    /* UDPサーバ初期化 */
    udp_sock = init_udpserver(port_number);

    /* TCPサーバ初期化 */
    tcp_sock = init_tcpserver(port_number, 5);

    /* ビットマスクの準備 */
    FD_ZERO(&Mask);
    FD_SET(0, &Mask);
    FD_SET(udp_sock, &Mask);
    FD_SET(tcp_sock, &Mask);

    if(tcp_sock > udp_sock){
        large_sd = tcp_sock;
    }else{
        large_sd = udp_sock;
    }

    for(;;){

        readfds = Mask;

        if(large_sd > Max_sd){
            Max_sd = large_sd;
        }
        
        if( select(Max_sd+1, &readfds, NULL, NULL, NULL)==0 ){
            exit_errmesg("select()");
        }

        if(FD_ISSET(udp_sock, &readfds)){
            /* HELOパケットの受信 */
            from_len = sizeof(from_adrs);
            strsize = Recvfrom(udp_sock, buf, 4, 0, (struct sockaddr *)&from_adrs, &from_len);
            buf[strsize] = '\0';
            if( strcmp(buf, "HELO") == 0 ){
                /* HEREパケットの送信 */
                Sendto(udp_sock, "HERE", 4, 0, (struct sockaddr *)&from_adrs, sizeof(from_adrs));
            }
        }

        if(FD_ISSET(tcp_sock, &readfds)){
            /* クライアントの接続を受け付ける */
            sock_accepted = Accept(tcp_sock, NULL, NULL);
            /* クライアント情報（ユーザ名を除く）の追加 */
            client_login(sock_accepted);
        }

        for( before_ptr = Client_header, after_ptr = Client_header->next; after_ptr != NULL; before_ptr = after_ptr, after_ptr = after_ptr->next ){
            if(FD_ISSET(after_ptr->sock, &readfds)){
                /* クライアントからメッセージを受信 */
                if( receive_message(after_ptr->sock, after_ptr)==-1 ){
                    /* クライアント情報の削除 */
                    client_logout(before_ptr, after_ptr);
                    after_ptr = before_ptr;
                }
            }
        }

        if(FD_ISSET(0, &readfds)){
            /* 自身のキーボードから入力したメッセージを全クライアントに送信 */
            fgets(buf, MESG_LEN, stdin);
            send_message(chop_nl(buf), my_username, NULL);
        }

    }
}

static void client_login(int sock)
{
    client_info *new_client, *before_ptr, *after_ptr;

    if( (new_client=(client_info*)malloc(sizeof(client_info)))==NULL ){
        exit_errmesg("malloc()");
    }
    new_client->sock = sock;
    strncpy(new_client->name, "unknown", USERNAME_LEN);

    for( before_ptr = Client_header, after_ptr = Client_header->next; after_ptr != NULL; before_ptr = after_ptr, after_ptr = after_ptr->next ){
        if(sock > after_ptr->sock){
            before_ptr->next = new_client;
            new_client->next = after_ptr;
            break;
        }
    }
    if(after_ptr == NULL){
        before_ptr->next = new_client;
        new_client->next = NULL;
    }

    Max_sd = (Client_header->next)->sock;

    N_client++;
    printf("[INFO] user join!\n");
    printf("[INFO] online_user %d\n", N_client);
    /* 新たに接続したしたクライアントのビットマスクのセット */
    FD_SET(sock, &Mask);

}

static int receive_message(int sock, client_info *post_client)
{

    char r_buf[R_BUFSIZE];
    int strsize;
    client_info *pos;

    strsize=Recv(sock, r_buf, R_BUFSIZE-1, 0);
    r_buf[strsize]='\0';
    chop_nl(r_buf);
    if( strcmp(r_buf, "QUIT") == 0 || strsize == 0 ){
        /* クライアントとの接続を終了 */
        close(sock);
        return (-1);
    }
    else if( strncmp(r_buf, "POST", 4) == 0 ){
        /* 他のクライアントにメッセージを送信 */
        send_message(&(r_buf[5]), post_client->name, post_client);
        printf("[%s]%s\n", post_client->name, &(r_buf[5]));
    }
    else if( strncmp(r_buf, "JOIN", 4) == 0 ){
        for( pos = Client_header->next; pos != NULL ; pos = pos->next ){
            if( pos->sock == sock ){
                /* クライアントのユーザ名情報の追加 */
                strncpy(pos->name, &(r_buf[5]), USERNAME_LEN-1);
                (pos->name)[USERNAME_LEN] = '\0';
                printf("[INFO] join %s!\n",pos->name);
                break;
            }
        }
    }

    return (0);
}

static void client_logout(client_info *before_logout_client, client_info *logout_client)
{
    N_client--;
    printf("[INFO] %s leave...\n", logout_client->name);
    printf("[INFO] online_user %d\n", N_client);
    /* 接続終了したクライアントのビットマスクのリセット */
    FD_CLR(logout_client->sock, &Mask);

    before_logout_client->next = logout_client->next;
    free(logout_client);

    if( Client_header->next != NULL ){
        Max_sd = (Client_header->next)->sock;
    }else{
        Max_sd = 0;
    }

}

static void send_message(char *mesg, char *username, client_info *post_client)
{
    char send_mesg[S_BUFSIZE];
    client_info *before_ptr, *after_ptr;

    snprintf(send_mesg, S_BUFSIZE, "MESG [%s]%s", username, mesg);
    
    for( before_ptr = Client_header, after_ptr = Client_header->next; after_ptr != NULL; before_ptr = after_ptr, after_ptr = after_ptr->next ){
        if( after_ptr != post_client ){
            if( send(after_ptr->sock, send_mesg, strlen(send_mesg), MSG_NOSIGNAL)==-1 ){
                if( errno == EPIPE ){
                    /* 接続が切れたクライアント情報の削除 */
                    close(after_ptr->sock);
                    client_logout(before_ptr, after_ptr);
                    after_ptr = before_ptr;
                }else{
                    exit_errmesg("send()");
                }
            }
        }
    }

}