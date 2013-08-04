#pragma once

void PAPI xMat4ToDouble( float* src, double* dst );
void PAPI xMat4ToFloat( double* src, float* dst );

void PAPI xVecMul4f( float* mat, float* vec, float* res );
void PAPI xVecMul4d( double* mat, double* vec, double* res );

void PAPI xMatInv4f( float* mat );
void PAPI xMatInv4d( double* mat );
