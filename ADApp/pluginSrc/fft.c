/** Modified code from Numerical Recipes four1 and fourn **/

#include <math.h>
#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

#include "fft.h"

/* Some systems do not define M_PI in math.h */
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif
 
void fft_1D(double data[], unsigned long nn, int isign)
{
  unsigned long n,mmax,m,j,istep,i;
  double wtemp,wr,wpr,wpi,wi,theta;
  double tempr,tempi;

  /* This algorithm uses 1-based array indices.  To be cleanly callable from C
     we subtract 1 here */
  data--;

  n=nn << 1;
  j=1;
  for (i=1;i<n;i+=2) {
    if (j > i) {
      SWAP(data[j],data[i]);
      SWAP(data[j+1],data[i+1]);
    }
    m=n >> 1;
    while (m >= 2 && j > m) {
      j -= m;
      m >>= 1;
    }
    j += m;
  }
  mmax=2;
  while (n > mmax) {
    istep=mmax << 1;
    theta=isign*(M_PI*2/mmax);
    wtemp=sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi=sin(theta);
    wr=1.0;
    wi=0.0;
    for (m=1;m<mmax;m+=2) {
      for (i=m;i<=n;i+=istep) {
        j=i+mmax;
        tempr=(double)(wr*data[j]-wi*data[j+1]);
        tempi=(double)(wr*data[j+1]+wi*data[j]);
        data[j]=data[i]-tempr;
        data[j+1]=data[i+1]-tempi;
        data[i] += tempr;
        data[i+1] += tempi;
      }
      wr=(wtemp=wr)*wpr-wi*wpi+wr;
      wi=wi*wpr+wtemp*wpi+wi;
    }
    mmax=istep;
  }
}

void fft_ND(double data[], unsigned long nn[], int ndim, int isign)
{
  int idim;
  unsigned long i1,i2,i3,i2rev,i3rev,ip1,ip2,ip3,ifp1,ifp2;
  unsigned long ibit,k1,k2,n,nprev,nrem,ntot;
  double tempi,tempr;
  double theta,wi,wpi,wpr,wr,wtemp;

  /* This algorithm uses 1-based array indices.  To be cleanly callable from C
     we subtract 1 here */
  data--;
  nn--;

  for (ntot=1,idim=1;idim<=ndim;idim++)
    ntot *= nn[idim];
  nprev=1;
  for (idim=ndim;idim>=1;idim--) {
    n=nn[idim];
    nrem=ntot/(n*nprev);
    ip1=nprev << 1;
    ip2=ip1*n;
    ip3=ip2*nrem;
    i2rev=1;
    for (i2=1;i2<=ip2;i2+=ip1) {
      if (i2 < i2rev) {
        for (i1=i2;i1<=i2+ip1-2;i1+=2) {
          for (i3=i1;i3<=ip3;i3+=ip2) {
            i3rev=i2rev+i3-i2;
            SWAP(data[i3],data[i3rev]);
            SWAP(data[i3+1],data[i3rev+1]);
          }
        }
      }
      ibit=ip2 >> 1;
      while (ibit >= ip1 && i2rev > ibit) {
          i2rev -= ibit;
          ibit >>= 1;
      }
      i2rev += ibit;
    }
    ifp1=ip1;
    while (ifp1 < ip2) {
      ifp2=ifp1 << 1;
      theta=isign*M_PI*2/(ifp2/ip1);
      wtemp=sin(0.5*theta);
      wpr = -2.0*wtemp*wtemp;
      wpi=sin(theta);
      wr=1.0;
      wi=0.0;
      for (i3=1;i3<=ifp1;i3+=ip1) {
        for (i1=i3;i1<=i3+ip1-2;i1+=2) {
          for (i2=i1;i2<=ip3;i2+=ifp2) {
            k1=i2;
            k2=k1+ifp1;
            tempr=(double)wr*data[k2]-(double)wi*data[k2+1];
            tempi=(double)wr*data[k2+1]+(double)wi*data[k2];
            data[k2]=data[k1]-tempr;
            data[k2+1]=data[k1+1]-tempi;
            data[k1] += tempr;
            data[k1+1] += tempi;
          }
        }
        wr=(wtemp=wr)*wpr-wi*wpi+wr;
        wi=wi*wpr+wtemp*wpi+wi;
      }
      ifp1=ifp2;
    }
    nprev *= n;
  }
}
