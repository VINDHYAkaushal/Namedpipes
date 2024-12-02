#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>

#define MAX_LEN 1000

// Function to read messages from the server
void *read_server(void *ptr) {
    FILE *client_fifo = ptr;
    char line[MAX_LEN];
    while (1) {
        if (!fgets(line, MAX_LEN, client_fifo)) {
            break;
        }
        printf("%s", line);
    }
}

// Function to write messages to the server
void *write_server(void *ptr) {
    FILE *server_fifo = ptr;
    while (1) {
        char response[MAX_LEN];
        fgets(response, MAX_LEN, stdin);
        fprintf(server_fifo, "%s", response);
        fflush(server_fifo);
    }
}

int main(int argc, char *argv[]) {
    // Check if correct number of arguments is provided
    if (argc != 3) {
        puts("Usage: imclient <server-fifo-name> username");
        exit(1);
    }

    // Create client FIFO
    char client_fifo[MAX_LEN];
    sprintf(client_fifo, "/tmp/%s-%d", getenv("USER"), getpid());
    mkfifo(client_fifo, 0600);
    chmod(client_fifo, 0622);

    // Open server FIFO for writing, and send client FIFO details
    FILE *server_fifo = fopen(argv[1], "w");
    fprintf(server_fifo, "%s %s\n", client_fifo, argv[2]);
    fclose(server_fifo);

    // Open client FIFO for reading
    FILE *client_fifo_fp = fopen(client_fifo, "r");

     // Read server FIFO details from client FIFO
    char server_fifo_name[MAX_LEN];
    fscanf(client_fifo_fp, "%s", server_fifo_name);
    char line[MAX_LEN];
    fgets(line, MAX_LEN, client_fifo_fp);

    // Open server FIFO for writing
    FILE *server_fifo_fp = fopen(server_fifo_name, "w");

    // Create threads for reading and writing messages
    pthread_t read_thread;
    pthread_t write_thread;

    pthread_create(&read_thread, NULL, read_server, (void *)client_fifo_fp);
    pthread_create(&write_thread, NULL, write_server, (void *)server_fifo_fp);

    // Wait for threads to finish (infinite loop)
    while (1) {}

    // Close client FIFO and unlink it
    fclose(client_fifo_fp);
    unlink(client_fifo);

    return 0;
}

           
