/*====================================================
 *    chat_client.c
 *==================================================*/
#include "chat.h"

#define S_BUFSIZE MESG_LEN /* 送信バッファサイズ（発言メッセージ） */
#define R_BUFSIZE MESG_BUFSIZE /* 受信バッファサイズ（MESGパケット） */

/* クライアントのメッセージをサーバに送信する関数 */
static int post_message(int sock, char *mesg);

void chat_client(struct sockaddr_in server_adrs, in_port_t port_number,
                 char *username) {
    int sock, strsize;
    char s_buf[S_BUFSIZE], r_buf[R_BUFSIZE]; /* 送信バッファ、受信バッファ */
    fd_set mask, readfds;

    /* ソケットをSTREAMモードで作成する */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        exit_errmesg("sock()");
    }

    /* ソケットにサーバの情報を対応づけてサーバに接続する */
    if (connect(sock, (struct sockaddr *)&server_adrs, sizeof(server_adrs)) ==
        -1) {
        exit_errmesg("connect()");
    }

    printf("Connected to Sever:)\n");

    /* ユーザの参加をサーバに知らせる */
    snprintf(s_buf, JOIN_BUFSIZE, "JOIN %s", username);
    Send(sock, s_buf, strlen(s_buf), 0);

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(sock, &mask);
    FD_SET(0, &mask);

    for (;;) {
        /* 受信データの有無をチェック */
        readfds = mask;

        if (select(sock + 1, &readfds, NULL, NULL, NULL) == -1) {
            exit_errmesg("select()");
        }

        if (FD_ISSET(sock, &readfds)) {
            /* MESGパケットの受信と表示 */
            if ((strsize = Recv(sock, r_buf, R_BUFSIZE - 1, 0)) == 0) {
                printf("Server is down:(\n");
                close(sock);
                return;
            }
            r_buf[strsize] = '\0';
            if (strncmp(r_buf, "MESG", 4) == 0) {
                printf("%s\n", chop_nl(&(r_buf[5])));
            }
        }

        if (FD_ISSET(0, &readfds)) {
            /* メッセージ（POSTまたはQUITパケット）の送信 */
            if (fgets(s_buf, S_BUFSIZE, stdin) == NULL) {
                exit_errmesg("fgets()");
            }
            if (post_message(sock, chop_nl(s_buf)) == -1) return;
        }
    }
}

static int post_message(int sock, char *mesg) {
    char post_message[POST_BUFSIZE]; /* POSTパケットの送信バッファ */

    if (strcmp(mesg, "QUIT") == 0) {
        /* QUITパケットの送信 */
        Send(sock, "QUIT", 4, 0);
        printf("Bye Bye:)\n");
        close(sock);
        return (-1);
    } else {
        /* POSTパケットの送信 */
        snprintf(post_message, POST_BUFSIZE, "POST %s", mesg);
        Send(sock, post_message, strlen(post_message), 0);
        return (0);
    }
}

char *chop_nl(char *s) {
    int len;
    len = strlen(s);

    if (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        if (len > 1 && s[len - 2] == '\r') {
            s[len - 2] = '\0';
        }
    }

    return s;
}