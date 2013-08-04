#ifndef RANDOMLIB_ABC_H
#define RANDOMLIB_ABC_H

#ifdef __cplusplus
extern "C" {
#endif

// 0 <= IJ <= 31328  and  0 <= KL <= 30081 
void   PAPI RandomInitialise(int ij, int kl);
double PAPI RandomUniform(void);
double PAPI RandomGaussian(double mean, double stddev);
int    PAPI RandomInt(int lower, int upper);
int    PAPI RandomBool();
double PAPI RandomDouble(double lower, double upper);

#ifdef __cplusplus
}
#endif

#endif
