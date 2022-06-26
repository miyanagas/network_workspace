#ifndef CHAT_H_
#define CHAT_H_

#include "mynet.h"
#include <sys/select.h>

#define MESG_LEN 489 /* メッセージバッファサイズ（発言メッセージ488バイト以内） */
#define USERNAME_LEN 16 /* ユーザ名サイズ（ユーザ名15文字以内） */

/* サーバメインルーチン */
void chat_server(in_port_t port_number, char *my_username);

/* クライアントメインルーチン */
void chat_client(struct sockaddr_in server_adrs, in_port_t port_number, char *username);


char *chop_nl(char *s);

#endif