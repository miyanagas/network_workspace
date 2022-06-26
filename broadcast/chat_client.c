#include "chat.h"

#define S_BUFSIZE 495 /*  */
#define R_BUFSIZE 511 /*  */

/*  */
static int post_message(int sock, char *mesg);

void chat_client(struct sockaddr_in server_adrs, in_port_t port_number, char *username){

    int sock, strsize;
    char s_buf[S_BUFSIZE], r_buf[R_BUFSIZE];
    fd_set mask, readfds;
    
    /* ソケットをSTREAMモードで作成する */
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        exit_errmesg("sock()");
    }

    /* ソケットにサーバの情報を対応づけてサーバに接続する */
    if(connect(sock, (struct sockaddr *)&server_adrs, sizeof(server_adrs))== -1){
        exit_errmesg("connect()");
    }

    /*  */
    snprintf(s_buf, USERNAME_LEN+5, "JOIN %s", username);
    Send(sock, s_buf, strlen(s_buf), 0);

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(sock, &mask);
    FD_SET(0, &mask);

    for(;;){
        /* 受信データの有無をチェック */
        readfds = mask;
        
        select( sock+1, &readfds, NULL, NULL, NULL );

        if(FD_ISSET(sock, &readfds)){
            strsize = Recv(sock, r_buf, R_BUFSIZE-1, 0);
            r_buf[strsize] = '\0';
            if(strncmp(r_buf, "MESG", 4) == 0){
                printf("%s\n", r_buf);
            }
        }

        if(FD_ISSET(0, &readfds)){
            fgets(s_buf, S_BUFSIZE-5, stdin);
            if(post_message(sock, chop_nl(s_buf)) == -1) return;
        }
    }
    
}

static int post_message(int sock, char *mesg)
{
    char post_message[S_BUFSIZE];

    if(strcmp(mesg, "QUIT") == 0){
        Send(sock, "QUIT", 5, 0);
        close(sock);
        return (-1);
    }
    else{
        snprintf(post_message, S_BUFSIZE, "POST %s", mesg);
        Send(sock, post_message, strlen(post_message), 0);
        return (0);
    }

}

char *chop_nl(char *s)
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