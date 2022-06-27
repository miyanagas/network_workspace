/*
  mynet.h
*/
#ifndef MYNET_H_
#define MYNET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int init_tcpserver(in_port_t myport, int backlog);
int init_tcpclient(char *servername, in_port_t serverport);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen); /* エラー処理付きaccept()関数 */
int Send(int s, void *buf, size_t len, int flags); /* エラー処理付きsend()関数 */
int Recv(int s, void *buf, size_t len, int flags); /* エラー処理付きrecv()関数 */

int init_udpserver(in_port_t myport);
int init_udpclient();
void set_sockaddr_in(struct sockaddr_in *server_adrs, char *servername, in_port_t port_number );
void set_sockaddr_in_broadcast(struct sockaddr_in *server_adrs, in_port_t port_number ); /* ブロードキャストアドレスの設定関数 */
int Sendto( int sock, const void *s_buf, size_t strsize, int flags, const struct sockaddr *to, socklen_t tolen);
int Recvfrom(int sock, void *r_buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);

void exit_errmesg(char *errmesg);
#endif  /* MYNET_H_ */