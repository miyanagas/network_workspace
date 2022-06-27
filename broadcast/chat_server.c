#include "chat.h"
#include <errno.h>

#define S_BUFSIZE MESG_BUFSIZE /* 送信バッファサイズ */
#define R_BUFSIZE POST_BUFSIZE /* 受信バッファサイズ */

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct{
  int  sock;
  char name[USERNAME_LEN];
} client_info;

/* プライベート変数 */
static int N_client = 0; /* クライアント数 */
static client_info *Client = NULL;  /* クライアント情報 */
static int Max_sd = 0; /* ソケットディスクリプタ最大値 */
static fd_set Mask;

/* プライベート関数 */
static void client_login(int sock); /* クライアントの情報を記録する（ログイン）関数 */
static int receive_message(int sock, int post_client); /* クライアントからのメッセージを受信する関数 */
static void client_logout(int logout_client); /* クライアントの情報を抹消する（ログアウト）関数 */
static void send_message(char *mesg, char *username, int post_client); /* クライアントにメッセージを送信する関数 */

void chat_server(in_port_t port_number, char *my_username)
{
    struct sockaddr_in from_adrs;
    socklen_t from_len;
    int sock, sock_listen, sock_accepted;
    int large_sd;
    fd_set readfds;

    char buf[MESG_BUFSIZE];
    int strsize, client_id;

    /* UDPサーバ初期化 */
    sock = init_udpserver(port_number);

    /* TCPサーバ初期化 */
    sock_listen = init_tcpserver(port_number, 5);

    /* ビットマスクの準備 */
    FD_ZERO(&Mask);
    FD_SET(0, &Mask);
    FD_SET(sock, &Mask);
    FD_SET(sock_listen, &Mask);

    large_sd = sock;
    if(sock_listen > large_sd){
        large_sd = sock_listen;
    }

    for(;;){

        readfds = Mask;

        if(large_sd > Max_sd){
            Max_sd = large_sd;
        }
        
        select(Max_sd+1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(sock, &readfds)){
            /* HELOパケットの受信 */
            from_len = sizeof(from_adrs);
            strsize = Recvfrom(sock, buf, 4, 0, (struct sockaddr *)&from_adrs, &from_len);
            buf[strsize] = '\0';
            if(strcmp(buf, "HELO") == 0){
                /* HEREパケットの送信 */
                Sendto(sock, "HERE", 4, 0, (struct sockaddr *)&from_adrs, sizeof(from_adrs));
            }
        }

        if(FD_ISSET(sock_listen, &readfds)){
            /* クライアントの接続を受け付ける */
            sock_accepted = Accept(sock_listen, NULL, NULL);
            /* 新たに接続したしたクライアントのビットマスクのセット */
            FD_SET(sock_accepted, &Mask);
            client_login(sock_accepted);           
        }

        for( client_id = 0; client_id < N_client; client_id++){
            if(FD_ISSET(Client[client_id].sock, &readfds)){
                /* クライアントのメッセージを受信 */
                if( receive_message(Client[client_id].sock, client_id) == -1){
                    client_logout(client_id);
                    client_id--;
                }
            }
        }

        if(FD_ISSET(0, &readfds)){
            /* 自身のキーボードから入力したメッセージを全クライアントに送信 */
            fgets(buf, MESG_LEN+1, stdin);
            send_message(chop_nl(buf), my_username, -1);
        }

    }
}

static void client_login(int sock)
{
    char r_buf[JOIN_BUFSIZE];
    int client_id, strsize;

    strsize=Recv(sock, r_buf, JOIN_BUFSIZE-1, 0);
    r_buf[strsize]='\0';
    if(strncmp(r_buf, "JOIN", 4) == 0){
        N_client++;
        client_id = N_client-1;
        /* クライアント情報の保存用構造体の構築（拡張） */
        if( (Client=(client_info *)realloc(Client, N_client*sizeof(client_info)))==NULL ){
            exit_errmesg("realloc()");
        }
        /* クライアント情報の記録 */
        Client[client_id].sock = sock;
        strncpy(Client[client_id].name, &(r_buf[5]), USERNAME_LEN);

        if(sock > Max_sd){
            Max_sd = sock;
        }
    }

}

static int receive_message(int sock, int post_client)
{

    char r_buf[R_BUFSIZE];
    int strsize, client_id;

    strsize=Recv(sock, r_buf, R_BUFSIZE-1, 0);
    r_buf[strsize]='\0';
    if(strcmp(r_buf, "QUIT") == 0){
        /* クライアントとの接続を終了 */
        if(sock == Max_sd){
            Max_sd = 0;
        }
        FD_CLR(sock, &Mask);
        close(sock);
        return (-1);
    }
    else if(strncmp(r_buf, "POST", 4) == 0){
        /* 他のクライアントにメッセージを送信 */
        send_message(&(r_buf[5]), Client[post_client].name, post_client);
        printf("[%s]%s\n", Client[post_client].name, chop_nl(&(r_buf[5])));
    }

    return (0);
}

static void client_logout(int logout_client)
{
    int client_id;

    N_client--;

    for( client_id = 0; client_id < N_client; client_id++){
        if(client_id >= logout_client){
            Client[client_id].sock = Client[client_id+1].sock;
            strncpy(Client[client_id].name, Client[client_id+1].name, USERNAME_LEN);
        }
        if(Client[client_id].sock > Max_sd){
            Max_sd = Client[client_id].sock;
        }
    }

    Client[client_id].sock = 0;
    strncpy(Client[client_id].name, "\0", USERNAME_LEN);

}

static void send_message(char *mesg, char *username, int post_client)
{
    char send_mesg[S_BUFSIZE];
    int client_id;

    snprintf(send_mesg, S_BUFSIZE, "MESG [%s]%s", username, mesg);
    for(client_id = 0; client_id < N_client; client_id++){
        if(client_id == post_client) continue;
        if(send(Client[client_id].sock, send_mesg, strlen(send_mesg), MSG_NOSIGNAL) == -1){
            if(errno == EPIPE){
                if(Client[client_id].sock == Max_sd){
                    Max_sd = 0;
                }
                FD_CLR(Client[client_id].sock, &Mask);
                client_logout(client_id);
                client_id--;
            }
        }
    }

}