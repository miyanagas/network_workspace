#include "mynet.h"
#include "chat.h"
#include <stdlib.h>
#include <sys/select.h>

#define NAMELENGTH 20 /* ログイン名の長さ制限 */
#define BUFLEN 500    /* 通信バッファサイズ */
#define N_CLIENT 3

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct{
  int  sock;
  char name[NAMELENGTH];
} client_info;

/* プライベート変数 */
static client_info *Client;  /* クライアントの情報 */
static int Max_sd;               /* ディスクリプタ最大値 */
static char Buf[BUFLEN];     /* 通信用バッファ */

/* プライベート関数 */
static int client_login(int sock_listen);
static int receive_message();
static char *chop_nl(char *s);

void init_client(int sock_listen)
{

  /* クライアント情報の保存用構造体の初期化 */
  if( (Client=(client_info *)malloc(N_CLIENT*sizeof(client_info)))==NULL ){
    exit_errmesg("malloc()");
  }

  /* クライアントのログイン処理 */
  Max_sd = client_login(sock_listen);

}

void chat_loop()
{
  for(;;){
    /* 解答の受信 */
    if(receive_message() == -1){
      printf("Bye\n");
      return;
    }
  }
}

static int client_login(int sock_listen)
{
  int client_id,sock_accepted;
  static char prompt[]="Input your name: ";
  char loginname[NAMELENGTH];
  int strsize;

  for( client_id=0; client_id<N_CLIENT; client_id++){
    /* クライアントの接続を受け付ける */
    sock_accepted = Accept(sock_listen, NULL, NULL);
    printf("Client[%d] connected.\n",client_id);

    /* ログインプロンプトを送信 */
    Send(sock_accepted, prompt, strlen(prompt), 0);

    /* ログイン名を受信 */
    strsize = Recv(sock_accepted, loginname, NAMELENGTH-1, 0);
    loginname[strsize] = '\0';
    chop_nl(loginname);

    /* ユーザ情報を保存 */
    Client[client_id].sock = sock_accepted;
    strncpy(Client[client_id].name, loginname, NAMELENGTH);
  }

  return(sock_accepted);

}

static int receive_message()
{
  fd_set mask, readfds;
  int client_id, client_id_;
  int message;
  int strsize;
  char send_message[BUFLEN];

  /* ビットマスクの準備 */
  FD_ZERO(&mask);
  for(client_id=0; client_id<N_CLIENT; client_id++){
    FD_SET(Client[client_id].sock, &mask);
  }

  for(;;){
    
    /* 受信データの有無をチェック */
    readfds = mask;
    select( Max_sd+1, &readfds, NULL, NULL, NULL );

    for( client_id=0; client_id<N_CLIENT; client_id++ ){

      if( FD_ISSET(Client[client_id].sock, &readfds) ){

        strsize = Recv(Client[client_id].sock, Buf, BUFLEN-1,0);
        Buf[strsize]='\0';
        strcpy(send_message, "[");
        strcat(send_message, Client[client_id].name);
        strcat(send_message, "]: ");
        strcat(send_message, Buf);
        chop_nl(Buf);

        if(strcmp(Buf,"quit") == 0){
          for(client_id_=0; client_id_<N_CLIENT; client_id_++){
            Send(Client[client_id_].sock, "Bye", strlen("Bye"), 0);
            close(Client[client_id_].sock);
          }
          free(Client);
          return (-1);
        }else{
            for(client_id_=0; client_id_<N_CLIENT; client_id_++){
                if(client_id_ != client_id){
                    Send(Client[client_id_].sock, send_message, strlen(send_message), 0);
                }
            }
        }
      }
    }
  }

  return (0);
}

static char *chop_nl(char *s)
{
  int len;
  len = strlen(s);

  if( len>0 && (s[len-1]=='\n' || s[len-1]=='\r') ){
    s[len-1] = '\0';
    if( len>1 && s[len-2]=='\r'){
      s[len-2] = '\0';
    }
  }

  return s;
}