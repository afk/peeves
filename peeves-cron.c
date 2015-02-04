#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <curl/curl.h>

#define FROM "<reminder-noreply@ttcrotgoldkoeln.de>"

int sendMail(char *to, FILE *filep)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "smtp://localhost");

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

    recipients = curl_slist_append(recipients, to);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READDATA, filep);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed with error #%i: %s\n", (int)res, curl_easy_strerror(res));

    curl_slist_free_all(recipients);

    curl_easy_cleanup(curl);
  }

  return (int)res;
}

int main(int argc, char **argv) {
    char line[1024];
    int year, mon, day, hour, min;
    char filename[10], to[1024];

    time_t time_now;
    time_t time_cron;
    struct tm *timeinfo;
    
    time(&time_now);
    timeinfo = localtime(&time_now);

    chdir(getenv("HOME"));
    chdir(".peeves");

    char file_name[] = "peeves_cron.XXXXXX";
    int fd;

    fd = mkstemp(file_name);
    if (fd < 1) {
        printf("Creation of temp file failed with error [%s]\n", strerror(errno));
	return 1;
    }

    FILE* cron_file = fopen("peeves_cron", "r");
    if (!cron_file) {
        printf("Reading of cron file failed with error [%s]\n", strerror(errno));
	return 1;
    }

    while (fgets(line, sizeof(line), cron_file)) {
        sscanf(line, "%i\t%i\t%i\t%i\t%i\t%s\t%[^\n]\n", &year, &mon, &day, &hour, &min, filename, to);
	timeinfo->tm_year = year - 1900;
	timeinfo->tm_mon = mon - 1;
	timeinfo->tm_mday = day;
	timeinfo->tm_hour = hour;
	timeinfo->tm_min = min;

	time_cron = mktime(timeinfo);

	if (time_cron <= time_now) {
	    FILE *msg_filep = fopen(filename, "r");
            if (!msg_filep) {
                printf("Reading of msg file failed with error [%s]\n", strerror(errno));
	        return 1;
            }

	    int ret = sendMail(to, msg_filep);
	    fclose(msg_filep);

	    if(ret != CURLE_OK) {
	        write(fd, line, strlen(line));
	    } else {
	        remove(filename);
	    }
	} else {
	    write(fd, line, strlen(line));
	}
    }

    fclose(cron_file);
    close(fd);

    remove("peeves_cron");
    rename(file_name, "peeves_cron");

    return 0;
}
