#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TESTPORT 50001
#define BUFFER_SIZE 1024
#define BACKLOG 10

int file_server() {
  // Initialize the socket.
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("[-]Error in creating socket.\n");
    exit(EXIT_FAILURE);
  }
  printf("[+]Socket is created.\n");

  // Reuse of the ports.
  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("[-]Failed to reuse the port.\n");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port =
      htons(TESTPORT); // Since the sinport is set as zero, the OS will
                       // assign a available port for it automatically.

  // Bind the address to the socket.
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("[-]Error in binding.\n");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  printf("[+]Binding successful.\n");
  printf("[+]Listening on the port: %d\n", ntohs(addr.sin_port));

  if (listen(sockfd, BACKLOG) < 0) {
    perror("[-]Error in listening.\n");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  printf("[+]Listening...\n");

  return sockfd;
}

int GetFileSize(char *filename) {
  FILE *filepointer = NULL;
  filepointer = fopen(filename, "rb");
  if (filepointer != NULL) {
    fseek(filepointer, 0, SEEK_END);
    int size = ftell(filepointer);
    fclose(filepointer);
    return size;
  }
  return 0;
}

int send_file(int connectfd, char *sharefolder, char *userip) {
  // Receiving file name from user.
  char file_name[BUFFER_SIZE] = {0};
  if (recv(connectfd, file_name, BUFFER_SIZE, 0) < 0) {
    printf("[-]Failed in receiving file name from (%s).\n", userip);
    close(connectfd);
    return -1;
  }

  char full_name[BUFFER_SIZE] = {0};
  snprintf(full_name, BUFFER_SIZE, "%s%s", sharefolder, file_name);

  // Getting the file size, then send it to client.
  long long int file_size = GetFileSize(full_name);
  if (file_size <= 0) {
    printf("[-]Failed in getting the size of the file (%s) requested by (%s), "
           "check if the file exists or you own the permission.\n",
           file_name, userip);
    close(connectfd);
    return -1;
  }
  send(connectfd, (char *)&file_size, sizeof(file_size), 0);
  printf("[+]File size sent to (%s).\n", userip);

  // Sending data
  char buffer[BUFFER_SIZE] = {0};
  FILE *fp = fopen(full_name, "r");
  if (fp != NULL) {
    long long int length = 0;
    long long int total_length = 0;

    while ((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) {
      if (send(connectfd, buffer, length, 0) < 0) {
        printf("[-]Error in sending file to (%s).\n", userip);
        fclose(fp);
        close(connectfd);
        return -1;
      }
      memset(buffer, 0, BUFFER_SIZE);
      total_length += length;
    }
    fclose(fp);
    printf("[+]File %s has been sent successfully to (%s).\n", file_name,
           userip);
    printf("[+]Total size of file %s: %lld KB\n", file_name,
           (total_length / 1024));
    close(connectfd);
    return 0;
  }
  printf("[-]File %s requested by (%s)not found.\n", file_name, userip);
  close(connectfd);
  return -1;
}

int main() {

  int sockfd = file_server();
  char *sharefolder = "/home/kali/Public/sharefolder/";

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int connectfd =
        accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
    char userip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &(client_addr.sin_addr), userip, INET_ADDRSTRLEN);
    if (connectfd < 0) {
      printf("[-]Failed in creating a connectfd for (%s).\n", userip);
    }
    printf("[+]Succeeded in creating a connectfd for (%s).\n", userip);
    int exit_code = send_file(connectfd, sharefolder, userip);
  }

  close(sockfd);
  return 0;
}
