
Rangecoder rc;

int main( int argc, char** argv ) {

  if( argc<3 ) return 1;

  uint f_DEC = (argv[1][0]=='d');
  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  uint i,c,code,low,total=0,freq[1+256];
  for( i=0; i<1+256; i++ ) total+=(freq[i]=1);

  if( f_DEC==0 ) rc.StartEncode(g); else rc.StartDecode(f);

  while(1) {

#ifdef RESCALE_64k
    if( total>=(1<<16) ) for( i=0,total=0; i<1+256; i++ ) total+=(freq[i]=freq[i]/2+1);
#endif

    if( f_DEC==0 ) {
      c=getc(f); c=(c==-1)?256:c;
      for( i=0,low=0; i<c; i++ ) low+=freq[i];
      rc.rc_Process(low,freq[c],total);
    } else {
      code = rc.rc_GetFreq(total);
      for( c=0,low=0; low+freq[c]<=code; c++ ) low+=freq[c];
      rc.rc_Process(low,freq[c],total);
    }

    if( c==256 ) break;

    if( f_DEC==1 ) putc(c,g);

    freq[c]++; total++;
  }

  if( f_DEC==0 ) rc.FinishEncode();

  fclose(g);
  fclose(f);

  return 0;
}
