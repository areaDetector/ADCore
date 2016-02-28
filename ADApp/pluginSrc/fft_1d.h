/* fft_1d.h 
 * Mark Rivers
 * February 27, 2016
 */
 
#ifndef FFT_1D_H
#define FFT_1D_H

#ifdef __cplusplus
extern "C" {
#endif

void fft_1d(double data[], unsigned long nn, int isign);

#ifdef __cplusplus
}
#endif

#endif  /* FFT_1D_H */
