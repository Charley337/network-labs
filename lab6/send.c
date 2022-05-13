#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095
#define MAX_SEND 65535

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
    char *crypt;
    int file_len = 0;
    uint8_t *file_buf;
    FILE *fp;
    FILE *tmp;
    switch (opt)
    {
    case MYSEND_NORMAL:
        send_data(s_fd, msg, val);
        break;
    
    case MYSEND_ENCODE_STR:
        crypt = encode_str(val);
        crypt[strlen(crypt)-1] = '\0';
        send_data(s_fd, msg, crypt);
        free(crypt);
        break;
    
    case MYSEND_ENCODE_FILE:
        fp = fopen(val, "rb");
        if (fp == NULL) {
            perror("Fail to open files");
            exit(EXIT_FAILURE);
        }
        tmp = tmpfile();
        if (tmp == NULL)
        {
            perror("Fail to open tmpfile");
            exit(EXIT_FAILURE);
        }
        encode_file(fp, tmp);
        fclose(fp);
        file_len = ftell(tmp);
        rewind(tmp);
        file_buf = (uint8_t*)malloc(sizeof(uint8_t) * (file_len + 1));
        fread(file_buf, sizeof(uint8_t), file_len, tmp);
        fclose(tmp);
        send_data(s_fd, msg, file_buf);
        free(file_buf);
        break;

    default:
        break;
    }
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.xxx.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "xxx@xxx.com"; // TODO: Specify the user
    const char* pass = "xxx"; // TODO: Specify the password
    const char* from = "xxx@xxx.com"; // TODO: Specify the mail address of the sender
    char dest_ip[16]; // Mail server IP address
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

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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

    // Send EHLO command and print server response
    mysend(s_fd, "EHLO 163.com\r\n", NULL, MYSEND_NORMAL);
    // TODO: Print server response to EHLO command
    recv_display(s_fd);
    // TODO: Authentication. Server response should be printed out.
    mysend(s_fd, "AUTH login\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);
    mysend(s_fd, "%s\r\n", (char*)from, MYSEND_ENCODE_STR);
    recv_display(s_fd);
    mysend(s_fd, "%s\r\n", (char*)pass, MYSEND_ENCODE_STR);
    recv_display(s_fd);
    // TODO: Send MAIL FROM command and print server response
    mysend(s_fd, "MAIL FROM:<%s>\r\n", (char*)from, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send RCPT TO command and print server response
    mysend(s_fd, "RCPT TO:<%s>\r\n", (char*)receiver, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send DATA command and print server response
    mysend(s_fd, "data\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send message data
    // 发件人（可伪造）
    mysend(s_fd, "From: %s\r\n", (char*)user, MYSEND_NORMAL);
    // 收件人
    mysend(s_fd, "To: %s\r\n", (char*)receiver, MYSEND_NORMAL);
    // MIME头部
    mysend(s_fd, "MIME-Version: 1.0\r\n", NULL, MYSEND_NORMAL);
    mysend(s_fd, "Message-ID:<>\r\n", NULL, MYSEND_NORMAL);
    mysend(s_fd, "Content-Type: multipart/mixed; boundary=qwertyuiopasdfghjklzxcvbnm\r\n", NULL, MYSEND_NORMAL);
    // 主题
    mysend(s_fd, "Subject: =?utf8?B?%s?=\r\n", (char*)subject, MYSEND_ENCODE_STR);
    mysend(s_fd, "\r\n--qwertyuiopasdfghjklzxcvbnm\r\n", NULL, MYSEND_NORMAL);
    mysend(s_fd, "Content-Type: text/html\r\n", NULL, MYSEND_NORMAL);
    mysend(s_fd, "Content-Transfer-Encoding: base64\r\n", NULL, MYSEND_NORMAL);
    // 邮件内容
    mysend(s_fd, "\r\n%s\r\n", (char*)msg, MYSEND_ENCODE_FILE);
    mysend(s_fd, "\r\n--qwertyuiopasdfghjklzxcvbnm\r\n", NULL, MYSEND_NORMAL);
    // 附件名字
    mysend(s_fd, "Content-Type: application/octet-stream; name=\"=?utf8?B?%s?=\"\r\n", (char*)att_path, MYSEND_ENCODE_STR);
    mysend(s_fd, "Content-Transfer-Encoding: base64\r\n", NULL, MYSEND_NORMAL);
    // 邮件附件
    mysend(s_fd, "\r\n%s\r\n", (char*)att_path, MYSEND_ENCODE_FILE);
    mysend(s_fd, "\r\n--qwertyuiopasdfghjklzxcvbnm\r\n", NULL, MYSEND_NORMAL);
    // TODO: Message ends with a single period
    mysend(s_fd, "%s", (char*)end_msg, MYSEND_NORMAL);
    recv_display(s_fd);
    // TODO: Send QUIT command and print server response
    mysend(s_fd, "quit\r\n", NULL, MYSEND_NORMAL);
    recv_display(s_fd);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
