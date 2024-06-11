#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


void   init_cpu(unsigned int n, float *a);
double get_elapsed_time(const struct timeval *tv0, const struct timeval *tv1);

void calc(unsigned int nx, unsigned int ny, const float *a, const float *b, float *c)
{
    
#pragma acc kernels present(a,b,c)
#pragma acc loop independent
    for (unsigned int j=0; j<ny; j++) {
#pragma acc loop independent
	for (unsigned int i=0; i<nx; i++) {
	    const int ix = i + j*nx;
	    c[ix] += a[ix] + b[ix];
	}
    }
                
}

int main(int argc, char *argv[])
{
    const unsigned int nt = 1000;
    
    const unsigned int nx = 4096;
    const unsigned int ny = 4096;
    const unsigned int n  = nx * ny;
    float *a = malloc(n*sizeof(float));
    float *b = malloc(n*sizeof(float));
    float *c = malloc(n*sizeof(float));

    const float b0 = 2.0;

    struct timeval tv0;
    gettimeofday(&tv0, NULL);

    /**** Begin ****/
    
    init_cpu(n, a);

    double sum = 0;
#pragma acc data copyin(a[0:n]) create(b[0:n]) copyout(c[0:n])
    {
#pragma acc kernels present(b)
#pragma acc loop independent
	for (unsigned int i=0; i<n; i++) {
	    b[i] = b0;
	}
#pragma acc kernels present(c)
#pragma acc loop independent
	for (unsigned int i=0; i<n; i++) {
	    c[i] = 0.0;
	}

	for (unsigned int icnt=0; icnt<nt; icnt++) {
	    calc(nx, ny, a, b, c);
	}
    }

	for (unsigned int i=0; i<n; i++) {
	    sum += c[i];
	}

    /**** End ****/
    
    struct timeval tv1;
    gettimeofday(&tv1, NULL);
    
    fprintf(stdout, "mean = %5.2f\n", sum / n);
    fprintf(stdout, "Time = %8.3f [sec]\n", get_elapsed_time(&tv0, &tv1));
    
    free(a);
    free(b);
    free(c);
    
    return 0;
}

void   init_cpu(unsigned int n, float *a)
{
    for (unsigned int i=0; i<n; i++) {
	a[i] = 1.0;
    }
}

double get_elapsed_time(const struct timeval *tv0, const struct timeval *tv1)
{
    return (double)(tv1->tv_sec - tv0->tv_sec) + (double)(tv1->tv_usec - tv0->tv_usec)*1.0e-6;
}
