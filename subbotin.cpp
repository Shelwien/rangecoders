
#include <stdio.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

static const uint NUM = 4;
static const uint TOP = 1<<24;
static const uint BOT = 1<<16;

struct Rangecoder {
 uint  low, code, range, passed;
 FILE  *f;
 uint f_DEC;

 void OutByte (byte c)	       { passed++; fputc(c,f); }
 byte InByte () 	       { passed++; return fgetc(f); }

 uint GetPassed ()	       { return passed; }
 void StartEncode (FILE *F)    { f=F; f_DEC=0; passed=low=0;  range= (uint) -1; }
 void FinishEncode ()	       { rc_Quit(); /*for(int _=0; _<NUM; _++) OutByte(low>>24), low<<=8;*/ }
 void StartDecode (FILE *F)    { passed=low=code=0;  range= (uint) -1;
				 f=F; f_DEC=1; for(int _=0; _<NUM; _++) code=(code<<8)|InByte();
			       }

 void rc_Process(uint cumFreq, uint freq, uint totFreq) {
   if( f_DEC==0 ) Encode(cumFreq,freq,totFreq); else Decode(cumFreq,freq,totFreq);
 }

 uint rc_GetFreq (uint totFreq) {
   uint tmp= (code-low) / (range/= totFreq);
   return tmp;					       // a valid value :)
 }

 void Decode (uint cumFreq, uint freq, uint totFreq) {
    low  += cumFreq*range;
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       code= (code<<8) | InByte(), range<<=8, low<<=8;
 }

 void Encode (uint cumFreq, uint freq, uint totFreq) {
    low  += cumFreq * (range/= totFreq);
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       OutByte(low>>24), range<<=8, low<<=8;
 }

 #define put OutByte
 void rc_Quit( void ) {
   if( f_DEC==0 ) {
     uint i, n = NUM;

     // Carry=0: cache   .. FF x FFNum .. low
     // Carry=1: cache+1 .. 00 x FFNum .. low
     qword llow=low;
     qword high=llow+range;
     qword mask=0xFF;
     uint Carry=0, FFNum=0, Cache=-1;

     for( i=0; i<NUM; i++ ) {
       if( (llow|mask)<high ) llow|=mask,n--;
       (mask<<=8)+=0xFF;
     }

     if( (Cache!=-1) && ((n!=0) || (Cache+Carry!=0xFF) ) ) put( Cache+Carry );
     if( (n==0) && (Carry==0) ) FFNum=0; // they're also FFs
     for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
     for( i=0; i<n; i++ ) put( llow>>24 ), llow<<=8;
   }
 }
 #undef put

};

#undef RESCALE_64k
#define RESCALE_64k
#include "main.inc"
