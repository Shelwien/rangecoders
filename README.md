# rangecoders
Various implementations of bytewise rangecoding

https://encode.su/threads/3084-Simple-rangecoder-with-low-redundancy-on-short-strings

Common frontend in *main.inc*, different RC classes in cpp files.

    book1   resc64k  wcc386  resc64k  name     freqbits
    ------- 435,901  ------- 430,266  subbotin  16
    ------- 435,595  ------- 430,070  fp32_rc   16
    435,398 435,581  439,267 430,035  fp64_rc   40
    435,398 435,581  439,267 430,036  sh_v1m    24
    435,398 435,580  439,266 430,036  sh_v2f    31
    435,398 435,581  439,266*430,036* sh_128    56 // wcc386 not decodable; probably a math bug
    435,395 435,575  439,263 430,033  sh_v1n    24

    input size:   0 1 2 3 4 6 7 8 9 A B C  D
    subbotin      2 2 3 4 5 6 6 7 7 8 9 9 10 // Carryless rangecoder by D.Subbotin, used in PPMd
    fp32_rc       0 2 3 4 5 5 6 7 7 8 9 9 10 // RC using float-point arithmetics for high precision
    fp64_rc       0 2 3 4 5 5 6 7 8 8 9 9 10 // as above, but with doubles
    sh_v1m        0 2 3 4 5 5 6 7 8 8 8 9 10 // basic RC with max precision, still 64k minrange/maxtotal
    sh_v2f        0 2 3 4 5 6 5 7 7 8 9 9 10 // more like original AC, with bitwise i/o and range renorm
    sh_128        0 2 3 4 5 5 6 7 7 8 9 9  9 // 128-bit RC originally based on compiler __int128 support, buggy atm
    sh_v1n        0 1 2 3 4 5 5 6 7 7 8 8  9 // sh_v1m with termination aware of filesystem EOF

