#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#define BUFFER 1024
#define SMALLBUFF 128

void error(char *msg)
{
	perror(msg);
	exit(1);
}
void saving(char*ip, char*url, int length);
int main(int argc, char *argv[])
{
	int this_sockfd, cli_sockfd, serv_sockfd; //descriptors rturn from socket and accept system calls
	int portno, serv_portno = 80; // port number
	int length, content_no;
	unsigned long long_temp;
	socklen_t clilen;

	char this_buffer[BUFFER], serv_buffer[BUFFER], temp_buffer[BUFFER], url[SMALLBUFF];
	char *temp, *temp_url, *ip;

	/*sockaddr_in: Structure Containing an Internet Address*/
	struct sockaddr_in this_addr, serv_addr, cli_addr;
	struct hostent *server;

	int n;
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	/*Create a new socket
AF_INET: Address Domain is Internet 
SOCK_STREAM: Socket Type is STREAM Socket */
	this_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (this_sockfd < 0)
		error("ERROR opening socket");

	bzero((char *) &this_addr, sizeof(this_addr));
	portno = atoi(argv[1]); //atoi converts from String to Integer
	this_addr.sin_family = AF_INET;
	this_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
	this_addr.sin_port = htons(portno); //convert from host to network byte order

	if (bind(this_sockfd, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0) //Bind the socket to the server address
		error("ERROR on binding");

	if(listen(this_sockfd,5) < 0) // Listen for socket connections. Backlog queue (connections to wait) is 5
	{
		error("ERROR on listening");
	}
	clilen = sizeof(cli_addr);
	/*accept function: 
	  1) Block until a new connection is established
	  2) the new socket descriptor will be used for subsequent communication with the newly connected client.
	 */
	while(1)
	{
		cli_sockfd = accept(this_sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (cli_sockfd < 0)
			error("ERROR on accept");

		bzero((char*)this_buffer,BUFFER);
		bzero((char*)serv_buffer,BUFFER);
		bzero((char*)temp_buffer,BUFFER);
		bzero((char*)url,BUFFER);
		n = read(cli_sockfd,this_buffer,BUFFER-1); //Read is a block function. It will read at most 255 bytes
		if (n < 0) error("ERROR reading from socket");
		//get the http request and host
		if(this_buffer[0] == '\0') continue;
		strcpy(serv_buffer,this_buffer);
		strcpy(temp_buffer,this_buffer);
		temp = strstr(this_buffer, "http://");
		strcpy(url, strtok(temp, " \r\n"));
		temp = strstr(serv_buffer, "Host: ");
		strtok(temp, " \r\n");
		temp = strtok(NULL, " \r\n");

		server = gethostbyname(temp);
		if(server == NULL)
			error("ERROR, such no host");

		bzero((char*)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_port = htons(serv_portno);
		serv_addr.sin_family = AF_INET;
		bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);

		serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(serv_sockfd<0)
			error("ERROR on opening the serv_socket");
		if(connect(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
			error("ERROR on connecting");
		n=write(serv_sockfd, temp_buffer, strlen(temp_buffer));
		if(n < 0)
		{
			error("ERROR on writing to socket");
		}
		content_no = 1;
		length = BUFFER;
//		while(content_no <= length/BUFFER);
		do
		{
			bzero((char*)serv_buffer, BUFFER);
			n = read(serv_sockfd, serv_buffer, BUFFER-1);
			if(n < 0)
				error("EROR on reading from server socket");
			write(cli_sockfd, serv_buffer, n);
			if(content_no == 1)
			{
				temp = strstr(serv_buffer, "Content-Length: ");
				strtok(temp, " \r\n");
				length = atoi(strtok(NULL, "\r\n"));
			}
			content_no++;
		}while(content_no < 100);
		long_temp = ntohl(cli_addr.sin_addr.s_addr);
		cli_addr.sin_addr.s_addr = long_temp;
		ip = (char*)inet_ntoa(cli_addr.sin_addr);
		saving(ip, url, length);
	}
	close(cli_sockfd);
	close(this_sockfd);
	close(serv_sockfd);

	return 0;
}

void saving(char*ip, char*url, int length)
{
	int fd;
	time_t current_time;
	char buffer[BUFFER], small_buffer[SMALLBUFF];
	char* filename = "proxy_log.txt";

	memset (buffer, 0, BUFFER);
	memset (small_buffer, 0, SMALLBUFF);
	time(&current_time);
	strcpy(buffer, (char*)ctime(&current_time));
	strcat(buffer, " Client: ");
	strcat(buffer, ip);
	strcat(buffer, " URL: ");
	strcat(buffer, url);
	strcat(buffer, " Length: ");
	sprintf(small_buffer, "%d\n", length);
	strcat(buffer, small_buffer);
	
	if((fd = open(filename, O_RDWR )) < 0)
	{
		if((fd = creat(filename, 0777)) <0 )
			error("ERROR file creation. ");
	}
	else
		lseek(fd, (off_t)0, SEEK_END);
		
	write(fd, buffer, strlen(buffer));
	close(fd);
}
