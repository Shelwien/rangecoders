
#include <stdio.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

struct Rangecoder {
  uint f_DEC; // 0=encode, 1=decode;
  FILE* f;
  byte get() { return byte(getc(f)); }
  void put( byte c ) { putc(c,f); }
  void StartEncode( FILE *F ) { f=F; f_DEC=0; rc_Init(); }
  void FinishEncode( void ) { rc_Quit(); }
  void StartDecode( FILE *F ) { f=F; f_DEC=1; rc_Init(); }

  static const uint NUM   = 4;
  static const uint sTOP  = 0x01000000U;
  static const uint gTOP  = 0x00010000U;
  static const uint Thres = 0xFF000000U;

  union {
    struct {
      uint  low;  
      uint  Carry;
    };
    qword lowc;
  };
  uint  code; 
  uint  FFNum;
  uint  Cache;
  qword range;

  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }

  void rc_Renorm( void ) {
    while( range<sTOP ) ShiftStuff();
  }

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {

    uint tmp  = range-mulRdiv( totFreq-cumFreq, totFreq );
    uint rnew = range-mulRdiv( totFreq-(cumFreq+freq), totFreq );

    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range = rnew - tmp;

    rc_Renorm();
  }

  uint rc_GetFreq( uint totFreq ) {
    return muldivR( code, totFreq );
  }

  void ShiftStuff( void ) {
    range = (range<<8)+0x00;
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    code  = (code<<8)+0x00;
    uint c = get();
    FFNum += (c==-1);
    code += byte(c);
  }

  void ShiftLow( void ) {
    if( (low<Thres) || Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for(;FFNum!=0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }

  void rc_Init( void ) {
    low   = 0;
    FFNum = 0;
    Carry = 0;    
    Cache = -1;
    if( f_DEC==1 ) {
      for(int _=0; _<NUM; _++) ShiftCode();
    }
    range = 1ULL<<32;//0xFFFFFFFF;
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword llow=low;
      qword high=llow+range;

      if( (llow|      0xFF) < high ) low|=      0xFF,n--;
      if( (llow|    0xFFFF) < high ) low|=    0xFFFF,n--;
      if( (llow|  0xFFFFFF) < high ) low|=  0xFFFFFF,n--;
      if( (llow|0xFFFFFFFF) < high ) low|=0xFFFFFFFF,n--;

      if( (Cache!=-1) && ((n!=0) || (Cache+Carry!=0xFF) ) ) put( Cache+Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also FFs
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
    }
  }

};

#include "main.inc"
