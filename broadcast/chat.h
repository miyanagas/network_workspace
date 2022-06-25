#ifndef CHAT_H_
#define CHAT_H_

#include "mynet.h"

/* サーバメインルーチン */
void chat_server(in_port_t port_number, char *my_username);

/* クライアントメインルーチン */
void chat_client(struct sockaddr_in server_adrs, in_port_t port_number, char *username);


int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);

/* 送信関数(エラー処理つき) */
int Send(int s, void *buf, size_t len, int flags);

/* 受信関数(エラー処理つき) */
int Recv(int s, void *buf, size_t len, int flags);


char *chop_nl(char *s);

#endif