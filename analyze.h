#ifndef analyze.h
#define analyze.h

typedef int (*dist)(double a, double b);

typedef struct data_point {
	int freq;
	int count;
} data_point;

typedef struct parameters {
	int param1;
	int param2;
} params;


bool check_dist(double param_1, double param_2, int data_point, dist d);

double getMean(int n, data_point *data[]);

double getVariance(int n, double mean, data_point *data[]);

params bootStrap(double counts[], double freqs[]);



#endif
