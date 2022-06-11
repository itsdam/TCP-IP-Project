#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include<time.h>

#define BUF_SIZE 128
#define NAME_SIZE 20
#define NOTSET 0
#define EXIST 1
#define NOTEXIST 2

void* send_msg(void* arg);
void* recv_msg(void* arg);
void error_handling(char * msg);

void menu();
void changeName();
void menuOptions();

char name[NAME_SIZE]= {NULL};
char msg[BUF_SIZE] = {NULL};
int cli_exist = NOTSET;
int setFName = 0;
int wOk = 1;
char serv_time[NAME_SIZE];
char serv_port[NAME_SIZE];
char clnt_ip[NAME_SIZE];


//pthread_mutex_t mutx;


int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if(argc!=4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	 }

	//pthread_mutex_init(&mutx, NULL);
	// pthread_mutex_lock(&mutx);
	// pthread_mutex_unlock(&mutx);

	/** local time **/
	struct tm *t;
	time_t timer = time(NULL);
	t=localtime(&timer);
	sprintf(serv_time, "%d-%d-%d %d:%d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour,
	t->tm_min);

	sprintf(name, "%s", argv[3]);
	sprintf(clnt_ip, "%s", argv[1]);
	sprintf(serv_port, "%s", argv[2]);
	sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

	write(sock, name, NAME_SIZE);

	printf("\n\n");
	printf("CONNECTING..... \n [TIP] If you want \"MENU\" -> /menu \n\n");

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);
	return 0;
}

void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);
	int Flength = 0;
	int i=0;
	int fSize = 0;
	int fEnd = 0;
	char name_msg[NAME_SIZE+BUF_SIZE] = {NULL};
	char t_msg[BUF_SIZE] = {NULL};
	char last_msg[BUF_SIZE] = {NULL};
	char t_name_msg[BUF_SIZE] = {NULL};
	char noUse[BUF_SIZE] = {NULL};
	const char enter[BUF_SIZE] = {"\n"};
	const char whisper[BUF_SIZE] = {"/whisper\n"};

	while(1)
	{
		if(wOk == 0) {
			sleep(1);
		}

		fgets(msg, BUF_SIZE, stdin);

		if(!strcmp(msg,"/exit\n"))
		{
			close(sock);
			exit(0);
		}
		else if(!strcmp(msg, "/sendfile\n"))
		{
			char location[BUF_SIZE];
			char who[NAME_SIZE];
			FILE *fp;
			FILE *size;

			printf("[Record] File location : ");
			scanf("%s", location);

			size = fopen(location, "rb");
			if(size == NULL) {
				printf("[Notice] No file \n");
				continue;
			}

			printf("[Record] To who(ID)? : ");
			scanf("%s", who);

			write(sock, "file : cl->sr", BUF_SIZE);

			write(sock, who, NAME_SIZE);

			while(cli_exist == NOTSET) {
				sleep(1);
			}

			if(cli_exist == NOTEXIST) {
				printf("[Notice] No user \n");
				cli_exist = NOTSET;
				continue;
			}

			while(1) {
				fEnd = fread(noUse, 1 , BUF_SIZE, size);
				fSize += fEnd;

				if(fEnd != BUF_SIZE)
					break;
			}
			fclose(size);

			printf("[Notice] File transfer start \n(File Size : %d Byte)\n", fSize);
			write(sock, &fSize, sizeof(int));
			fSize = 0;

			fp = fopen(location, "rb");

			while(1) {

				Flength = fread(t_msg, 1 , BUF_SIZE, fp);

				if(Flength != BUF_SIZE) {
					for(i=0; i<Flength; i++) {
						last_msg[i] = t_msg[i];
					}

					write(sock, last_msg, BUF_SIZE);

					write(sock, "FileEnd_cl->sr", BUF_SIZE);
					break;
				}
				write(sock, t_msg, BUF_SIZE);

			}

			fclose(fp);
			printf("[Notice] File transfer success \n");
			cli_exist = NOTSET;

		}
		else if(!strcmp(msg, "/sendfile all\n")) {

			char location[BUF_SIZE];
			FILE *fp;
			FILE *size;

			printf("[Record] File location : ");
			scanf("%s", location);

			size = fopen(location, "rb");
			if(size == NULL) {
				printf("[Notice] No file \n");
				continue;
			}

			write(sock, "file : cl->sr_all", BUF_SIZE);

			while(1) {
				fEnd = fread(noUse, 1 , BUF_SIZE, size);
				fSize += fEnd;

				if(fEnd != BUF_SIZE)
					break;
			}
			fclose(size);

			printf("[Notice] File transfer start \n(File Size : %d Byte)\n", fSize);
			write(sock, &fSize, sizeof(int));
			fSize = 0;

			fp = fopen(location, "rb");

			while(1) {

				Flength = fread(t_msg, 1 , BUF_SIZE, fp);

				if(Flength != BUF_SIZE) {
					for(i=0; i<Flength; i++) {
						last_msg[i] = t_msg[i];
					}

					write(sock, last_msg, BUF_SIZE);

					write(sock, "FileEnd_cl->sr", BUF_SIZE);
					break;
				}
				write(sock, t_msg, BUF_SIZE);

			}

			fclose(fp);
			printf("[Notice] File transfer success \n");
		}
		else if(!strcmp(msg, "/menu\n")) {

			printf("\n");
			printf("**** Chatting Service Menu Page ****\n");
			printf("1. /menu -> Menu Page \n");
			printf("2. /newName -> Name change \n");
			printf("3. /statusInfo -> My status info \n");
			printf("4. /sendfile -> 1:1 file transfer \n");
			printf("5. /sendfile all -> 1:N file transfer \n");
			printf("6. /exit -> chatting service end \n");
			printf("*************************************\n");

		}
		else if(setFName == 1) {
			if(strcmp(msg, enter)) {
				setFName = 0;
			}
		}
		else if(!strcmp(msg,"/newName\n")){
			char nameTemp[100];
  			printf("\n\tNew name(ID) -> ");
   			scanf("%s", nameTemp);
    			sprintf(name, "%s", nameTemp);
    			printf("\n\tComplete.\n\n");
		}
		else if(!strcmp(msg,"/statusInfo\n")){
    			printf(" **** My Chatting client Infomation ****\n");
    			printf(" Server port : %s \n", serv_port);
    			printf(" Client IP   : %s \n", clnt_ip);
    			printf(" Chat name(ID)   : %s \n", name);
    			printf(" **** Let's chat on manners ****\n");
		} 

		else
		{
			strcpy(t_msg, "\n");
			sprintf(t_name_msg,"[%s] %s", name, t_msg);
			sprintf(name_msg,"[%s] %s", name, msg);

			if(strcmp(name_msg, t_name_msg) != 0)
				write(sock, name_msg, BUF_SIZE);
		}
	}

	return NULL;
}

