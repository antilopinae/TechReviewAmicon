#include "process_cpu_usage.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utility.h"

static long long get_total_time()
{
    FILE* file = fopen("/proc/stat", "r");
    if (!file)
    {
        DEBUG_LOG("get_total_time: Error open /proc/stat: %s\n", strerror(errno));
        return -1;
    }

    char buffer[128];
    if (fgets(buffer, sizeof(buffer), file) == NULL)
    {
        DEBUG_LOG("get_total_time: Error read из /proc/stat: %s\n", strerror(errno));
        fclose(file);
        return -1;
    }
    fclose(file);

    long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8)
    {
        DEBUG_LOG("get_total_time: Error parsing /proc/stat\n");
        return -1;
    }

    return user + nice + system + idle + iowait + irq + softirq + steal;
}

static long long get_process_time(int pid)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE* file = fopen(path, "r");
    if (!file)
    {
        DEBUG_LOG("get_process_time: Error open %s: %s\n", path, strerror(errno));
        return -1;
    }

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL)
    {
        DEBUG_LOG("get_process_time: Error read %s: %s\n", path, strerror(errno));
        fclose(file);
        return -1;
    }
    fclose(file);

    long long utime, stime;
    unsigned long long dummy;
    if (sscanf(buffer, "%llu %*s %*c %*llu %*llu %*llu %*llu %*llu %*llu %*llu %*llu %*llu %*llu %lld %lld", &dummy, &utime, &stime) != 3)
    {
        DEBUG_LOG("get_process_time: Error parse %s\n", path);
        return -1;
    }

    return utime + stime;
}

double get_process_cpu_usage(pid_t pid)
{
    long long start_total = get_total_time();
    long long start_proc = get_process_time(pid);

    if (start_total == -1 || start_proc == -1)
        return -1.0;

    usleep(20000);    // 0.02s

    long long end_total = get_total_time();
    long long end_proc = get_process_time(pid);

    if (end_total == -1 || end_proc == -1)
        return -1.0;

    long long total_diff = end_total - start_total;
    long long proc_diff = end_proc - start_proc;

    if (total_diff <= 0)
    {
        DEBUG_LOG("get_process_cpu_usage: total_diff <= 0, returning 0%%\n");
        return 0.0;
    }

    return (100.0 * proc_diff) / total_diff;
}
