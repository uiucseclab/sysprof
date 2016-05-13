#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "../include/stats.h"
#include "./statcalcs.h"

//	Extract stored cutoffs
int extract_cutoff(void *data, int argc, char **argv, char **azColName){
	for(int i = 0; i < argc; i++)
		((float *)data)[i] = strtol(argv[i], NULL, 10);
	return 0;
}


int main(){
	//	Constants
	int PAGE_SIZE = getpagesize();
	size_t FILESIZE = PAGE_SIZE * 32;
	char * FILEPATH = "/dev/sysprof";
	int my_pid = getpid();


	// Counters
	int index = 0;
	int newsample_counter = 0;


	//	Store pid for kernel to use
	FILE *ifp;
	char *mode = "w";
	ifp = fopen("/proc/sysprof", mode);
	fprintf(stdout, "R %d\n", my_pid);
	fprintf(ifp, "R %d", my_pid);
	fclose(ifp);


	//	Open database connection
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	rc = sqlite3_open("gather.db", &db);
	if(rc){
  	fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
  	exit(EXIT_FAILURE);
  }


	//	Create raw data table if it doesn't exist
	char * sql_create = malloc(256);
	sql_create = "CREATE TABLE IF NOT EXISTS NET_DATA("  \
         "PAC_IN				INT," \
         "UDP_IN				INT," \
         "TCP_IN        INT," \
         "ICMP_IN				INT," \
         "OTHER_IN			INT," \
         "PAC_OUT				INT," \
         "UDP_OUT				INT," \
         "TCP_OUT				INT," \
         "ICMP_OUT			INT," \
         "OTHER_OUT			INT);";
  rc = sqlite3_exec(db, sql_create, NULL, 0, &zErrMsg);
	if(rc != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		exit(EXIT_FAILURE);
	}
  

	//	Get a file descriptor to the char device.
	int fd = open(FILEPATH, O_RDONLY|O_SYNC);
	if(fd < 0){
		fprintf(stderr, "Error opening file for reading.");
		exit(EXIT_FAILURE);
	}


	//	Memory map the shared memory area
	void * map = mmap(0, FILESIZE, PROT_READ, MAP_SHARED, fd, 0);
	if(!map){
		fprintf(stderr, "Failed to mmap device file.\n");
		return -1;
	}
	struct nf_data * shmem = (struct nf_data *)map;


	//	Loop and wait for signal
	for(;;){
		kill(my_pid, SIGSTOP);
		newsample_counter = newsample_counter + 1;
		if(newsample_counter >= SAMPLE_SIZE){
			system("./statcalcs");
			newsample_counter = newsample_counter % SAMPLE_SIZE;
		}


		//	Extract data
		if(map + (index + 1) * sizeof(struct nf_data) > map + FILESIZE)
			index = 0;
		struct nf_data current = shmem[index];
		index++;


		//	Insert newly collected data into database
		char * sql_add = malloc(512);
		snprintf(sql_add, 512, 
			"INSERT INTO NET_DATA (PAC_IN,UDP_IN,TCP_IN,ICMP_IN,OTHER_IN,PAC_OUT,UDP_OUT,TCP_OUT,ICMP_OUT,OTHER_OUT)"\
			" VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u);", current.pac_in, current.udp_in, current.tcp_in, 
			current.icmp_in, current.other_in, current.pac_out, current.udp_out, current.tcp_out, current.icmp_out, current.other_out);
		rc = sqlite3_exec(db, sql_add, NULL, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}


		//	Get cutoffs from database	
		char * cutoff_data = malloc(sizeof(int) * 11);
		char * sql_cutoffs = malloc(32);
		sql_cutoffs = "SELECT * from NET_CUTOFFS";
		rc = sqlite3_exec(db, sql_cutoffs, extract_cutoff, (void *)cutoff_data, &zErrMsg);
		if( rc != SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			exit(EXIT_FAILURE);
		}
		free(sql_cutoffs);

		
		// Check if current data is within parameters
			// Create signal if neccessary

		//	Compare sample data to cutoff
		if(current.pac_in > cutoff_data[0])
			puts("WARNING: Recieved more packets than normal.  There is a possibility that this is normal performance, however there is a chance this is an attack.");

		if(current.pac_out > cutoff_data[5])
			puts("WARNING: Sent more packets than normal.  There is a possibility that this is normal performance, however there is a chance this is an attack.");





		printf("pack in %u\npack out %u\n", current.pac_in, current.pac_out);
	}


	//	Clean up and exit
	sqlite3_close(db);
	munmap(map, FILESIZE);
	close(fd);
	return 0;
}
