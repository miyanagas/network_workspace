#include "chat.h"
#include "mynet.h"
#include <stdlib.h>
#include <unistd.h>

#define SERVER_LEN 256     /* サーバ名格納用バッファサイズ */
#define DEFAULT_PORT 50000 /* ポート番号既定値 */
#define DEFAULT_MODE 'C'   /* 省略時はクライアント */

extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char *argv[])
{
  int port_number=DEFAULT_PORT;
  char servername[SERVER_LEN] = "localhost";
  char mode = DEFAULT_MODE;
  int c;

  /* オプション文字列の取得 */
  opterr = 0;
  while( 1 ){
	  c = getopt(argc, argv, "SCs:p:h");
	  if( c == -1 ) break;
	  
	  switch( c ){
	  case 'S' :  /* サーバモードにする */
	    mode = 'S';
	    break;

	  case 'C':   /* クライアントモードにする */
	    mode = 'C';
	    break;

	  case 's' :  /* サーバ名の指定 */
	    snprintf(servername, SERVER_LEN, "%s", optarg);
	    break;

	  case 'p':  /* ポート番号の指定 */
	    port_number = atoi(optarg);
	    break;

	  case '?' :
	    fprintf(stderr,"Unknown option '%c'\n", optopt );
	  case 'h' :
	    fprintf(stderr,"Usage(Server): %s -S -p port_number\n", argv[0]);
	    fprintf(stderr,"Usage(Client): %s -C -s server_name -p port_number\n", argv[0]);
	    exit(EXIT_FAILURE);
	    break;
	  }
  }

  switch(mode){

  case 'S':
  	chat_server(port_number);
    break;
  case 'C':
  	chat_client(servername, port_number);
    break;

  }

  exit(EXIT_SUCCESS);
}