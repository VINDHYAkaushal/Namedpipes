#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>

#define MAX_USERS 100
#define MAX_LEN 1000

char user_names[MAX_USERS][MAX_LEN]; // Array to store usernames
FILE *user_output_streams[MAX_USERS]; // Output streams for each user
int num_users = 0; // Number of connected users

// Function to handle communication with each client
void *handle_client(void *ptr) {
    int user_index = (int)ptr;

    // Create a private FIFO for communication with the client
    char private_fifo[MAX_LEN];
    sprintf(private_fifo, "/tmp/%s-%d-%d", getenv("USER"), getpid(), user_index);
    mkfifo(private_fifo, 0600);
    chmod(private_fifo, 0622);

    // Send the private FIFO path to the client
    fprintf(user_output_streams[user_index], "%s\n", private_fifo);
    fflush(user_output_streams[user_index]);

    // Open the private FIFO for reading
    FILE *private_fifo_fp = fopen(private_fifo, "r");

    // Send a welcome message to the client
    fprintf(user_output_streams[user_index], "%s\n", "Welcome!");
    fflush(user_output_streams[user_index]);

    // Main communication loop
    char command[MAX_LEN];
    while (1) {
        fgets(command, MAX_LEN, private_fifo_fp);
        command[strlen(command) - 1] = 0;

        // Handle different commands
        if (strcmp(command, "list") == 0) {
            // Send the list of connected users to the client
            char user_list[MAX_LEN] = ""; // Initialize to empty string
            for (int i = 0; i < num_users; i++) {
                strcat(user_list, user_names[i]);
                strcat(user_list, "\n"); // Add newline for each username
            }
            fprintf(user_output_streams[user_index], "%s\n", user_list);
            fflush(user_output_streams[user_index]);
        }
        else if (strncmp(command, "send", 4) == 0) {
            // Extract the recipient username and message
            strtok(command, "  ");
            char *recipient_username = strtok(NULL, "  ");
            char *message = strtok(NULL, "\n");

            // Find the index of the recipient user
            int recipient_index = -1;
            for (int i = 0; i < num_users; i++) {
                if (strcmp(recipient_username, user_names[i]) == 0) {
                    recipient_index = i;
                    break;
                }
            }

            // Send the message to the recipient if found
            if (recipient_index != -1) {
                fprintf(user_output_streams[recipient_index], "%s says: %s\n", user_names[user_index], message);
                fflush(user_output_streams[recipient_index]);
                fprintf(user_output_streams[user_index], "Message sent!\n");
                fflush(user_output_streams[user_index]);
            }
            else {
                // Send error message if recipient not found
                fprintf(user_output_streams[user_index], "%s is not a valid user!\n", recipient_username);
                fflush(user_output_streams[user_index]);
            }
        } else {
            // Send error message for invalid command
            fprintf(user_output_streams[user_index], "%s is not a valid command!\n", command);
            fflush(user_output_streams[user_index]);
        }
    }

    return NULL;
}

int main() {
    char line[MAX_LEN];

    // Create a well-known FIFO for clients to connect
    char server_fifo[MAX_LEN];
    sprintf(server_fifo, "/tmp/%s-%d", getenv("USER"), getpid());
    mkfifo(server_fifo, 0600);
    chmod(server_fifo, 0622);
    printf("Connect to %s to use IM!\n", server_fifo);

    // Main server loop
    while (1) {
        FILE *server_fifo_fp = fopen(server_fifo, "r");
        printf("Opened %s to read...\n", server_fifo);

        // Wait for clients' connection requests
        while (fgets(line, MAX_LEN, server_fifo_fp)) {
            // Extract client's FIFO path and username from the request
            char *client_fifo = strtok(line, " ");
            char *username = strtok(NULL, " ");
            username[strlen(username) - 1] = '\0'; // Correct null termination

            // Store the username and open output stream for the client
            strcpy(user_names[num_users], username);
            printf("%s joined!\n", username);
            user_output_streams[num_users] = fopen(client_fifo, "w");

            // Create a thread to handle communication with the client
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void *)num_users);
            num_users++;
        }

        fclose(server_fifo_fp);
    }

    return 0;
}
