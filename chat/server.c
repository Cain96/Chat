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
    int sock, csock[MAXCLIENTS +1];
    char user[MAXCLIENTS][1024];
    struct sockaddr_in svr;
    struct sockaddr_in clt;
    int clen;
    char rbuf[1024];
    int nbytes;
    int reuse;
    fd_set rfds;
    struct timeval tv;
    int k = 0, i, user_count, state, sock_num, strlength;

    if (SIG_ERR == signal(SIGINT, sigcatch)) {
      printf("failed to set signal handler.\n");
      exit(1);
    }

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
    for(int i=0; i<=MAXCLIENTS; i++){
      csock[i] = -1;
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
    state = 1;
    while (1) {
      switch (state) {
        case 1:
        /* 入力待ち, 入力処理 */
    		/* 入力を監視するファイル記述子の集合を変数 rfds にセットする */
    		FD_ZERO(&rfds); /* rfds を空集合に初期化 */
        FD_SET(sock,&rfds);
        for(int i=0; i<k; i++){
    		  FD_SET(csock[i],&rfds); /* クライアントを受け付けたソケット */
        }
    		/* 監視する待ち時間を 1 秒に設定 */
    		tv.tv_sec = 1;
        tv.tv_usec = 0;
    		/* 標準入力とソケットからの受信を同時に監視する */
    		if(select(7, &rfds, NULL, NULL, &tv)>0) {
          if(FD_ISSET(sock,&rfds)) {
            state = 2;
          } else {
            for(sock_num=0; sock_num<k; sock_num++){
        			if(FD_ISSET(csock[sock_num],&rfds)) {
                state = 4;
                break;
        			}
            }
          }
    		}
        break;

        case 2:
        /* 参加受付 */
        clen = sizeof(clt);
        if ((csock[k] = accept(sock, (struct sockaddr *) &clt, &clen)) < 0) {
            perror("accept");
            exit(2);
        } else {
          k++;
          if(k<=5){
            int strlength = strlen("REQUEST ACCEPTED\n");
            write(csock[k-1], "REQUEST ACCEPTED\n", sizeof(char)*strlength);
            write(1, "REQUEST ACCEPTED\n", sizeof(char)*strlength);
            state = 3;
          } else {
            int strlength = strlen("REQUEST REJECTED\n");
            write(csock[k-1], "REQUEST REJECTED\n", sizeof(char)*strlength);
            write(1, "REQUEST REJECTED\n", sizeof(char)*strlength);
            close(csock[k-1]);
            csock[k-1] = -1;
            k--;
            state = 1;
          }
        }
        break;

        case 3:
        /* ユーザー名登録 */
        if ((nbytes = read(csock[k-1], rbuf, sizeof(rbuf))) < 0) {
           perror("read");
        } else {
          int flag = 0;
          char *p = rbuf;
          int buf_count = 0;
          while (*p != '\n') {
            buf_count++;
            p++;
          }
          for(i=0; i<k-1; i++){
            if(strncmp(user[i], rbuf, buf_count) == 0){
              strlength = strlen("USERNAME REJECTED\n");
              write(csock[k-1], "USERNAME REJECTED\n", sizeof(char)*strlength);
              write(1, "USERNAME REJECTED\n", sizeof(char)*strlength);
              close(csock[k-1]);
              csock[k-1] = -1;
              k--;
              flag  = 1;
              break;
            }
          }
          if(!flag){
            strlength = strlen("USERNAME REGISTERED\n");
            write(csock[k-1], "USERNAME REGISTERED\n", sizeof(char)*strlength);
            write(1, "USERNAME REGISTERED\n", sizeof(char)*strlength);
            strncpy(user[k-1], rbuf, buf_count);
            printf("Join UserName : %s\n",user[k-1]);
            for(int j=0; j<k-1; j++){
              write(csock[j], "Join ", sizeof("Join "));
              write(csock[j], user[k-1], sizeof(user[k-1]));
              write(csock[j], "\n", sizeof("\n"));
            }
          }
          state = 1;
        }
        break;

        case 4:
        /* メッセージ配信 */
        if ((nbytes = read(csock[sock_num], rbuf, sizeof(rbuf))) < 0) {
           perror("read");
        } else if(nbytes == 0){
          state = 5;
          break;
        } else {
          if(strncmp(rbuf, "/list\n", 6) == 0){
            write(csock[sock_num], "User List\n", sizeof("User List\n"));
            for(int j=0; j<k; j++){
              write(csock[sock_num], user[j], sizeof(user[j]));
              write(csock[sock_num], "\n", sizeof("\n"));
            }
          } else {
            for(int j=0; j<k; j++){
              if(j != sock_num){
                write(csock[j], user[sock_num], sizeof(user[sock_num]));
                write(csock[j], " > ", sizeof(" > "));
                write(csock[j], rbuf, nbytes);
              }
            }
          }
        }
        state = 1;
        break;

        case 5:
        /* 離脱処理 */
        printf("Escapsed User:%s\n", user[sock_num]);
        for(int j=0; j<k; j++){
          if(j != sock_num){
            write(csock[j], "Escapsed ", sizeof("Escapsed "));
            write(csock[j], user[sock_num], sizeof(user[sock_num]));
            write(csock[j], "\n", sizeof("\n"));
          }
        }
        close(csock[sock_num]);
        strcpy(user[sock_num], user[k-1]);
        memset(user[k-1], 0, 1024);
        csock[sock_num] = csock[k-1];
        csock[k-1] = -1;
        k--;
        state = 1;
        break;
      }
    }
  }
