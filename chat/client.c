#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>

#define STRLEN 128
#define PORTNUM 10140

int main(int argc, char *argv[]) {
    char text[STRLEN];
    char *rtext;
    int sock;
    struct sockaddr_in svr;
    int reuse;
    struct hostent *host;
    int clen;
    char buf[1024];
    char rbuf[1024];
    int nbytes;
    fd_set rfds;
    struct timeval tv;
    char *user;

    /* 引数が2つであることの確認 */
    if (argc != 3) {
        perror("Illegal arguments");
        exit(1);
    }

    /* ユーザー名のチェック */
    char *p = argv[2];
    while (*p != '\0') {
      if(isalnum(*p) == 0){
        /* 英数字チェック*/
        if((*p != '-') && (*p != '_')){
          /* -(ハイフン),_(アンダーバー) チェック */
          fprintf(stderr, "Illegal username : %s\n", argv[2]);
          exit(1);
        }
      }
      p++;
    }
    strcpy(user, argv[2]);
    strcat(user, "\n");

    /* ソケットの生成 */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }
    /* ソケットアドレス再利用の指定 */
    reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    		perror("setsockopt");
    		exit(1);
    }

    memset((char *) &svr, 0, sizeof(svr));
    /* client_addr構造体にサーバの設定 */
    if((host = gethostbyname(argv[1])) == NULL){
  		  perror("setIP");
		    exit(1);
    }
	  bcopy(host->h_addr, &svr.sin_addr, host->h_length);
    svr.sin_family = AF_INET;
    svr.sin_port = htons(PORTNUM);

    /* connectでhostと接続 */
    if (connect(sock, (struct sockaddr *) &svr, sizeof(svr)) > 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    /* 参加 */
    if(read(sock, rbuf, 17) != 0){
      if(strncmp(rbuf, "REQUEST ACCEPTED\n", 17) != 0){
          perror("Not Accepted");
          close(sock);
          exit(1);
      }
    }

    /* ユーザ名登録 */
    printf("%s", user);
    write(sock, user, sizeof(user));
    if(read(sock, rbuf, 20) != 0){
      if(strncmp(rbuf, "USERNAME REGISTERED\n", 20) != 0){
          perror("Not Registered");
          close(sock);
          exit(1);
      }
    }

    do{
                /* 入力を監視するファイル記述子の集合を変数 rfds に
セットする */
  	    FD_ZERO(&rfds); /* rfds を空集合に初期化 */
        FD_SET(0,&rfds); /* 標準入力 */
        FD_SET(sock,&rfds); /* クライアントを受け付けたソケット */
        /* 監視する待ち時間を 1 秒に設定 */
        tv.tv_sec = 1; tv.tv_usec = 0;
        /* 標準入力とソケットからの受信を同時に監視する */
        if(select(sock+1,&rfds,NULL,NULL,&tv)>0) {
            if(FD_ISSET(0,&rfds)) {
                /* 標準入力から入力があったなら */
                /* 標準入力から読み込みクライアント
に送信 */
                memset(buf, '\0', 1024);
                if((nbytes = read(0,buf, 1024))!=0){
                    write(sock, buf, nbytes);
                }else{
                    printf("break");
                    break;
                }
            }
            if(FD_ISSET(sock,&rfds)) {
                /* ソケットから受信したなら */
                /* ソケットから読み込み端末に出力 */
                if ((nbytes = read(sock, rbuf, sizeof(rbuf))) < 0) {
                    perror("read");
                } else if(nbytes == 0){
			break;
		} else {
                    write(1, rbuf, nbytes);
                }
            }
        }
    }while(1); /* 繰り返す */
    close(sock);
    printf("closed\n");
}
