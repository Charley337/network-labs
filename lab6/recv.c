#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535
#define MAX_SEND 4095

#define MYSEND_NORMAL 0
#define MYSEND_ENCODE_STR 1
#define MYSEND_ENCODE_FILE 2

char buf[MAX_SIZE+1];

void recv_display(int s_fd)
{
    int r_size;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0';
    printf("%s\n", buf);
}

void send_data(int s_fd, const char *msg, char *val)
{
    static char sendbuf[MAX_SEND+1];
    char *buf = sendbuf;
    int len_data = strlen(msg);
    if (val != NULL)
        len_data += strlen(val);
    if (len_data >= MAX_SEND)
        buf = (char*)malloc(sizeof(char) * (len_data + 1));
    if (val == NULL)
        sprintf(buf, msg);
    else
        sprintf(buf, msg, val);
    send(s_fd, buf, strlen(buf), 0);
    if (len_data >= MAX_SEND)
        free(buf);
}

void mysend(int s_fd, const char *msg, char *val, int opt)
{
    switch (opt)
    {
    case MYSEND_NORMAL:
        send_data(s_fd, msg, val);
        break;

    default:
        break;
    }
}

void recv_mail()
{
    const char* host_name = "pop.xxx.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "xxx@xxx.com"; // TODO: Specify the user
    const char* pass = "xxx"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    struct sockaddr_in addr;
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dest_ip);
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(s_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) != 0) {
        herror("connect");
        exit(EXIT_FAILURE);
    }

    // Print welcome message
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);

    // TODO: Send user and password and print server response
    mysend(s_fd, "user %s\r\n", (char*)user, MYSEND_NORMAL);
    recv_display(s_fd);
    mysend(s_fd, "pass %s\r\n", (char*)pass, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send STAT command and print server response
    mysend(s_fd, "stat\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send LIST command and print server response
    mysend(s_fd, "list\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Retrieve the first mail and print its content
    mysend(s_fd, "retr 1\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send QUIT command and print server response
    mysend(s_fd, "quit\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
