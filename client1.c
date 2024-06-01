#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 50001

int file_client(const char *server_ip);
char *ensure_end_with_slash(char *savepath);
char *set_file_path(int sockfd, char *filename, char *savepath);
int recvfile(int sockfd, char *fullname);
void print_progressbar(double percentage, int speed);

int file_client(const char *server_ip) {
  // Initialize socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("[-]Error in creating socket.\n");
    exit(EXIT_FAILURE);
  }
  printf("[+]Socket is created.\n");

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // Switch the target server ip address from string to binary
  if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
    perror("[-]inet_pton failed to convert ip address.\n");
    exit(EXIT_FAILURE);
  }

  int connect_result =
      connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connect_result < 0) {
    perror("[-]Failed to connect to server. Exiting...\n");
    exit(EXIT_FAILURE);
  }
  printf("[+]Connected to server.\n");

  return sockfd;
}

char *ensure_end_with_slash(char *savepath) {
  size_t len = strlen(savepath);
  char *newpath = (char *)malloc(len + 2);
  strcpy(newpath, savepath);

  if (len == 0 || newpath[len - 1] != '/') {
    newpath[len] = '/';
    newpath[len + 1] = '\0';
  }

  return newpath;
}

char *set_file_path(int sockfd, char *filename, char *savepath) {
  if (send(sockfd, filename, strlen(filename), 0) < 0) {
    perror("[-]Failed to send filename to server.\n");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  printf("[+]Requested file %s\n", filename);

  char fullpath[BUFFER_SIZE] = {0};
  snprintf(fullpath, sizeof(fullpath), "%s%s", savepath, filename);
  printf("[+]The file will be saved in the path %s\n", fullpath);
  return strdup(fullpath);
}

int recvfile(int sockfd, char *fullname) {
  char buffer[BUFFER_SIZE] = {0};
  FILE *fp;
  clock_t start_time = clock();
  clock_t current_time;
  double elapsed_time;
  int speed;

  // Receiving file size:
  long long int file_size = 0;
  recv(sockfd, (char *)&file_size, sizeof(file_size), 0);
  if (file_size <= 0) {
    printf("[-]Failed in receiving the file size.\n");
    return -1;
  }
  printf("[+]File size: %lld KB.\n", (file_size / 1024));

  // Getting ready to receive file.
  fp = fopen(fullname, "w");
  if (fp != NULL) {
    long long int length = 0;
    long long int total_length = 0;
    // Receiving data
    while ((length = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
      if (fwrite(buffer, sizeof(char), length, fp) != length) {
        perror("[-]Error in writing to file.\n");
        break;
      }
      total_length += length;
      memset(buffer, 0, BUFFER_SIZE);

      // Download speed
      current_time = clock();
      elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
      speed = (int)total_length / elapsed_time;
      double percentage = (double)total_length / file_size;
      print_progressbar(percentage, speed);
    }
    printf("\n");
    if (length < 0) {
      perror("[-]Error in receiving file.\n");
      return -1;
    }
    printf("[+]Total KBytes received: %lld\n", (total_length / 1024));
    fclose(fp);
    return 0;
  }
  fclose(fp);
  return -1;
}

void print_progressbar(double percentage, int speed) {
  int barWidth = 50;
  int barLength = barWidth * percentage;
  printf("\r[+]Downloading... [");
  for (int i = 0; i < barWidth; i++) {
    if (i < barLength)
      printf("=");
    else if (i == barLength)
      printf(">");
    else
      printf(" ");
  }
  printf("] %.2f%% , current speed:%d KB/S", (double)(percentage * 100.0),
         (speed / 1024));
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  // Check num of arguments
  if (argc < 4) {
    fprintf(stderr,
            "[-]Too few arguments. Usage: %s <ip> <filename> <savepath>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  } else if (argc > 4) {
    fprintf(stderr,
            "[-]Too many arguments. Usage: %s <ip> <filename> <savepath>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  char *server_ip = argv[1];
  char *filename = argv[2];
  char *savepath = argv[3];

  // Add slash to the end of savepath if it's not added.
  savepath = ensure_end_with_slash(savepath);

  // Check if savepath is valid
  if (access(savepath, F_OK) != 0 || access(savepath, W_OK) != 0) {
    fprintf(stderr,
            "[-]The path %s is not valid, or you don't have permission to "
            "add a new file to it.\n",
            savepath);
    exit(EXIT_FAILURE);
  }

  int sockfd;
  char *fullname;

  sockfd = file_client(server_ip);
  fullname = set_file_path(sockfd, filename, savepath);
  if (recvfile(sockfd, fullname) == 0) {
    printf("[+]File received successfully.\n");
  } else {
    printf("[-]File received failed.\n");
  }

  close(sockfd);
  return 0;
}
