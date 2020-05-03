
#include <inttypes.h>

void normalize_signal(uint8_t bits, int count, const uint16_t samples[], double normalized[])
{
	double mw_bias = 0;

	for(int i=0;i<count;i++)
	{
		mw_bias += samples[i];
	}
	mw_bias /= count;

	for(int i=0;i<count;i++)
	{
		normalized[i] = (samples[i]-mw_bias)/((1 << bits) - 1);//subtract bias and normalize
		//printf("%"PRIi32".%"PRIi32"\n", (int32_t)vol, abs((int32_t)(vol*1000) - (int32_t)vol*1000));
	}
}
