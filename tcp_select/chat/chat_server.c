#include "chat.h"
#include "mynet.h"
#include <stdlib.h>
#include <unistd.h>

void chat_server(int port_number)
{
  int sock_listen;

  /* サーバの初期化 */
  sock_listen = init_tcpserver(port_number, 5);

  /* クライアントの接続 */
  init_client(sock_listen);

  close(sock_listen);

  /* メインループ */
  chat_loop();

}