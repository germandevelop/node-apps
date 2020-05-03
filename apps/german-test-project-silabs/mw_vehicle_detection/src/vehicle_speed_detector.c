
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "vehicle_speed_detector.h"

#include "FFT.h"

static void sorted_order_of_indices (const double *arr, int n, int *sorted_order);
static void bit_alignment(double *data_startpoint, double *y_startpoint, int data_points);


static void int_swap(int * a, int * b)
{
	int temp = *b;
	*b = *a;
	*a = temp;
}


static void bit_alignment(double *data_startpoint, double *y_startpoint, int data_points) {
	int i,j,k,i2;
	double tx;

	i2 = data_points >> 1;
	j = 0;

	for (i=0;i<data_points-1;i++)
	{
		if (i < j)
		{
			tx = *(data_startpoint + i);
			*(data_startpoint + i) = *(data_startpoint + j);
			*(data_startpoint + j) = tx;
		}
		k = i2;
		while (k <= j)
		{
			j -= k;
			k >>= 1;
		}
		j += k;
		*(y_startpoint + i) = 0.0;
	}
}


static void sorted_order_of_indices (const double *arr, int n, int *sorted_order)
{
	int i, j;

	for (i=0; i<n; i++){
		sorted_order[i] = i;
	}

	for (i=0; i<n; i++){
		for (j=i+1; j<n; j++){
			if (arr[sorted_order[i]] > arr[sorted_order[j]]){
				int_swap (&sorted_order[i], &sorted_order[j]);
			}
		}
 	}
}

// FIXME handle this better, the arrays can be shorter or user allocated
double filtered_result[256]; // (num_points/2) - LOW_PASS_FILTER
int sorted_order[256];

bool detect_vehicle_speed (double output[2], int num_points, double *points)
{
	#ifdef DOPPLER_DEBUG_OUTPUT
	FILE * qfile;
	qfile = fopen("fourier.txt", "w");
	FILE * sfile;
	sfile = fopen("sorter_fourier.txt", "w");
	FILE * afile;
	afile = fopen("sorter_indices.txt", "w");
	#endif//DOPPLER_DEBUG_OUTPUT

	//printf("start find doppler \n");

	//double filtered_result[(num_points/2) - LOW_PASS_FILTER];
	int num_freqs = (int)(num_points/2) - LOW_PASS_FILTER;

	double accumulated_energy = 0;
	double *data_startpoint, *y_startpoint;
	double doppler_speeds;

	data_startpoint = points;
	y_startpoint = points + num_points;
	bit_alignment(data_startpoint, y_startpoint, num_points);
	FFT(data_startpoint, y_startpoint, num_points);

	int j;
	//filter out low frequencies

	//printf("before filtering  \n");
	memset(filtered_result, 0, sizeof filtered_result); // fill with zeroes

	for (j=0; j<num_freqs; j++)
	{
		filtered_result[j] = data_startpoint[LOW_PASS_FILTER + j];
		//printf("%f ", filtered_result[j]);

		accumulated_energy += data_startpoint[LOW_PASS_FILTER + j];
	}
	//printf("\n");
	//printf("after accumulated_energy \n");
	//TEMPORARY SOLUTION FOR RETURNING SUMMED ENERGY OVER CURRENT FRAME

	//return (accumulated_energy * 1000);

	// Commented out, as computing doppler frequency is not needed for testing hand movement
	//get sorted indices in ascending order

	//int sorted_order[sizeof (int) * num_freqs];
	sorted_order_of_indices (filtered_result, num_freqs, sorted_order);

	//select frequency (index) which have spectral energy larger that defined threshold
	if (filtered_result[sorted_order[num_freqs - 1]] > SPECTRAL_ENERGY_COEFICIENT)
	{
		doppler_speeds = sorted_order[num_freqs - 1]*(FREQUENCY/(num_points));

		#ifdef DOPPLER_DEBUG_OUTPUT
			for (int i=0;i<num_freqs;i++)
			{
				fprintf(qfile, "%f\n", filtered_result[i]);
				fprintf(sfile, "%f\n", filtered_result[sorted_order[i]]);
				fprintf(afile, "%d\n", sorted_order[i]);
			}
		#endif//DOPPLER_DEBUG_OUTPUT

	}
	else
	{
		doppler_speeds = 0;
	}

	#ifdef DOPPLER_DEBUG_OUTPUT
	fclose(qfile);
	fclose(sfile);
	fclose(afile);
	#endif//DOPPLER_DEBUG_OUTPUT

	output[0] = doppler_speeds / SPEED_COEFICIENT;
	output[1] = accumulated_energy;

	return true;
}
