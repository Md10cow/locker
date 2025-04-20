#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>

#define ERROR_MISSING_ARG        "Missing arg: file name"
#define ERROR_MALLOC_FAILED      "Error while allocating buffer with 'malloc'"
#define ERROR_LOCK_FILE_MISSING  "Not found lock file for pid: %d"
#define ERROR_PID_MISMATCH       "Error in compare pid in lock file: Expected pid: %d, actual pid: %d"
#define ERROR_UNLOCK_FAILED      "Error unlock lock file"
#define STATS_LOCK_SUCCESS       "[%d] Successful locks = %d\n"
#define LOCK_FILE_SUFFIX         ".lck"
#define STATS_FILE_NAME          "statistics"

static int g_successful_locks = 0;
int g_current_pid;

void HandleSignalInterrupt(int signal_number)
{
    int stats_fd = open(STATS_FILE_NAME, O_WRONLY | O_APPEND | O_CREAT, 0666);
    char stats_buffer[200];
    snprintf(stats_buffer, sizeof(stats_buffer), STATS_LOCK_SUCCESS, g_current_pid, g_successful_locks);
    write(stats_fd, stats_buffer, strlen(stats_buffer));
    close(stats_fd);
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, HandleSignalInterrupt);
    if (optind >= argc) {
        fprintf(stderr, ERROR_MISSING_ARG);
        return EXIT_FAILURE;
    }
    char *target_file_path = argv[optind++];
    size_t lock_file_path_length = strlen(target_file_path) + strlen(LOCK_FILE_SUFFIX) + 1;
    char *lock_file_path = malloc(lock_file_path_length);
    if (!lock_file_path) {
        fprintf(stderr, ERROR_MALLOC_FAILED);
        exit(EXIT_FAILURE);
    }
    snprintf(lock_file_path, lock_file_path_length, "%s%s", target_file_path, LOCK_FILE_SUFFIX);

    g_current_pid = getpid();

    while (1) {
        int lock_file_descriptor;
        while ((lock_file_descriptor = open(lock_file_path, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1) {

        }
        g_successful_locks++;
        char pid_string_buffer[20];
        int pid_string_length = snprintf(pid_string_buffer, sizeof(pid_string_buffer), "%d", g_current_pid);
        write(lock_file_descriptor, pid_string_buffer, pid_string_length);
        int target_file_descriptor = open(target_file_path, O_RDWR);
        sleep(1);
        close(target_file_descriptor);
        if (access(lock_file_path, F_OK) == -1) {
            fprintf(stderr, ERROR_LOCK_FILE_MISSING, g_current_pid);
            exit(EXIT_FAILURE);
        }
        lseek(lock_file_descriptor, 0, SEEK_SET);
        char read_pid_buffer[20];
        read(lock_file_descriptor, read_pid_buffer, sizeof(read_pid_buffer));
        if (atoi(read_pid_buffer) != g_current_pid) {
            fprintf(stderr, ERROR_PID_MISMATCH, g_current_pid, atoi(read_pid_buffer));
            close(lock_file_descriptor);
            free(lock_file_path);
            return EXIT_FAILURE;
        }
        close(lock_file_descriptor);
        if (unlink(lock_file_path) == -1) {
            perror(ERROR_UNLOCK_FAILED);
            free(lock_file_path);
            return EXIT_FAILURE;
        }
    }

    free(lock_file_path);
    return EXIT_SUCCESS;
}
