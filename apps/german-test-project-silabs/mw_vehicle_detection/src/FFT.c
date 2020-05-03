/**
 * A basic software implementation of the Fast Fourier Transform.
 *
 * Copyright <TBD?>
 *
 * TODO: To whom does the copyright for this implementation belong to?
 *       Perhaps there should be a reference to somewhere, to some pseudocode
 *       or implementation guide?
 */
#include <math.h>

#include "FFT.h"

void FFT(double *data_startpoint, double *y_startpoint, int data_points) {
	int i,i1,j,l,l1,l2;
	double c1,c2,t1,t2,u1,u2,z;

	int power;
	int p;
	int power_of_two = 0;

	if(data_points != 0 && !(data_points & (data_points -1)))
	{
		p=data_points;
		while(((p & 1) == 0) && p > 1)
		{
			p >>= 1;
			power_of_two++;
		}
		power = 1;
	}
	else
	{
		power = 0;
	}

	if(power)
	{
		c1 = -1.0;
		c2 = 0.0;
		l2 = 1;
		for (l=0;l<power_of_two;l++)
		{
			l1 = l2;
			l2 <<= 1;
			u1 = 1.0;
			u2 = 0.0;
			for (j=0;j<l1;j++)
			{
				for (i=j;i<data_points;i+=l2)
				{
					i1 = i + l1;
					t1 = u1 * *(data_startpoint + i1) - u2 * y_startpoint[i1];
					t2 = u1 * y_startpoint[i1] + u2 * *(data_startpoint + i1);
					*(data_startpoint + i1) = *(data_startpoint + i) - t1;
					y_startpoint[i1] = y_startpoint[i] - t2;
					*(data_startpoint + i) += t1;
					y_startpoint[i] += t2;
				}
				z =  u1 * c1 - u2 * c2;
				u2 = u1 * c2 + u2 * c1;
				u1 = z;
			}
			c2 = -(sqrt((1.0 - c1) / 2.0));
			c1 = sqrt((1.0 + c1) / 2.0);
		}
		for (i=0;i<data_points;i++)
		{
			 *(data_startpoint + i) /= (double)data_points;
			 *(y_startpoint + i) /= (double)data_points;
			 *(data_startpoint + i) = sqrt(*(data_startpoint + i)* *(data_startpoint + i) + *(y_startpoint + i)* *(y_startpoint + i));
		}
	}
}
