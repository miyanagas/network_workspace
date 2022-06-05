#include "mynet.h"

#define BUFSIZE 50

int main(int argc, char *argv[])
{
    struct sockaddr_in from_adrs;
    int sock_listen, sock_accepted;

    char buf[BUFSIZE];
    int strsize;

    /* 引数のチェックと使用法の表示 */
    if( argc != 2 ){
        fprintf(stderr,"Usage: %s Port_number\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sock_listen = init_tcpserver(atoi(argv[1]), 5);

    sock_accepted = accept(sock_listen, NULL, NULL);

    close(sock_listen);

    do{
        if((strsize=recv(sock_accepted, buf, BUFSIZE, 0)) == -1){
            exit_errmesg("recv()");
        }

        if(send(sock_accepted, buf, strsize, 0) == -1){
            exit_errmesg("send()");
        }
    }while( buf[strsize-1] != '\n');

    close(sock_accepted);

    exit(EXIT_SUCCESS);
}