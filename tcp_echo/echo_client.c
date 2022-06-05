#include "mynet.h"

#define PORT 50000         /* ポート番号 */
#define S_BUFSIZE 512   /* 送信用バッファサイズ */
#define R_BUFSIZE 512   /* 受信用バッファサイズ */

int main(int argc, char *argv[])
{
  int sock;
  char s_buf[S_BUFSIZE], r_buf[R_BUFSIZE];
  int strsize;

  if( argc != 3){
      fprintf(stderr, "Usage: %s Server_name Port_number\n", argv[0]);
      exit(EXIT_FAILURE);
  }

  /* サーバに接続する */
  sock = init_tcpclient(argv[1], atoi(argv[2])); /* ←ライブラリの関数を使った */

  /* キーボードから文字列を入力する */
  fgets(s_buf, S_BUFSIZE, stdin);
  strsize = strlen(s_buf);

  /* 文字列をサーバに送信する */
  if( send(sock, s_buf, strsize, 0) == -1 ){
    exit_errmesg("send()");
  }

  /* サーバから文字列を受信する */
  do{
    if((strsize=recv(sock, r_buf, R_BUFSIZE-1, 0)) == -1){
      exit_errmesg("recv()");
    }
    r_buf[strsize] = '\0';
    printf("%s",r_buf);      
  }while( r_buf[strsize-1] != '\n' );

  close(sock);             /* ソケットを閉じる */

  exit(EXIT_SUCCESS);
}