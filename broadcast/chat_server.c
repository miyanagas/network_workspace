#include "chat.h"
#include <errno.h>

#define S_BUFSIZE MESG_BUFSIZE /* 送信バッファサイズ */
#define R_BUFSIZE POST_BUFSIZE /* 受信バッファサイズ */

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct client_tag {
  int  sock;
  char name[USERNAME_LEN];
  struct client_tag *prev;
  struct client_tag *next;
} client_info;

/* プライベート変数 */
static client_info *Client_header = NULL;  /* クライアント情報 */
static int Max_sd = 0; /* ソケットディスクリプタ最大値 */
static fd_set Mask;

/* プライベート関数 */
static void client_login(int sock); /* クライアントの情報を記録する（ログイン）関数 */
static int receive_message(int sock, client_info *post_client); /* クライアントからのメッセージを受信する関数 */
static void client_logout(client_info **logout_client); /* クライアントの情報を抹消する（ログアウト）関数 */
static void send_message(char *mesg, char *username, client_info *post_client); /* クライアントにメッセージを送信する関数 */

void chat_server(in_port_t port_number, char *my_username)
{
    struct sockaddr_in from_adrs;
    socklen_t from_len;
    int sock, sock_listen, sock_accepted;
    int large_sd;
    fd_set readfds;

    char buf[MESG_BUFSIZE];
    int strsize;
    client_info *p, **q;

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
                printf("recv HELO\n");
            }
        }

        if(FD_ISSET(sock_listen, &readfds)){
            /* クライアントの接続を受け付ける */
            printf("ready Accept\n");
            sock_accepted = Accept(sock_listen, NULL, NULL);
            printf("accepted\n");
            /* 新たに接続したしたクライアントのビットマスクのセット */
            FD_SET(sock_accepted, &Mask);
            client_login(sock_accepted);           
        }

        if(Client_header != NULL){
            p = Client_header;
            q = &Client_header;
            while(p != NULL){
                if(FD_ISSET(p->sock, &readfds)){
                    /* クライアントのメッセージを受信 */
                    if( receive_message(p->sock, p) == -1){
                        client_logout(q);
                        continue;
                    }
                }
                p = p->next;
                q = &((*q)->next);
            }
        }

        if(FD_ISSET(0, &readfds)){
            /* 自身のキーボードから入力したメッセージを全クライアントに送信 */
            fgets(buf, MESG_LEN+1, stdin);
            send_message(chop_nl(buf), my_username, NULL);
        }

    }
}

static void client_login(int sock)
{
    client_info *new_client, *p, **q;

    if( (new_client=(client_info*)malloc(sizeof(client_info)))==NULL ){
        exit_errmesg("malloc()");
    }
    new_client->sock = sock;
    strncpy(new_client->name, "\0", USERNAME_LEN);
    new_client->prev = NULL;
    new_client->next = NULL;

    if(Client_header == NULL){
        Client_header = new_client;
    }
    else{
        for( p = Client_header, q = &Client_header; p != NULL; p = p->next, q = &((*q)->next) ){
            if(sock <= p->sock){
                new_client->next = p->next;
                p->next = new_client;
            }else{
                new_client->next = p;
                *q = new_client;
            }
        }
        if(p == NULL){
            *q = new_client;
        }
    }

    Max_sd = Client_header->sock;

}

static int receive_message(int sock, client_info *post_client)
{

    char r_buf[R_BUFSIZE];
    int strsize;
    client_info *pos;

    strsize=Recv(sock, r_buf, R_BUFSIZE-1, 0);
    r_buf[strsize]='\0';
    if(strcmp(r_buf, "QUIT") == 0){
        /* クライアントとの接続を終了 */
        FD_CLR(sock, &Mask);
        close(sock);
        return (-1);
    }
    else if(strncmp(r_buf, "POST", 4) == 0){
        /* 他のクライアントにメッセージを送信 */
        send_message(&(r_buf[5]), post_client->name, post_client);
        printf("[%s]%s\n", post_client->name, chop_nl(&(r_buf[5])));
    }
    else if(strncmp(r_buf, "JOIN", 4) == 0){
        for( pos = Client_header; pos != NULL ; pos = pos->next ){
            if(pos->sock == sock){
                strncpy(pos->name, &(r_buf[5]), USERNAME_LEN);
                printf("join\n");
            }
        }
    }

    return (0);
}

static void client_logout(client_info **logout_client)
{
    client_info *temp;

    temp = (*logout_client)->next;
    (*logout_client) = temp;
    free(temp);

    Max_sd = Client_header->sock;

}

static void send_message(char *mesg, char *username, client_info *post_client)
{
    char send_mesg[S_BUFSIZE];
    client_info *p, **q;

    snprintf(send_mesg, S_BUFSIZE, "MESG [%s]%s", username, mesg);
    p = Client_header;
    q = &Client_header;
    while(p != NULL){
        if(p != post_client){
            if(send(p->sock, send_mesg, strlen(send_mesg), MSG_NOSIGNAL) == -1){
                if(errno == EPIPE){
                    FD_CLR(p->sock, &Mask);
                    client_logout(q);
                    continue;
                }
            }
        }
        p = p->next;
        q = &((*q)->next);
    }

}