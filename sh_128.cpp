
#include <stdio.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;

void H_ADC( qword& Carry, qword& a, qword b ) {
  qword old = a; 
  a += b; if( a<old ) ++Carry;
}

uint H_CMP( qword aH,qword aL, qword bH,qword bL ) {
  uint r = 0;

  if( aH<bH ) r=1; else
  if( aH>bH ) r=-1; else
  /* aH==bH */ 
  if( aL<bL ) r=1; else
  if( aL>bL ) r=-1; else 
  r=0;

  return r;
}

qword multdiv( qword a, qword b, qword c ) {
  const qword base = 1ULL<<32;
  const qword maxdiv = (base-1)*base + (base-1);

  // First get the easy thing
  qword res = (a/c) * b + (a%c) * (b/c);
  a %= c;
  b %= c;
  if( (a==0) || (b==0) ) return res;

  // Is it easy to compute what remain to be added?
  if( c<base ) return res + (a*b/c);

  // Now 0 < a < c, 0 < b < c, c >= 1ULL
  // Normalize
  qword norm = maxdiv/c;
  c *= norm;
  a *= norm;

  // split into 2 digits
  qword ah = a / base, al = a % base;
  qword bh = b / base, bl = b % base;
  qword ch = c / base, cl = c % base;

  // compute the product
  qword p0 = al*bl;
  qword p1 = p0 / base + al*bh;
  p0 %= base;
  qword p2 = p1 / base + ah*bh;
  p1 = (p1 % base) + ah * bl;
  p2 += p1 / base;
  p1 %= base;
  // p2 holds 2 digits, p1 and p0 one

  // first digit is easy, not null only in case of overflow
  qword q2 = p2 / c;
  p2 = p2 % c;

  // second digit, estimate
  qword q1 = p2 / ch;
  // and now adjust
  qword rhat = p2 % ch;

  // the loop can be unrolled, it will be executed at most twice for
  // even bases -- three times for odd one -- due to the normalisation above
  while( (q1>=base) || ((rhat<base) && (q1*cl>rhat*base+p1)) ) { q1--; rhat+=ch; }

  // subtract 
  p1 = ((p2 % base) * base + p1) - q1 * cl;
  p2 = (p2 / base * base + p1 / base) - q1 * ch;
  p1 = p1 % base + (p2 % base) * base;

  // now p1 hold 2 digits, p0 one and p2 is to be ignored
  qword q0 = p1 / ch;
  rhat = p1 % ch;
  while( (q0>=base) || ((rhat<base) && (q0*cl>rhat*base+p0)) ) { q0--; rhat+=ch; }

  // we don't need to do the subtraction (needed only to get the remainder,
  // in which case we have to divide it by norm)
  return res + q0 + q1 * base; // + q2 *base*base
}

//typedef unsigned __int128 hword;
//struct Hword { qword L; qword H; };

qword mulQQ( qword a, qword b ) {
  //Hword r;
  qword CMid=0,CHai=0;
  qword ah = a>>32, al = uint(a);
  qword bh = b>>32, bl = uint(b);

  qword ah_bh = ah*bh; // <<64
  qword al_bl = al*bl; // <<0
  qword al_bh = al*bh; // <<32
  qword ah_bl = ah*bl; // <<32

  H_ADC( CMid, al_bh, ah_bl ); // al*bh+ah*bl with carry in CMid, still <<32

  ah_bl=al_bl; ah_bl>>=32; // high dword of al*bl
  //al_bl = uint(al_bl); // high dword already went to ah_bl

  H_ADC( CMid, al_bh, ah_bl ); // al*bh+ah*bl+(al*bl>>32) with carry in CMid, still <<32

  CMid<<=32; // because al*bh are shifted like that
  CMid |= al_bh>>32; // move high dword there

  //al_bh = uint(al_bh);
  //al_bh<<=32;
  //al_bh |= al_bl; // two low dwords

  H_ADC( CHai, ah_bh, CMid ); // ah*bh + ((al*bh+ah*bl+(al*bl>>32))>>32) in ah_bh

  //r.L = al_bh;
  //r.H = ah_bh;
  //return r;
  return ah_bh;
}

// (hword(a)*b)/(hword(range)+1); 
qword muldivR( qword a, qword b, qword range ) { 
  qword r;
  range++;
  if( range==0 ) {
    //Hword a1 = mulQQ( a, b );
    //r = a1.H;
    r = mulQQ(a,b); // >>64
  } else { 
    r = multdiv( a, b, range );
  }
  return r;
}

