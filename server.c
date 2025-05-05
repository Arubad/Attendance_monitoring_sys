#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>

#define SERVER_PORT 9090
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_ROLLS 200

typedef struct {
    char roll_no[10];
    char timestamp[20];
    bool present;
} AttendanceRecord;

AttendanceRecord records[MAX_ROLLS];
char valid_rolls[MAX_ROLLS][10];
int num_valid_rolls = 0;
int record_count = 0;
int attendance_duration_seconds = 3600;
time_t login_time;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void load_valid_rolls(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening valid roll numbers file");
        exit(EXIT_FAILURE);
    }
    char line[20];
    while (fgets(line, sizeof(line), file) && num_valid_rolls < MAX_ROLLS) {
        line[strcspn(line, "\n")] = 0;
        strcpy(valid_rolls[num_valid_rolls++], line);
    }
    fclose(file);
    printf("Loaded %d valid roll numbers.\n", num_valid_rolls);
}

int find_roll_index(const char* roll) {
    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].roll_no, roll) == 0)
            return i;
    }
    return -1;
}

bool is_valid_roll(const char *roll) {
    for (int i = 0; i < num_valid_rolls; i++) {
        if (strcmp(roll, valid_rolls[i]) == 0) return true;
    }
    return false;
}

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    char buffer[BUFFER_SIZE];

    memset(buffer, 0, sizeof(buffer));
    recv(client_socket, buffer, sizeof(buffer), 0);

    char roll_no[10];
    sscanf(buffer, "%s", roll_no);

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", local);

    double diff = difftime(now, login_time);
    printf("Client [%s] connected at %s | Time since login: %.2f seconds\n", roll_no, timestamp, diff);

    pthread_mutex_lock(&lock);
    if (diff >= attendance_duration_seconds) {
        send(client_socket, "Attendance period is over.", 27, 0);
    } else if (!is_valid_roll(roll_no)) {
        send(client_socket, "Invalid Roll Number.", 21, 0);
    } else if (find_roll_index(roll_no) != -1) {
        send(client_socket, "Attendance already marked.", 27, 0);
    } else {
        strcpy(records[record_count].roll_no, roll_no);
        strcpy(records[record_count].timestamp, timestamp);
        records[record_count].present = true;
        record_count++;

        char confirmation_msg[BUFFER_SIZE];
        snprintf(confirmation_msg, sizeof(confirmation_msg), "Roll no: %s, Attendance submitted at %s", roll_no, timestamp);
        send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);
    }
    pthread_mutex_unlock(&lock);
    close(client_socket);
    return NULL;
}

void save_attendance_csv() {
    const char *filename = "attendance.csv";
    FILE *file = fopen(filename, "a");

    if (!file) {
        fprintf(stderr, "Error: Could not open %s for appending.\n", filename);
        perror("Reason");
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    if (size == 0) {
        fprintf(file, "Roll No,Date,Present,Timestamp\n");
    }

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char date[11];
    strftime(date, sizeof(date), "%Y-%m-%d", local);

    for (int i = 0; i < num_valid_rolls; i++) {
        const char *roll = valid_rolls[i];
        int index = find_roll_index(roll);
        if (index != -1) {
            fprintf(file, "%s,%s,1,%s\n", roll, date, records[index].timestamp);
        } else {
            fprintf(file, "%s,%s,0,—\n", roll, date);
        }
    }

    fclose(file);
    printf("Attendance saved to %s\n", filename);
}

int main() {
    int server_fd, *client_socket, addr_len;
    struct sockaddr_in server_addr, client_addr;

    int duration_minutes;
    printf("Enter attendance window duration (in minutes): ");
    scanf("%d", &duration_minutes);
    attendance_duration_seconds = duration_minutes * 60;
    getchar();

    load_valid_rolls("valid_rolls.txt");
    time(&login_time);
    printf("Login time recorded. Attendance open for %d minutes.\n", duration_minutes);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    addr_len = sizeof(client_addr);
    printf("Server started on port %d. Waiting for students...\n", SERVER_PORT);

    time_t now;
    while (1) {
        time(&now);
        double diff = difftime(now, login_time);
        if (diff >= attendance_duration_seconds) {
            printf("Attendance window expired. Closing server.\n");
            break;
        }

        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            client_socket = malloc(sizeof(int));
            *client_socket = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, client_socket);
            pthread_detach(tid);
        }
    }

    save_attendance_csv();
    close(server_fd);
    return 0;
}
