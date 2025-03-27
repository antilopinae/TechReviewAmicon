#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h> // For isdigit

// ********************************* //

// Function to get current date and time string for logging
static char* get_current_datetime_string() {
    time_t timer;
    char* buffer = malloc(26); // Allocate enough space for date time string
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    if (!buffer) {
        perror("malloc in get_current_datetime_string");
        return NULL;
    }

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

// ********************************* //

// Function to calculate CPU usage for a given PID
static int calculate_cpu_usage(pid_t pid, float *cpu_percent) {
    char stat_path[64];
    FILE *stat_file;
    char line[512];
    unsigned long utime, stime, cutime, cstime, starttime;
    unsigned long long uptime_jiffies, btime_jiffies;
    long hertz;

    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

    stat_file = fopen(stat_path, "r");
    if (!stat_file) {
        if (errno == ENOENT) {
            return -1; // Process not found
        } else {
            perror("fopen /proc/[pid]/stat");
            return -2; // Error opening stat file
        }
    }

    if (fgets(line, sizeof(line), stat_file) == NULL) {
        if (ferror(stat_file)) {
            perror("fgets /proc/[pid]/stat");
            fclose(stat_file);
            return -2; // Error reading stat file
        } else {
            // Empty file or unexpected EOF
            fclose(stat_file);
            return -2;
        }
    }
    fclose(stat_file);

    // Parse /proc/[pid]/stat - fields 14, 15, 22 for utime, stime, starttime
    if (sscanf(line, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %lu %lu %*d %*d %*d %*d %llu",
               &utime, &stime, &cutime, &cstime, &starttime) != 5) {
        fprintf(stderr, "sscanf failed to parse /proc/[pid]/stat\n");
        return -2; // Error parsing stat file
    }

    // Get system uptime from /proc/uptime
    FILE *uptime_file = fopen("/proc/uptime", "r");
    if (!uptime_file) {
        perror("fopen /proc/uptime");
        return -2; // Error opening uptime file
    }
    double uptime_seconds_double;
    if (fscanf(uptime_file, "%lf", &uptime_seconds_double) != 1) {
        perror("fscanf /proc/uptime");
        fclose(uptime_file);
        return -2; // Error reading uptime file
    }
    fclose(uptime_file);
    uptime_jiffies = (unsigned long long)(uptime_seconds_double * sysconf(_SC_CLK_TCK));


    hertz = sysconf(_SC_CLK_TCK);
    if (hertz <= 0) {
        fprintf(stderr, "sysconf(_SC_CLK_TCK) failed or returned invalid value\n");
        return -2; // Error getting clock ticks per second
    }

    unsigned long total_time = utime + stime + cutime + cstime;
    double seconds = (double)(uptime_jiffies - starttime) / hertz;

    if (seconds <= 0) {
        *cpu_percent = 0.0; // Avoid division by zero or negative time
    } else {
        *cpu_percent = 100.0 * ((double)total_time / hertz) / seconds;
    }

    return 0; // Success
}

// ********************************* //

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *pid_str = argv[1];
    pid_t pid;
    char *endptr;

    // Validate PID argument - must be a positive integer
    for (char *p = pid_str; *p != '\0'; p++) {
        if (!isdigit(*p)) {
            char* datetime_str = get_current_datetime_string();
            fprintf(stderr, "%s: %s invalid request\n", datetime_str ? datetime_str : "ERROR", pid_str);
            free(datetime_str);
            printf("invalid\n");
            return EXIT_FAILURE;
        }
    }

    pid = strtol(pid_str, &endptr, 10);
    if (*endptr != '\0' || pid <= 0) { // Basic PID validation
        char* datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: %s invalid request\n", datetime_str ? datetime_str : "ERROR", pid_str);
        free(datetime_str);
        printf("invalid\n");
        return EXIT_FAILURE;
    }

    float cpu_usage;
    int result = calculate_cpu_usage(pid, &cpu_usage);

    char* datetime_str = get_current_datetime_string();
    if (!datetime_str) {
        fprintf(stderr, "ERROR: Failed to get datetime for log\n");
    }

    if (result == 0) {
        printf("%.2f\n", cpu_usage);
        fprintf(stderr, "%s: PID %d %.2f%%\n", datetime_str ? datetime_str : "ERROR", pid, cpu_usage);
    } else if (result == -1) {
        printf("not found\n");
        fprintf(stderr, "%s: PID %d not found\n", datetime_str ? datetime_str : "ERROR", pid);
    } else {
        printf("error\n"); // Generic error if calculate_cpu_usage fails for other reasons
        fprintf(stderr, "%s: PID %d error calculating CPU usage\n", datetime_str ? datetime_str : "ERROR", pid);
    }

    free(datetime_str);

    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

// ********************************* //