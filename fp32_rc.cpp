
#include <stdio.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

static const uint NUM     = 3;
static const uint BITS    = NUM*8;
static const double sTOP  = 0x01ULL<<(BITS-8);
static const qword ThresL = 0xFFULL<<(BITS-8);
static const qword ThresH = 0x01ULL<<(BITS-0);

struct Rangecoder {
  uint f_DEC; // 0=encode, 1=decode;
  FILE* f;
  byte get() { return byte(getc(f)); }
  void put( byte c ) { putc(c,f); }
  void StartEncode( FILE *F ) { f=F; f_DEC=0; rc_Init(); }
  void FinishEncode( void ) { rc_Quit(); }
  void StartDecode( FILE *F ) { f=F; f_DEC=1; rc_Init(); }

  uint  low;
  float code; 
  float range;

  uint  FFNum;
  uint  Cache;

  void rc_Renorm( void ) {
    while( range<sTOP ) ShiftStuff();
  }

  void rc_Process( qword cumFreq, qword freq, qword totFreq ) {

    qword  tmp  = (float(cumFreq)*range)/totFreq+1;
    float rnew = (float(cumFreq+freq)*range)/totFreq;

    if( f_DEC ) code-=tmp; else low += tmp;
    range = rnew - tmp;

    rc_Renorm();
  }

  uint rc_GetFreq( uint totFreq ) {
    return float(code)*totFreq/range;
  }

  void ShiftStuff( void ) {
    range *= 256;
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    code *= 256;
    uint c = get();
    FFNum += (c==-1);
    code += byte(c);
  }

  void ShiftLow( void ) {
    uint Carry = (low>=ThresH);
    if( (low<ThresL) || Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for(;FFNum!=0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>(BITS-8);
      Carry = 0;
    } else FFNum++;
    low<<=8;
    low &= ThresH-1;
  }

  void rc_Init( void ) {
    low   = 0;
    FFNum = 0;
    Cache = -1;
    if( f_DEC==1 ) {
      for(int _=0; _<NUM; _++) ShiftCode();
    }
    range = ThresH;//0xFFFFFFFF;
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword llow=low;
      qword high=llow+range;
      qword mask=0xFF;
      uint Carry = (low>=ThresH);

      for( i=0; i<NUM; i++ ) {
        if( (llow|mask)<high ) llow|=mask,n--;
        (mask<<=8)+=0xFF;
      }

      if( (Cache!=-1) && ((n!=0) || (Cache+Carry!=0xFF) ) ) put( Cache+Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also FFs
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( llow>>(BITS-8) ), llow<<=8;
    }
  }

};

#undef RESCALE_64k
#define RESCALE_64k
#include "main.inc"