// (hword(a)*(hword(range)+1))/c; 
qword mulRdiv( qword a, qword c, qword range ) { 
  qword r;
  range++;

  if( a==c ) r=range; else

  if( range==0 ) {
    // a*(b-c)/c+a = a*b/c
    r = multdiv( a, -c, c ) + a;
  } else {
    r = multdiv( a, range, c );
  }
  return r;
}

struct Rangecoder  {
  uint f_DEC; // 0=encode, 1=decode;
  FILE* f;
  byte get() { return byte(getc(f)); }
  void put( byte c ) { putc(c,f); }
  void StartEncode( FILE *F ) { f=F; f_DEC=0; rc_Init(); }
  void FinishEncode( void ) { rc_Quit(); }
  void StartDecode( FILE *F ) { f=F; f_DEC=1; rc_Init(); }

  static const uint  NUM = 8;
  static const uint  MSByte = 24+32;
  static const qword sTOP  = 0x0100000000000000ULL;
  static const qword Thres = 0xFF00000000000000ULL;

  qword low;
  qword Carry;
  qword code; 
  qword range;

  uint FFNum;
  uint Cache;


  void rc_Process( qword cumFreq, qword freq, qword totFreq ) {
    qword tmp = mulRdiv(cumFreq,totFreq,range); // + 1;
    if( f_DEC ) code-=tmp; else {
      //lowc+=tmp;
      qword old=low; low+=tmp; if( low<old ) Carry++;
    }
    range = mulRdiv(cumFreq+freq,totFreq,range)-1 - tmp;
    if( f_DEC ) 
      while( range<sTOP ) {
        range = (range<<8)+0xFF; // (range+1)*256-1=range*256+255
        uint c=get(); FFNum += (c==-1);
        (code<<=8)+=byte(c);
      }
    else 
      while( range<sTOP ) range=(range<<8)+0xFF, ShiftLow();
  }


  void rc_Arrange( qword totFreq ) {}

  qword rc_GetFreq( qword totFreq ) {
    qword r = muldivR(code,totFreq,range);
    //printf( "GetFreq: %I64X\n", r );
    return r;
  }


  void rc_BProcess( qword freq, int& bit ) { 
    qword x[] = { 0, freq, SCALE };
    if( f_DEC ) {
      qword count = rc_GetFreq( SCALE );
      bit = (count>=freq);
    }
    rc_Process( x[bit], x[1+bit]-x[bit], SCALE );
  }

  void ShiftLow( void ) {
    if( low<Thres || Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for (;FFNum != 0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>MSByte;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }


  void rcInit( void ) { 
    range = -1;
    low   = 0;
    FFNum = 0;
    Carry = 0;    
    Cache = -1;
  }
  
  void rc_Init() {
    rcInit();
    //f_DEC = _f_DEC;
    if( f_DEC==1 ) {
      for(int _=0; _<NUM; _++) (code<<=8)+=get(); 
    }
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword hlow=0,llow=low,tlow;
      qword h_hi=0,l_hi=low; H_ADC(h_hi,l_hi,range); H_ADC(h_hi,l_hi,1);
      qword mask=0xFF;

//printf( "l128=%016I64X%016I64X Carry=%i Cache=%02X FFNum=%i n=%i\n", hlow,llow, uint(Carry),Cache,FFNum, n );
//printf( "h128=%016I64X%016I64X\n", h_hi,l_hi );

      for( i=0; i<NUM; i++ ) {
        tlow = llow|mask;
        if( H_CMP(hlow,tlow,h_hi,l_hi)==1 ) llow|=mask,n--;
        (mask<<=8)+=0xFF;
      }

//printf( "l128=%016I64X%016I64X Carry=%i Cache=%02X FFNum=%i n=%i\n", hlow,llow, uint(Carry),Cache,FFNum, n );

      if( (Cache!=-1) && ((n!=0) || (Cache+Carry!=0xFF) ) ) put( Cache+Carry );
      if( (n==0) && (Carry==0) ) FFNum=0; // they're also FFs
      for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
      for( i=0; i<n; i++ ) put( llow>>MSByte ), llow<<=8;
    }
  }

};

#include "main.inc"
