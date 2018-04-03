#include "espressif/esp_common.h"
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "my_http.h"
#include "configs.h"

static QueueHandle_t publish_queue;

/*
static void http_processing(uint32_t argc, char *argv[]);
static void wifi_configuring(uint32_t argc, char *argv[])
static void cmd_help(uint32_t argc, char *argv[]);
static void handle_command(char *cmd);
static void terminal(void);
*/
static void wifi_connect(const char * ssid, const char * pwd);

static void http_processing(uint32_t argc, char *argv[])
{
	if(argc < 3){
		printf("Error: missing arguments\n");
		return;
	}
	if(	strcmp(argv[1],"POST") != 0 && 
		strcmp(argv[1],"GET") != 0
		)
	{
		printf("Error: wrong type\n");
		return;
	}

	char * request = 0;
	char * host = init_request(&request,argv[2], argv[3]?argv[3]:0);

	const struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
	};
	struct addrinfo *res;
	printf("Running DNS lookup for %s...\n",host);
	int err = getaddrinfo(host,"80",&hints,&res);
	if(err !=0 || res == NULL){
		printf("DNS lookup failed err=%d res=%p\n",err,res);
		if(res){
			freeaddrinfo(res);
			}
		goto error;
	} 

	struct sockaddr *sa = res->ai_addr;
	if(sa->sa_family == AF_INET){
		printf(
			"DNS lookup succeeded. IP=%s\n",
			inet_ntoa(((struct sockaddr_in *)sa)->sin_addr)
			); 
	}
	int s = socket(res->ai_family,res->ai_socktype,0);
	if(s < 0){
		printf("... Failed  to allocate socket.\n");
		freeaddrinfo(res);
		goto error;
	}
	printf("...allocated socket\n");
	if(connect(s,res->ai_addr,res->ai_addrlen)!=0){
		close(s);
		freeaddrinfo(res);
		printf("... socket connect failed\n");
		goto error;
	}
	printf("...conneced\n");
	freeaddrinfo(res);

	if(write(s,request,strlen(request))<0){
		printf("...socket send failed \n");
		close(s);
		goto error;
	}
	printf("...socket send success\n");

	char recv_buf[INPUT_BUFF_SIZE];
	int r;
	do{
		bzero(recv_buf,128);
		r = read(s,recv_buf,127);
		if(r > 0)
		{
			printf("%s",recv_buf);
		}
	} while(r >0);

	printf("...done reading from socket. Last read return=%d errno=%d\n",r,errno);

error:
	close(s);
	free_request(&request);
}

static void wifi_configuring(uint32_t argc, char *argv[])
{
    if (argc >= 2) {
	if(argc>=3)
        wifi_connect(argv[1],argv[2]);
	else
	wifi_connect(argv[1],"");
    } else {
        printf("Error: missing ssid. Type 'help'\n");
    }
}

static void cmd_help(uint32_t argc, char *argv[])
{
    printf("wifi <wifi_ssid> [wifi_password] establishing new connection\n");
    printf("http <REQ> <URL> [BODY] http request\n");
    printf("help show help\n");
    printf("\nExample:\n");
    printf("  wifi Your_Network your_pwd\n");
    printf("  http GET google.com\n");
    printf("  http POST yoursite.com data1=123&data2=hello\n");
}

static void handle_command(char *cmd)
{
    char *argv[MAX_ARGC];
    int argc = 1;
    char *temp, *rover;
    memset((void*) argv, 0, sizeof(argv));
    argv[0] = cmd;
    rover = cmd;
    while(argc < MAX_ARGC && (temp = strstr(rover, " "))) {
        rover = &(temp[1]);
        argv[argc++] = rover;
        *temp = 0;
    }
    if (strlen(argv[0]) > 0) {
        if (strcmp(argv[0], "help") == 0) cmd_help(argc, argv);
        else if (strcmp(argv[0], "http") == 0) http_processing(argc, argv);
        else if (strcmp(argv[0], "wifi") == 0) wifi_configuring(argc, argv);
        else printf("Unknown command %s, try 'help'\n", argv[0]);
    }
}

static void terminal()
{
    char ch;
    char cmd[CMD_SIZE];
    int i = 0;
    printf("\n\n\nWelcome to serial_http_request. Type 'help<enter>' for, well, help\n");
    printf("%% ");
    fflush(stdout);
    while(1) {
        if (read(0, (void*)&ch, 1)) {
            printf("%c",ch);
		if('\b' == ch){
	    		cmd[i] = 0;
			if(i>0)
		   		--i;
			printf(" ");
			printf("%c",ch);
			fflush(stdout);
			continue;
	    	}
            fflush(stdout);
            if ('\n' == ch || '\r' == ch) {
                cmd[i] = 0;
                i = 0;
                printf("\n");
                handle_command((char*) cmd);
                printf("%% ");
                fflush(stdout);
            } else {
                if (i < sizeof(cmd)) cmd[i++] = ch;
            }
        } else {
            printf("You will never see this print as read(...) is blocking\n");
        }
    }
}

static void wifi_connect(const char * ssid, const char * pwd)
{
	sdk_wifi_station_disconnect();
	uint8_t retries = CONNECTION_RETRIES;
	uint8_t status = 0;
	struct sdk_station_config config;

	strcpy(&config.ssid[0],ssid);
	strcpy(&config.password[0],pwd);
	sdk_wifi_set_opmode(STATION_MODE);
	sdk_wifi_station_set_config(&config);
	sdk_wifi_station_connect();
	while(retries)
	{
		status = sdk_wifi_station_get_connect_status();
		if(STATION_WRONG_PASSWORD == status){
		printf("Wrong password\n");
		break;
		} else if (STATION_NO_AP_FOUND == status){
		printf("No access point found\n");
		break;
		} else if (STATION_CONNECT_FAIL == status){
		printf("Connection failed\n");
		break;
		} else if(STATION_GOT_IP == status){
		printf("Connected to access point\n");
		break;
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
		--retries;
	}
	if(0 == retries || status != STATION_GOT_IP)
	{
		printf("WiFi error connection\n");
		sdk_wifi_station_disconnect();
	}
}

void user_init(void)
{
    	uart_set_baud(0, 115200);
	sdk_wifi_station_set_auto_connect(false);
	publish_queue = xQueueCreate(1,16);
	xTaskCreate(&terminal, "terminal",2048,NULL,2,NULL);
}
