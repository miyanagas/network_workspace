/*====================================================
 *    chat.h
 *==================================================*/
#ifndef CHAT_H_
#define CHAT_H_

#include <sys/select.h>

#include "mynet.h"

#define MESG_LEN 488 /* 発言メッセージ長（発言メッセージ488バイト以内） */
#define USERNAME_LEN 16 /* ユーザ名文字列長（ユーザ名15文字以内） */

#define JOIN_BUFSIZE USERNAME_LEN + 5 /* JOINパケットバッファサイズ */
#define POST_BUFSIZE MESG_LEN + 5 /* POSTパケットバッファサイズ */
#define MESG_BUFSIZE \
    MESG_LEN + USERNAME_LEN + 6 /* MESGパケットバッファサイズ */

/* サーバメインルーチン */
void chat_server(in_port_t port_number, char *my_username);

/* クライアントメインルーチン */
void chat_client(struct sockaddr_in server_adrs, in_port_t port_number,
                 char *username);

/* 改行コードの削除 */
char *chop_nl(char *s);

#endif