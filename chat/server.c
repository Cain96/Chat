#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXCLIENTS 5
#define PORTNUM 10140

int main(int argc, char **argv) {
    int sock, csock[5];
    struct sockaddr_in svr;
    struct sockaddr_in clt;
    struct hostent *cp;
    int clen;
    char buf[1024];
    char rbuf[1024];
    int nbytes;
    int reuse;
    fd_set rfds;
    struct timeval tv;
    int k = 0;
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
/* csockの初期化 */
    for(int i=0; i<5; i++){
      csokc[i] = -1;
    }

/* client 受付用ソケットの情報設定 */
    //bzero(&svr, sizeof(svr));
    memset(&svr, 0, sizeof(svr));
    svr.sin_family = AF_INET;
    svr.sin_addr.s_addr = htonl(INADDR_ANY);
/* 受付側の IP アドレスは任意 */
    svr.sin_port = htons(PORTNUM); /* ポート番号 10130 を介して受け付ける */
/* ソケットにソケットアドレスを割り当てる */
    if (bind(sock, (struct sockaddr *) &svr, sizeof(svr)) < 0) {
        perror("bind");
        exit(1);
    }
/* 待ち受けクライアント数の設定 */
    if (listen(sock, 30) < 0) { /* 待ち受け数に 30 を指定 */
        perror("listen");
        exit(1);
    }
    while (1) {
/* クライアントの受付 */
        clen = sizeof(clt);
        if ((csock[0] = accept(sock, (struct sockaddr *) &clt, &clen)) < 0) {
            perror("accept");
            exit(2);
        }
/* クライアントのホスト情報の取得 */
        cp = gethostbyaddr((char *) &clt.sin_addr, sizeof(struct in_addr),
                           AF_INET);
        printf("[%s]\n", cp->h_name);
      	while(1){
      		/* 入力を監視するファイル記述子の集合を変数 rfds にセットする */
      		FD_ZERO(&rfds); /* rfds を空集合に初期化 */
      		FD_SET(0,&rfds); /* 標準入力 */
      		FD_SET(csock[0],&rfds); /* クライアントを受け付けたソケット */
      		/* 監視する待ち時間を 1 秒に設定 */
      		tv.tv_sec = 1;
          tv.tv_usec = 0;
      		/* 標準入力とソケットからの受信を同時に監視する */
      		if(select(6, &rfds, NULL, NULL, &tv)>0) {
      			if(FD_ISSET(0,&rfds)) {
      				/* 標準入力から入力があったなら */
      				/* 標準入力から読み込みクライアントに送信 */
      				memset(buf, '\0', 1024);
      				if(read(0,buf, 1024)!=0){
      					write(csock, buf, sizeof(buf));
      				}else{
      					break;
      				}
      			}
      			if(FD_ISSET(csock,&rfds)) {
      				/* ソケットから受信したなら */
      				/* ソケットから読み込み端末に出力 */
      				if ((nbytes = read(csock, rbuf, sizeof(rbuf))) < 0) {
                      			perror("read");
      				} else if(nbytes == 0){
      					break;
        			} else {
          			write(1, rbuf, nbytes);
        			}
      			}
      		}
      	}  /* 繰り返す */
        close(csock);
        printf("closed\n");
    } /* 次の接続要求を繰り返し受け付ける */
}