void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);
	char name_msg[BUF_SIZE] = {NULL};
	char file_msg[BUF_SIZE] = {NULL};
	const char signal[BUF_SIZE] = {"file : sr->cl"};
	const char end_msg[BUF_SIZE] = {"FileEnd_sr->cl"};
	const char nocl_msg[BUF_SIZE] = {"[NoClient_sorry]"};
	const char yescl_msg[BUF_SIZE] = {"[continue_ok_nowgo]"};
	const char noConnect[BUF_SIZE] = {"[ERROR] Capacity is full"};
	int str_len = 0;
	int fSize = 0;

	while(1)
	{
		str_len=read(sock, name_msg, BUF_SIZE);


		if(!strcmp(name_msg, signal)) {

			setFName = 1;
			wOk = 0;

			printf("[Notice] receive request. ");

			read(sock, &fSize, sizeof(int));
			printf("(File size : %d Byte)\n [press Enter key to continue]", fSize);

			printf("[Notice] Set file name : ");

			wOk = 1;
			while(setFName == 1) {
				sleep(1);
			}

			msg[strlen(msg)-1] = '\0';

			FILE *fp;
			fp = fopen(msg, "wb");

			while(1)
			{
				read(sock, file_msg, BUF_SIZE);

				if(!strcmp(file_msg, end_msg))
					break;
				fwrite(file_msg, 1, BUF_SIZE, fp);
			}

			fclose(fp);

			printf("[Notice] File receive finished \n");

		}
		else if(strcmp(name_msg, yescl_msg) == 0) {

			cli_exist = EXIST;

		}
		else if(strcmp(name_msg, nocl_msg) == 0) {

			cli_exist = NOTEXIST;
		}
		else if(!strcmp(name_msg, noConnect)) {
			printf("[ERROR] Capacity is full \n");
			exit(0);
		}
		else {
			fputs(name_msg, stdout);
		}
	}
	return NULL;
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

