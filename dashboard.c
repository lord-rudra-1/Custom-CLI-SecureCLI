#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#include "dashboard.h"
#include "logger.h"

#define LOG_FILE "securecli.log"
#define MAX_LOG_LINES 20
#define MAX_PROCESS_LINES 15

static WINDOW *process_win;
static WINDOW *memory_win;
static WINDOW *log_win;
static WINDOW *status_win;
static int dashboard_active = 0;

// -------------------- MEMORY INFO --------------------
static void get_memory_info(char *buffer, size_t size) {
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (!meminfo) {
        snprintf(buffer, size, "Unable to read memory info");
        return;
    }

    long total_mem = 0, free_mem = 0, available_mem = 0;
    char line[256];

    while (fgets(line, sizeof(line), meminfo)) {
        if (sscanf(line, "MemTotal: %ld kB", &total_mem) == 1) continue;
        if (sscanf(line, "MemFree: %ld kB", &free_mem) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &available_mem) == 1) break;
    }
    fclose(meminfo);

    if (total_mem > 0) {
        long used_mem = total_mem - available_mem;
        double used_percent = (double)used_mem / total_mem * 100.0;
        snprintf(buffer, size,
            "Total: %.1f MB\nUsed: %.1f MB (%.1f%%)\nFree: %.1f MB\nAvailable: %.1f MB",
            total_mem / 1024.0, used_mem / 1024.0, used_percent,
            free_mem / 1024.0, available_mem / 1024.0);
    } else {
        snprintf(buffer, size, "Memory info unavailable");
    }
}

// -------------------- LOG READING --------------------
static void read_log_lines(char lines[][256], int max_lines, int *count) {
    *count = 0;
    FILE *log = fopen(LOG_FILE, "r");
    if (!log) return;

    char all_lines[1000][256];
    int total = 0;
    while (fgets(all_lines[total], sizeof(all_lines[0]), log) && total < 1000) {
        all_lines[total][strcspn(all_lines[total], "\n")] = 0;
        total++;
    }
    fclose(log);

    int start = (total > max_lines) ? total - max_lines : 0;

    for (int i = start; i < total && *count < max_lines; i++) {
        strncpy(lines[*count], all_lines[i], sizeof(lines[0]) - 1);
        lines[*count][sizeof(lines[0]) - 1] = '\0';
        (*count)++;
    }
}

// -------------------- DASHBOARD INIT --------------------
int dashboard_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    process_win = newwin(max_y / 2 - 1, max_x / 2, 0, 0);
    memory_win  = newwin(max_y / 2 - 1, max_x / 2, 0, max_x / 2);
    log_win     = newwin(max_y / 2, max_x / 2, max_y / 2, 0);
    status_win  = newwin(max_y / 2, max_x / 2, max_y / 2, max_x / 2);

    if (!process_win || !memory_win || !log_win || !status_win) {
        endwin();
        return -1;
    }

    dashboard_active = 1;
    dashboard_update();
    return 0;
}

// -------------------- DASHBOARD UPDATE --------------------
void dashboard_update(void) {
    if (!dashboard_active) return;

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // ---------- PROCESS PANEL ----------
    werase(process_win);
    box(process_win, 0, 0);
    wattron(process_win, COLOR_PAIR(1));
    mvwprintw(process_win, 0, 1, " Processes (PID | CPU%% | MEM%%) ");
    wattroff(process_win, COLOR_PAIR(1));

    mvwprintw(process_win, 1, 1, "PID     CPU%%   MEM%%");
    mvwprintw(process_win, 2, 1, "-----------------------");

    // NEW: Only PID, CPU%, MEM%
    FILE *proc = popen("ps -eo pid,pcpu,pmem --sort=-pcpu | head -n 16", "r");
    if (proc) {
        char line[256];
        int y = 3;
        fgets(line, sizeof(line), proc); // skip header
        while (fgets(line, sizeof(line), proc) && y < MAX_PROCESS_LINES + 3) {
            line[strcspn(line, "\n")] = 0;
            mvwprintw(process_win, y++, 1, "%s", line);
        }
        pclose(proc);
    }

    wrefresh(process_win);

    // ---------- MEMORY PANEL ----------
    werase(memory_win);
    box(memory_win, 0, 0);
    wattron(memory_win, COLOR_PAIR(1));
    mvwprintw(memory_win, 0, 1, " Memory Usage ");
    wattroff(memory_win, COLOR_PAIR(1));

    char mem_info[512];
    get_memory_info(mem_info, sizeof(mem_info));
    char *line = strtok(mem_info, "\n");
    int y = 1;
    while (line && y < max_y / 2 - 2) {
        mvwprintw(memory_win, y++, 1, "%s", line);
        line = strtok(NULL, "\n");
    }

    wrefresh(memory_win);

    // ---------- LOG PANEL ----------
    werase(log_win);
    box(log_win, 0, 0);
    wattron(log_win, COLOR_PAIR(1));
    mvwprintw(log_win, 0, 1, " Command Log ");
    wattroff(log_win, COLOR_PAIR(1));

    char log_lines[MAX_LOG_LINES][256];
    int log_count = 0;
    read_log_lines(log_lines, MAX_LOG_LINES, &log_count);

    wattron(log_win, COLOR_PAIR(5));
    for (int i = 0; i < log_count && i < max_y / 2 - 2; i++) {
        mvwprintw(log_win, i + 1, 1, "%s", log_lines[i]);
    }
    wattroff(log_win, COLOR_PAIR(5));

    wrefresh(log_win);

    // ---------- STATUS PANEL ----------
    werase(status_win);
    box(status_win, 0, 0);
    wattron(status_win, COLOR_PAIR(1));
    mvwprintw(status_win, 0, 1, " System Status ");
    wattroff(status_win, COLOR_PAIR(1));

    // Uptime: available on Linux via sysinfo; on other systems, skip
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        long days = info.uptime / 86400;
        long hours = (info.uptime % 86400) / 3600;
        long mins = (info.uptime % 3600) / 60;
        mvwprintw(status_win, 1, 1, "Uptime: %ldd %ldh %ldm", days, hours, mins);
    }
#endif

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    mvwprintw(status_win, 2, 1, "Time: %s", time_str);

    double load[3];
    if (getloadavg(load, 3) == 3) {
        mvwprintw(status_win, 3, 1, "Load: %.2f %.2f %.2f", load[0], load[1], load[2]);
    }

    mvwprintw(status_win, 5, 1, "Press 'q' to quit dashboard");
    mvwprintw(status_win, 6, 1, "Press 'r' to refresh");

    wrefresh(status_win);
    refresh();
}

// -------------------- CLEANUP --------------------
void dashboard_cleanup(void) {
    if (!dashboard_active) return;

    if (process_win) delwin(process_win);
    if (memory_win) delwin(memory_win);
    if (log_win) delwin(log_win);
    if (status_win) delwin(status_win);

    endwin();
    dashboard_active = 0;
}

int dashboard_is_active(void) {
    return dashboard_active;
}
