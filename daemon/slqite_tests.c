#include <unistd.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>

int callback(void *data, int argc, char **argv, char **azColName){
	for(int i = 0; i < 1440; i++)
	{
		if(((float *)data)[i] == 0)
		{
			((float *)data)[i] = strtof(argv[0], NULL);
			break;
		}
	}
	return 0;
}



int main(int argc, char* argv[])
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char * sql = malloc(2048);
	if (!sql) { perror("malloc"); exit(EXIT_FAILURE); };

	rc = sqlite3_open("gather.db", &db);

	if( rc ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	}else{
		fprintf(stderr, "Opened database successfully\n");
	}

	//	Creates the table if it doesn't exist already

	sql = "CREATE TABLE IF NOT EXISTS NET_DATA("  \
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

  rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
	if(rc != SQLITE_OK){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		fprintf(stdout, "Table created successfully\n");
	}

	unsigned int pac_in = 10;
  unsigned int udp_in = 11;
  unsigned int tcp_in = 21;
  unsigned int icmp_in = 13;
  unsigned int other_in = 14;

  unsigned int pac_out = 51;
  unsigned int udp_out = 61;
  unsigned int tcp_out = 17;
  unsigned int icmp_out = 18;
  unsigned int other_out = 19;

	time_t t;
	srand((unsigned) time(&t));

	int * values = malloc(sizeof(int) * 10);
	values[0] = 5;
values[2] = 6;
values[3] = 4;
values[4] = 12;
values[5] = 3;
values[6] = 2;
values[7] = 1;
values[8] = 7;
values[9] = 7;
values[1] = 6;
	//	Inserts into table
	for(int i = 0; i < 10; i++){
		char * sql2 = malloc(512);
		snprintf(sql2, 512, "INSERT INTO NET_DATA (PAC_IN,UDP_IN,TCP_IN,ICMP_IN,OTHER_IN,PAC_OUT,UDP_OUT,TCP_OUT,ICMP_OUT,OTHER_OUT)"\
			" VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u);", 
			pac_in, udp_in, tcp_in, icmp_in, other_in, pac_out, 
			udp_out, tcp_out, icmp_out, other_out);
			pac_in = values[i];
			pac_out = rand() % 100;
		rc = sqlite3_exec(db, sql2, NULL, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		free(sql2);
	}
	fprintf(stdout, "Records created successfully\n");
	exit(0);

	//	Get * from db	
	char * sql3 = malloc(32);
	float *sample_pacin = calloc(1440, sizeof(float));
	sql3 = "SELECT PAC_IN from NET_DATA";
	rc = sqlite3_exec(db, sql3, callback, sample_pacin, &zErrMsg);
	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}else{
		fprintf(stdout, "Operation done successfully\n");
	}

	for(int i = 0; i < 10; i++)
		printf("element of float %f\n", sample_pacin[i]);
	



	sqlite3_close(db);
}






















