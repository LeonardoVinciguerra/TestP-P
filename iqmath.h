//---------------------------------------------------------------------------
//
// Name:        iqmath.h
// Author:      Gabriel Ferri
// Created:     23/06/2011
// Description: Definitions for the IQ math
//
//---------------------------------------------------------------------------
#ifndef __IQMATH_H
#define __IQMATH_H


	//---------------//
	//    IQ Math    //
	//---------------//

typedef   int    _iq;
typedef   int    _iq30;
typedef   int    _iq29;
typedef   int    _iq28;
typedef   int    _iq27;
typedef   int    _iq26;
typedef   int    _iq25;
typedef   int    _iq24;
typedef   int    _iq23;
typedef   int    _iq22;
typedef   int    _iq21;
typedef   int    _iq20;
typedef   int    _iq19;
typedef   int    _iq18;
typedef   int    _iq17;
typedef   int    _iq16;
typedef   int    _iq15;
typedef   int    _iq14;
typedef   int    _iq13;
typedef   int    _iq12;
typedef   int    _iq11;
typedef   int    _iq10;
typedef   int    _iq9;
typedef   int    _iq8;
typedef   int    _iq7;
typedef   int    _iq6;
typedef   int    _iq5;
typedef   int    _iq4;
typedef   int    _iq3;
typedef   int    _iq2;
typedef   int    _iq1;
//---------------------------------------------------------------------------
#define   _IQ30(A)      (int) ((A) * 1073741824.0L)
#define   _IQ29(A)      (int) ((A) * 536870912.0L)
#define   _IQ28(A)      (int) ((A) * 268435456.0L)
#define   _IQ27(A)      (int) ((A) * 134217728.0L)
#define   _IQ26(A)      (int) ((A) * 67108864.0L)
#define   _IQ25(A)      (int) ((A) * 33554432.0L)
#define   _IQ24(A)      (int) ((A) * 16777216.0L)
#define   _IQ23(A)      (int) ((A) * 8388608.0L)
#define   _IQ22(A)      (int) ((A) * 4194304.0L)
#define   _IQ21(A)      (int) ((A) * 2097152.0L)
#define   _IQ20(A)      (int) ((A) * 1048576.0L)
#define   _IQ19(A)      (int) ((A) * 524288.0L)
#define   _IQ18(A)      (int) ((A) * 262144.0L)
#define   _IQ17(A)      (int) ((A) * 131072.0L)
#define   _IQ16(A)      (int) ((A) * 65536.0L)
#define   _IQ15(A)      (int) ((A) * 32768.0L)
#define   _IQ14(A)      (int) ((A) * 16384.0L)
#define   _IQ13(A)      (int) ((A) * 8192.0L)
#define   _IQ12(A)      (int) ((A) * 4096.0L)
#define   _IQ11(A)      (int) ((A) * 2048.0L)
#define   _IQ10(A)      (int) ((A) * 1024.0L)
#define   _IQ9(A)       (int) ((A) * 512.0L)
#define   _IQ8(A)       (int) ((A) * 256.0L)
#define   _IQ7(A)       (int) ((A) * 128.0L)
#define   _IQ6(A)       (int) ((A) * 64.0L)
#define   _IQ5(A)       (int) ((A) * 32.0L)
#define   _IQ4(A)       (int) ((A) * 16.0L)
#define   _IQ3(A)       (int) ((A) * 8.0L)
#define   _IQ2(A)       (int) ((A) * 4.0L)
#define   _IQ1(A)       (int) ((A) * 2.0L)

#define   _IQ15ToF(A)   (double) ((A) / 32768.0L)
#define   _IQ24ToF(A)   (double) ((A) / 16777216.0L)

#endif
