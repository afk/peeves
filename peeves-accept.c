#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define FROM "<reminder-noreply@ttcrotgoldkoeln.de>"

char new_to[1024];
struct tm *timeinfo;

char *trimWS(char *ptr) {
    char *end = ptr + strlen(ptr) - 1;
    
    while (isspace(*ptr)) ptr++;
    
    if (*ptr != 0) {
        while (end > ptr && isspace(*end)) end--;
        *(end + 1) = 0;
    }
    
    return ptr;
}

void calculateReminderDate(char *reminder_date) {
    /*
     * y: years
     * m: months
     * w: weeks
     * d: days
     *
     * dd.mm.yyyy
     * yyyy-mm-dd
     *
     * hh:mm
     */

    time_t rawtime;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char *years = strstr(reminder_date, "y");
    char *months = strstr(reminder_date, "m");
    char *weeks = strstr(reminder_date, "w");
    char *days = strstr(reminder_date, "d");
    char *points = strstr(reminder_date, ".");
    char *dashes = strstr(reminder_date, "-");
    char *colon = strstr(reminder_date, ":");

    if (years) {
	char *end = years;
	while (!isspace(*(years - 1))) years--;
	*end = 0;

	timeinfo->tm_year += atoi(years);
	rawtime = mktime(timeinfo);
    }
    
    if (months) {
	char *end = months;
	while (!isspace(*(months - 1))) months--;
	*end = 0;

	timeinfo->tm_mon += atoi(months);
	rawtime = mktime(timeinfo);
    }
    
    if (weeks) {
	char *end = weeks;
	while (!isspace(*(weeks - 1))) weeks--;
	*end = 0;

	rawtime += 60 * 60 * 24 * 7 * atoi(weeks);
        timeinfo = localtime(&rawtime);
    }
    
    if (days) {
	char *end = days;
	while (!isspace(*(days - 1))) days--;
	*end = 0;

	rawtime += 60 * 60 * 24 * atoi(days);
        timeinfo = localtime(&rawtime);
    }
    
    if (points) {
	char *day = strtok(reminder_date, ".");
	char *month = strtok(NULL, ".");
	char *year = strtok(NULL, ".");

	timeinfo->tm_year = atoi(year) - 1900;
	timeinfo->tm_mon = atoi(month) - 1;
	timeinfo->tm_mday = atoi(day);

	rawtime = mktime(timeinfo);
    }
    
    if (dashes) {
        char *year = strtok(reminder_date, "-");
	char *month = strtok(NULL, "-");
	char *day = strtok(NULL, "-");

	timeinfo->tm_year = atoi(year) - 1900;
	timeinfo->tm_mon = atoi(month) - 1;
	timeinfo->tm_mday = atoi(day);

	rawtime = mktime(timeinfo);
    }

    if (colon) {
        char *hour = colon;
	char *min = colon + 1;
	char *end = min;
	while (!isspace(*(hour - 1))) hour--;
	*colon = 0;
	while (!isspace(*end)) end++;
	*end = 0;

	timeinfo->tm_hour = atoi(hour);
	timeinfo->tm_min = atoi(min);
	timeinfo->tm_sec = 0;

	rawtime = mktime(timeinfo);
    }

    timeinfo = localtime(&rawtime);
}

char *extractHeaderContent(char *line) {
    char *content = strstr(line, ":") + 1;
    while (isspace(*content)) content++;
    return content;
}

void handleFrom(char *orig_line) {
    char line[1024];
    strcpy(line, orig_line);
    
    char *from_header_content = extractHeaderContent(line);
    strcpy(new_to, from_header_content);
    trimWS(new_to);

    strcpy(orig_line, "To: ");
    strcat(orig_line, from_header_content);

    char env_to[1024];
    strcpy(env_to, new_to);
    char *less_sign = strstr(env_to, "<");
    char *greater_sign = strstr(env_to, ">");

    if (less_sign && greater_sign) {
        *greater_sign = 0;
	strcpy(new_to, less_sign + 1);
    }
}

void handleSubject(char *orig_line) {
    char line[1024];
    strcpy(line, orig_line);
    
    char *subject_header_content = extractHeaderContent(line);
    char *reminder_date = strtok(subject_header_content, "|");
    char *orig_subject = strtok(NULL, "|");
    while (isspace(*orig_subject)) orig_subject++;

    calculateReminderDate(reminder_date);

    strcpy(orig_line, "Subject: # Reminder # ");
    strcat(orig_line, orig_subject);
}

void updateCron(int year, int mon, int day, int hour, int min, char *filename) {
    FILE* cron_file = fopen("peeves_cron", "a");
    if (!cron_file) {
        printf("Updating of cron file failed with error [%s]\n", strerror(errno));
	exit(1);
    }
    fprintf(cron_file, "%i\t%i\t%i\t%i\t%i\t%s\t%s\n", year + 1900, mon + 1, day, hour, min, filename, new_to);
    fclose(cron_file);
}

int main(int argc, char **argv) {
    char line[1024];
    int print = 1;
    int prev_deleted = 0;
    int header = 1;

    char file_name[] = "msgXXXXXX";
    int fd;
    
    chdir(getenv("HOME"));
    chdir(".peeves");
    fd = mkstemp(file_name);

    if (fd < 1) {
        printf("Creation of temp file failed with error [%s]\n", strerror(errno));
	return 1;
    }

    while (fgets(line, sizeof(line), stdin)) {
        if (header) {
            if (strstr(line, "Received") || 
	        strstr(line, "Date") || 
		strstr(line, "Message-ID") || 
		strstr(line, "X-Sender") || 
		strstr(line, "Return-Path") || 
		strstr(line, "User-Agent") || 
		strstr(line, "Delivered-To")) {
	            print = 0;
	            prev_deleted = 1;
	    } else if (strstr(line, "From")) {
	        handleFrom(line);
	    } else if (strstr(line, "To")) {
	        strcpy(line, "From: " FROM "\r\n");
	    } else if (strstr(line, "Subject")) {
	        handleSubject(line);
	    } else if (prev_deleted) {
		if (isspace(line[0])) {
		    print = 0;
		} else {
		    prev_deleted = 0;
		}
	    } 
	
	    char *ptr = line;
	    while (isspace(*ptr)) ptr++;
	
	    if (ptr[0] == 0) {
	        header = 0;
	        print = 1;
	    }
	}

	if (print) {
	    write(fd, line, strlen(line));
	}

	print = 1;
    }
    
    close(fd);

    updateCron(timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, file_name);

    return 0;
}
