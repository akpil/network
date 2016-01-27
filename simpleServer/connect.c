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

#define BUFFER 512
#define SMALLBUFF 100

void requesthandler(int sock, char* request_line);
void send_data(int sock, char* ct, char* filename);
void respons_error(int sock);
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd; //descriptors rturn from socket and accept system calls
     int portno; // port number
     socklen_t clilen;

     char buffer[BUFFER];

     /*sockaddr_in: Structure Containing an Internet Address*/
     struct sockaddr_in serv_addr, cli_addr;

     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     /*Create a new socket
       AF_INET: Address Domain is Internet 
       SOCK_STREAM: Socket Type is STREAM Socket */
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]); //atoi converts from String to Integer
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
     serv_addr.sin_port = htons(portno); //convert from host to network byte order

     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
              error("ERROR on binding");

     listen(sockfd,5); // Listen for socket connections. Backlog queue (connections to wait) is 5

     clilen = sizeof(cli_addr);
     /*accept function: 
       1) Block until a new connection is established
       2) the new socket descriptor will be used for subsequent communication with the newly connected client.
     */
     while(1)
     {
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0)
          error("ERROR on accept");

     bzero(buffer,256);
     n = read(newsockfd,buffer,255); //Read is a block function. It will read at most 255 bytes
     if (n < 0) error("ERROR reading from socket");
        printf("Here is the message: %s\n",buffer);

	char request_line[SMALLBUFF];
	strcpy(request_line, strtok(buffer, "\r\n"));
        requesthandler(newsockfd, request_line);

     }
     close(sockfd);
     close(newsockfd);

     return 0;
}

/* find out the request and call send function*/
void requesthandler(int sock, char* request_line)
{
	int cli_sock = sock;//get cli fd
	int n;

	char method[10];
	char protocol[10];
	char file_name[30];
	char temp[SMALLBUFF];
	char header[BUFFER];
	char* contents;
	memset(header, 0x00, BUFFER);
	size_t file_size;
	

	//check the request is correctly arrived
	if(strstr(request_line, "HTTP/")==NULL)
	{
		respons_error(cli_sock);
		return;
	}
	/*get the information of request such as method, file_name, protocol*/
	strcpy(method, strtok(request_line, " /"));
	strcpy(file_name, strtok(NULL, " /"));
	strcpy(temp, strtok(NULL, " "));
	strcpy(protocol, strtok(temp, " "));
	memset(temp, 0x00, SMALLBUFF);
	
	/*because of this part this code can only response the HTTP protocol*/
	if (strstr(protocol, "HTTP") != NULL)
	{
		strcpy(temp, protocol);
		strcat(temp, " 200 ok\r\nContent-Type:");
	}
	
	//check filename
	if (strstr(file_name, ".html") != NULL)
	{
		strcat(temp, " text/html\r\n");
	}
	else if(strstr(file_name, ".jpg") != NULL|strstr(file_name, ".jpeg") != NULL)
	{
		strcat(temp, " image/jpeg\r\n");
	}
	else if(strstr(file_name, ".gif") != NULL)
	{
		strcat(temp, " image/gif\r\n");
	}
	else if(strstr(file_name, ".pdf") != NULL)
	{
		strcat(temp, " application/pdf\r\n");
	}
	else if(strstr(file_name, ".mp3") != NULL)
	{
		strcat(temp, " audio/mpeg3;audio/x-mpeg-3;video/mpeg;video/x-mpeg;text/xml\r\n");
	}
	else
	{
		respons_error(cli_sock);
		return;
	}
	
	//prepare header information
	strcpy(header, temp);
	strcat(header, "Content-Length: ");
	memset(temp, 0x00, SMALLBUFF);
	
	//open the file
	FILE* file = fopen(file_name, "rb");
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);//get file size
	fseek(file, 0, SEEK_SET);
	sprintf(temp, "%d\r\n\r\n", file_size);
	strcat(header, temp);//end make up response header
	
	//load whole file
	contents = (char*)malloc(sizeof(char)*file_size);
	memset(contents, 0x00, file_size);
	fread(contents,sizeof(char) ,file_size, file);

	n = write(sock, header, strlen(header));
	printf("%s\n\n", header);
	if(n < 0) perror("header send fail");

	n = write(sock, contents, file_size);
	printf("%s\n\n", contents);
	if(n < 0) perror("contents send fail");
		
	close(sock);
	return;
}

/*send error message when request handler find out incorrent request*/
void respons_error(int sock)
{
	char protocol[] = "HTTP/1.1 400 Bad Request\r\n"
	"Content-length:2048\r\n"
	"Content-type:text/html\r\n\r\n"
	"<html><head><title>Network</title></head>"
	"<body><font size =+5><br>error occur check the request!"
	"</font></body></html>";

	write(sock, protocol, strlen(protocol));
	close(sock);
	return;
}
