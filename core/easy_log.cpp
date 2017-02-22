#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>

inline void fprint_time(FILE *file)
{
	if (0 == file)
		return;
	char _pbuf[56] = {0};
	time_t now;
	time(&now);
	struct tm timenow;
	localtime_r(&now, &timenow);
	strftime(_pbuf,sizeof(_pbuf),"%Y-%m-%d %H:%M:%S", &timenow);
	fprintf(file,"[%s]",_pbuf);


}

int 
my_log_init(FILE **p_file,const char * file_name)
{
	if (p_file == 0)
		return -1;
	*p_file = 0;
	// clear log
	FILE *temp_pfile = NULL;
	temp_pfile = fopen(file_name, "w");
	if (temp_pfile != NULL)
		fclose(temp_pfile);
	//
	*p_file = fopen(file_name, "a");
	if (*p_file == NULL)
	{
		printf("could not open %s because %s.\n",file_name,strerror(errno) );
		return -2;
	}
	return 0;
}

	int
my_vlog(FILE *p_file,const char *format, va_list ap)
{
	int ret;
	FILE *f = p_file;
	fprint_time(f);
	ret = vfprintf(f, format, ap);
	fflush(f);
	return ret;
}

	int
my_log(FILE *p_file, const char *format, ...)
{
	if (p_file == 0)
		return -1;
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = my_vlog(p_file, format, ap);
	va_end(ap);
	return ret;
}

