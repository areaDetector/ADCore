/* fft.h 
 * Mark Rivers
 * February 27, 2016
 */
 
#ifndef FFT_H
#define FFT_H

#ifdef __cplusplus
extern "C" {
#endif

void fft_1D(double data[], unsigned long nn, int isign);
void fft_ND(double data[], unsigned long nn[], int ndim, int isign);

#ifdef __cplusplus
}
#endif

#endif  /* FFT_H */
