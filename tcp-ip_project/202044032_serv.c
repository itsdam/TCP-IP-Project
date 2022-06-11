#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include<time.h>

#define BUF_SIZE 128
#define MAX_CLNT 1
#define NAME_SIZE 20

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);
char* serverState(int count);
void menu(char port[]);

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
char clnt_name[NAME_SIZE]= {NULL};
char clnt_names[MAX_CLNT][NAME_SIZE]= {NULL};

pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock, i;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;

	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	menu(argv[1]);

	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);

		if(clnt_cnt >= MAX_CLNT) {
			printf("[ERROR] Connect Error : %d \n", clnt_sock);
			write(clnt_sock, "[ERROR] Capacity is full", BUF_SIZE);
			continue;
		}


		pthread_mutex_lock(&mutx);

		clnt_socks[clnt_cnt]=clnt_sock;
		read(clnt_sock, clnt_name, NAME_SIZE);
		strcpy(clnt_names[clnt_cnt++], clnt_name);
		pthread_mutex_unlock(&mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}
	close(serv_sock);
	return 0;
}

void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0, i;
	int fSize = 0;
	const char sig_file[BUF_SIZE] = {"file : cl->sr"};
	const char Fmsg_end[BUF_SIZE] = {"FileEnd_cl->sr"};
	const char sig_file_all[BUF_SIZE] = {"file : cl->sr_all"};
	const char sig_whisper[BUF_SIZE] = {"whisper : cl->sr"};
	char msg[BUF_SIZE] = {NULL};
	char file_msg[BUF_SIZE] = {NULL};

	while((str_len=read(clnt_sock, msg, BUF_SIZE))!=0)
	{

		if(!strcmp(msg, sig_file))
		{
			int j;
			int noCli = 0;
			int fileGo = NULL;
			char tmpName[NAME_SIZE]= {NULL};

			read(clnt_sock, tmpName, NAME_SIZE);

			pthread_mutex_lock(&mutx);

			for(j=0; j<clnt_cnt; j++) {


				if(!strcmp(tmpName, clnt_names[j]) ) {
					noCli = 0;
					fileGo = j;
					break;
				}
				else if(j == clnt_cnt - 1) {
					noCli = 1;
					break;
				}
			}

			if(noCli == 1) {

				write(clnt_sock, "[NoClient_sorry]", BUF_SIZE);
				pthread_mutex_unlock(&mutx);
				continue;
			}
			else if(noCli == 0) {

				write(clnt_sock, "[continue_ok_nowgo]", BUF_SIZE);

			}

			write(clnt_socks[fileGo], "file : sr->cl", BUF_SIZE);

			read(clnt_sock, &fSize, sizeof(int));
			printf("File size %d Byte\n", fSize);
			write(clnt_socks[fileGo], &fSize, sizeof(int));

			while(1) {
				read(clnt_sock, file_msg, BUF_SIZE);
				if(!strcmp(file_msg, Fmsg_end))
					break;
				write(clnt_socks[fileGo], file_msg, BUF_SIZE);
			}


			write(clnt_socks[fileGo], "FileEnd_sr->cl", BUF_SIZE);

			pthread_mutex_unlock(&mutx);
			printf("[Notice] File data transfered \n");

		}
		else if(!strcmp(msg, sig_file_all)) {

			pthread_mutex_lock(&mutx);


			for(i=0; i<clnt_cnt; i++) {
				if(clnt_sock == clnt_socks[i])
					continue;
				write(clnt_socks[i], "file : sr->cl", BUF_SIZE);
			}

			read(clnt_sock, &fSize, sizeof(int));
			printf("File size %d Byte\n", fSize);

			for(i=0; i<clnt_cnt; i++) {
				if(clnt_sock == clnt_socks[i])
					continue;
				write(clnt_socks[i], &fSize, sizeof(int));
			}

			while(1) {
				read(clnt_sock, file_msg, BUF_SIZE);
				if(!strcmp(file_msg, Fmsg_end))
					break;

				for(i=0; i<clnt_cnt; i++) {
					if(clnt_sock == clnt_socks[i])
						continue;
					write(clnt_socks[i], file_msg, BUF_SIZE);
				}
			}

			for(i=0; i<clnt_cnt; i++) {
				if(clnt_sock == clnt_socks[i])
					continue;
				write(clnt_socks[i], "FileEnd_sr->cl", BUF_SIZE);
			}


			pthread_mutex_unlock(&mutx);
			printf("[Notice] File data transfered \n");
		}
		else if(!strcmp(msg, sig_whisper)) {
			int j=0;
			int noCli = 0;
			int mGo = 0;
			char tmpName[NAME_SIZE]= {NULL};
			char msg[NAME_SIZE]= {NULL};

			read(clnt_sock, tmpName, NAME_SIZE);

			pthread_mutex_lock(&mutx);
			for(j=0; j<clnt_cnt; j++) {
				if(!strcmp(tmpName, clnt_names[j]) ) {
					noCli = 0;
					mGo = j;
					break;
				}
				else if(j == clnt_cnt - 1) {
					noCli = 1;
					break;
				}
			}
			pthread_mutex_unlock(&mutx);

			read(clnt_sock, msg, BUF_SIZE);

			if(noCli == 1) {
				write(clnt_sock, "[ERROR] client error", BUF_SIZE);
			}
			else {
				write(clnt_socks[j], msg, BUF_SIZE);
			}

		}
		else
		{
			printf("[Notice] Chatting message transfered \n");
			send_msg(msg, str_len);
		}
	}


	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
	{
		if(clnt_sock==clnt_socks[i])
		{
			while(i++<clnt_cnt-1) {
				clnt_socks[i]=clnt_socks[i+1];
				strcpy(clnt_names[i], clnt_names[i+1]);
			}
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	return NULL;
}
void send_msg(char * msg, int len)
{
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
		write(clnt_socks[i], msg, BUF_SIZE);
	pthread_mutex_unlock(&mutx);
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

char* serverState(int count)
{
    char* stateMsg = malloc(sizeof(char) * 20);
    strcpy(stateMsg ,"None");

    if (count < 5)
        strcpy(stateMsg, "Good");
    else
        strcpy(stateMsg, "Bad");

    return stateMsg;
}

void menu(char port[])
{
    system("clear");
    printf(" **** Chatting service server ****\n");
    printf(" server port    : %s\n", port);
    printf(" server state   : %s\n", serverState(clnt_cnt));
    printf(" max connection : %d\n", MAX_CLNT);
    printf(" ****            Log          ****\n\n");
}

