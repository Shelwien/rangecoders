
#include <stdio.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

struct Rangecoder {
  uint f_DEC; // 0=encode, 1=decode;
  FILE* f;
  uint get() { return getc(f); }
  void put( byte c ) { putc(c,f); }
  void StartEncode( FILE *F ) { f=F; f_DEC=0; rc_Init(); }
//  void FinishEncode( void ) { rc_Quit(); }
  void StartDecode( FILE *F ) { f=F; f_DEC=1; rc_Init(); }

  byte getx() {
    uint c = get();
    EOFn += (c&0x0800)>>8;
    return byte(c);
  }

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
  uint  EOFn;
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

  uint rc_GetFreq1( uint totFreq ) {
    return muldivR( code-((1ULL<<EOFn)-1), totFreq );
  }

  void ShiftStuff( void ) {
    range = (range<<8)+0x00;
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    code  = (code<<8)+0x00;
    uint c = getx();
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
    EOFn  = 0;
    low   = 0;
    FFNum = 0;
    Carry = 0;    
    Cache = -1;
    if( f_DEC==1 ) for(int _=0; _<NUM; _++) ShiftCode();
    range = 1ULL<<32;
  }

  void rc_Quit( uint* freq, uint CNUM ) {
    if( f_DEC==0 ) {

      uint i,c,n,sumFreq,ln,mln,total;
      qword mid,hai,mid_h,mhai;

      sumFreq = 0; 
      hai = lowc; mln=0; total=0;
      for( c=0; c<CNUM; c++ ) total+=freq[c];
      for( c=0; c<CNUM; c++ ) {
        sumFreq += freq[c];
        mid = hai;
        hai = lowc + range-mulRdiv( total-sumFreq, total );
        if( c>=2 ) {
          mid_h=mid; ln=0; 
          for( i=8; i<=32; i+=8 ) {
            qword msk = (1ULL<<i)-1;
            qword nsk = ~msk;
            if( ((mid_h|msk)<hai) && ((mid_h&nsk)<mid) && ((mid_h&nsk)>=lowc) ) mid_h|=msk,ln=i;
          }
          if( mln<=ln ) mln=ln, mhai=mid_h;
        }
      } // for 
      lowc = mhai;

      n = NUM-mln/8;
      if( (Cache!=-1) && ((n!=0) || (Cache+Carry!=0xFF) ) ) put( Cache+Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also FFs
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
    }
  }

};

//#include "main.inc"

Rangecoder rc;

//#define RESCALE_64k

int main( int argc, char** argv ) {

  if( argc<3 ) return 1;

  uint f_DEC = (argv[1][0]=='d');
  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  uint i,c,code,low,total=0,freq[256];
  for( i=0; i<256; i++ ) total+=(freq[i]=1);

  if( f_DEC==0 ) rc.StartEncode(g); else rc.StartDecode(f);

  while(1) {

#ifdef RESCALE_64k
    if( total>=(1<<16) ) for( i=0,total=0; i<256; i++ ) total+=(freq[i]=freq[i]/2+1);
#endif

    if( f_DEC==0 ) {
      c=getc(f); if( c==-1 ) break;
      for( i=0,low=0; i<c; i++ ) low+=freq[i];
      rc.rc_Process(low,freq[c],total);
    } else {
      code = rc.rc_GetFreq(total);
      for( c=0,low=0; low+freq[c]<=code; c++ ) low+=freq[c];
      if( (rc.EOFn>0) && (rc.rc_GetFreq1(total)<low) ) break;
      rc.rc_Process(low,freq[c],total);
    }

    if( f_DEC==1 ) putc(c,g);

    freq[c]++; total++;
  }

  if( f_DEC==0 ) rc.rc_Quit( freq, 256 );

  fclose(g);
  fclose(f);

  return 0;
}
