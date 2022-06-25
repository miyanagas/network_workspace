#include "mynet.h"
#include "chat.h"
#include <sys/select.h>

#define BUFSIZE 494
#define USERNAME_LEN 16

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct{
  int  sock;
  char name[USERNAME_LEN];
} client_info;

/* プライベート変数 */
static int N_client = 0;
static client_info *Client = NULL;  /* クライアントの情報 */
static int Max_sd = 0;

static void client_login(int sock);
static int receive_message(int sock, int post_client);
static void client_logout(int logout_client);
static void send_message(char *mesg, char *username, int post_client);

void chat_server(in_port_t port_number, char *my_username)
{
    struct sockaddr_in from_adrs;
    socklen_t from_len;
    int sock, sock_listen, sock_accepted, large_sd;
    fd_set mask, readfds;

    char r_buf[BUFSIZE], s_buf[BUFSIZE];
    int strsize, client_id;

    sock = init_udpserver(port_number);

    sock_listen = init_tcpserver(port_number, 5);

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(0, &mask);
    FD_SET(sock, &mask);
    FD_SET(sock_listen, &mask);

    large_sd = sock;
    if(sock_listen > large_sd){
        large_sd = sock_listen;
    }

    for(;;){

        readfds = mask;

        if(large_sd > Max_sd){
            Max_sd = large_sd;
        }
        
        select(Max_sd+1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(sock, &readfds)){
            from_len = sizeof(from_adrs);
            strsize = Recvfrom(sock, r_buf, BUFSIZE-1, 0, (struct sockaddr *)&from_adrs, &from_len);
            if(strcmp(r_buf, "HELO") == 0){
                Sendto(sock, "HERE", strlen("HERE"), 0, (struct sockaddr *)&from_adrs, sizeof(from_adrs));
            }
        }

        if(FD_ISSET(sock_listen, &readfds)){
            /* クライアントの接続を受け付ける */
            sock_accepted = Accept(sock_listen, NULL, NULL);
            client_login(sock_accepted);
            FD_SET(sock_accepted, &mask);
        }

        for( client_id = 0; client_id < N_client; client_id++){
            if(FD_ISSET(Client[client_id].sock, &readfds)){
                if( receive_message(Client[client_id].sock, client_id) == -1){
                    client_id--;
                }
            }
        }

        if(FD_ISSET(0, &readfds)){
            fgets(s_buf, BUFSIZE, stdin);
            send_message(s_buf, my_username, -1);
        }

    }
}

static void client_login(int sock)
{
    char r_buf[BUFSIZE];
    int client_id, strsize;

    strsize=Recv(sock, r_buf, BUFSIZE-1, 0);
    r_buf[strsize]='\0';
    if(strncmp(r_buf, "JOIN", 4) == 0){
        printf("%s\n", r_buf);
        N_client++;
        client_id = N_client-1;
        /* クライアント情報の保存用構造体 */
        if( (Client=(client_info *)realloc(Client, N_client*sizeof(client_info)))==NULL ){
            exit_errmesg("realloc()");
        }
        Client[client_id].sock = sock;
        strncpy(Client[client_id].name, &(r_buf[5]), USERNAME_LEN);
    }

    if(sock > Max_sd){
        Max_sd = sock;
    }
}

static int receive_message(int sock, int post_client)
{

    char buf[BUFSIZE];
    int strsize, client_id;

    strsize=Recv(sock, buf, BUFSIZE-1, 0);
    buf[strsize]='\0';
    if(strcmp(buf, "QUIT") == 0){
        if(sock == Max_sd){
            Max_sd = 0;
        }
        close(sock);
        client_logout(post_client);
        return (-1);
    }
    else if(strncmp(buf, "POST", 4) == 0){
        send_message(&(buf[5]), Client[post_client].name, post_client);
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
    int client_id;
    char send_message[BUFSIZE];

    snprintf(send_message, BUFSIZE, "MESG [%s]%s", username, mesg);

    for( client_id = 0; client_id < N_client; client_id++){
        if(client_id == post_client) continue;
        Send(Client[client_id].sock, send_message, strlen(send_message), 0);
    }

}