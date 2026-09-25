#include "Actors.h"

#include <cstdint>
#include <cstdio>

namespace openfranko::src::engine::amal::actors {

int amosBool(bool condition) { return condition ? -1 : 0; }

std::string hex(int value) {
  char digits[16];
  std::snprintf(digits, sizeof(digits), "%X", static_cast<uint32_t>(value));
  return std::string("$") + digits;
}

std::string playerBlood() {
  return "A:P;IRZ=0JA;LY=RB-RZ;IRC=0JB;LR0=16;LX=RA-16;JC;B:LR0=-16;LX=RA+16;"
         "C:A1,(2,2)(3,2)(4,2)(5,2)(6,2)(7,2)(8,2);MR0,RZ,14;A1,(9,1);M0,0,5;"
         "LRZ=0;P;LA=10;JA;";
}

std::string enemyBlood() {
  return "A:P;IRY=0JA;LR5=RY;LR3=RU;LY=R5-R3;LR1=RX;LR4=RT;IR4=0JB;LR0=8;"
         "LX=R1+8;JC;B:LR0=-16;LX=R1-16;C:A1,(2,2)(3,2)(4,2)(5,2)(6,2)(7,2)(8,"
         "2);MR0,R3,R3/6;A1,(9,1);M0,0,5;P;LA=10;LRY=0;JA;";
}

std::string screenShake() {
  return "A:IRM=0JA;FR0=0T2;M0,8,1;M0,-8,2;NR0;LRM=0;JA;";
}

PlayerPrograms streetPlayer(int stage) {
  PlayerPrograms programs;

  std::string a =
      "LR0=0;A:P;P;P;LRB=Y;LR9=J1;IR9=16JH;IR9=18JO;IRC=0JJ;IR9=24JI;IR9=20JN;"
      "IR9=17JP;IR9=26JR;JU;J:IR9=20JI;IR9=24JN;IR9=17JQ;IR9=22JR;U:IR9&8JB;"
      "IR9&4JC;IR9&2JD;IR9&1JE;LA=17+RC;JA;B:IX>288JA;LX=X+6-R1;LRC=0;JL;C:IX<"
      "32JA;LX=X-6+R1;LRC=$8000;JL;";
  a += "D:IY>215JM;LY=Y+4;JF;E:IY<172JM;LY=Y-4;JF;H:LRD=1;LA=RC+20;LRE=1;M0,0,"
       "7;LA=16+RC;M0,0,7;LA=31+RC;LRE=2;M0,0,7;LRD=0;LA=16+RC;M0,0,7;JK;I:LA="
       "30+RC;LRD=3;LRE=2;M0,0,15;LRD=0;LA=17+RC;M0,0,15;JK;N:LA=18+RC;M0,0,10;"
       "LA=22+RC;LRE=1;LRD=6;";
  a += "M0,0,10;LRD=0;LA=11+RC;M0,0,15;JK;O:LRE=2;LRD=2;LA=19+RC;M0,0,5;LA=32+"
       "RC;M0,0,10;LA=19+RC;M0,0,5;LRD=0;LA=17+RC;M0,0,10;JK;P:LA=$8015;LRD=4;"
       "M-8,-12,4;LRE=3;LA=$8021;M-8,-12,4;M-64,0,16;M-16,24,8;LRD=0;LA=$801B;"
       "M0,0,7;JK;Q:LA=21;LRD=4;";
  a += "M8,-12,4;LRE=3;LA=33;M8,-12,4;M64,0,16;M16,24,8;LRD=0;LA=27;M0,0,7;JK;"
       "R:LA=21+RC;M0,-8,4;LRD=5;LA=33+RC;M0,-8,5;LRE=3;LA=$8021+RC;M0,-8,5;LA="
       "33+RC;M0,0,5;LA=$8021+RC;M0,8,5;LA=21+RC;M0,16,4;LRD=0;LA=27+RC;M0,0,"
       "15;JK;M:IR9&8JF;IR9&4JF;JA;";
  a += "L:IR9&2JD;IR9&1JE;F:LR0=R0+1;IR0>11JG;LA=R0/2+RC+11;JA;G:LR0=0;LA=R0+"
       "RC+11;JA;K:LA=11+RC;LRD=0;JA;";
  programs.locomotion = a;

  a = "A:LRA=X;IR1=1JM;IR1=2JN;IR1=3JO;IR5=1JC;IR4=1JB;P;JA;B:LR4=3;LA=27+RC;"
      "M0,0,10;LA=28+RC;M0,0,10;LA=27+RC;M0,0,10;LA=28+RC;M0,0,10;LA=27+RC;M0,"
      "0,10;LA=28+RC;M0,0,10;LR4=9;JA;C:LA=22+RC;M0,0,10;LR9=0;T:LR0=J1;IR0="
      "18JG;IR0=17JH;IR0=1JJ;IR0=2JF;";
  a += "IRC=0JE;IR0=8JI;IR0=24JK;JU;E:IR0=4JI;IR0=20JK;U:P;LR9=R9+1;IR9<29JT;"
       "JD;F:LRV=1;FR9=0T2;LA=23+RC;M0,0,10;LA=24+RC;LRW=8;M0,0,10;NR9;LA=12+"
       "RC;M0,0,5;JL;G:LRV=2;FR9=0T2;LA=22+RC;M0,0,10;LA=29+RC;LRW=9;M0,0,10;"
       "NR9;LA=16+RC;M0,0,5;JL;";
  a += "H:LRV=3;FR9=0T2;LA=25+RC;M0,0,10;LA=26+RC;LRW=10;M0,0,10;NR9;LA=16+RC;"
       "M0,0,5;JL;I:LRV=4;FR9=0T2;LA=27+RC;M0,0,10;LA=34+RC;LRW=8;M0,0,10;NR9;"
       "LA=16+RC;M0,0,5;JL;J:LRV=5;LRD=4;LA=25+RC;M0,0,15;LA=35+RC;M0,0,5;JL;";
  a += "K:LRV=6;LRD=4;LA=25+RC;M0,0,15;LA=36+RC;M0,0,5;JL;L:LRV=0;LR4=9;LR5=0;"
       "LRK=0;LR1=9;LRD=0;JA;D:LA=18+RC;M0,0,15;JL;P:IRF<1JQ;JL;Q:A1,(39+RC,10)"
       "(42+RC,10);M0,0,20;LRM=1;LRW=11;LRF=64;LRG=RG-1;IRG<0JR;M0,0,20;LRW=4;"
       "LA=41+RC;";
  a += "M0,0,10;JL;R:LA=43+RC;LRW=5;M0,0,10;LRG=-2;S:P;JS;M:LRD=9;LRZ=60;LA=38+"
       "RC;M0,0,5;LRW=7;LA=37+RC;LRF=RF-1;M0,0,10;LA=38+RC;M0,0,10;LA=17+RC;M0,"
       "0,7;JP;N:LRD=9;LRW=8;LRZ=60;A1,(39+RC,10)(40+RC,10);MR2,-32,12;MR2,32,"
       "12;M0,0,4;LRF=RF-4;";
  a += "LRM=1;LRW=11;LA=42+RC;LRA=RA+R2+R2;M0,0,20;LRE=4;M0,0,20;LA=41+RC;M0,0,"
       "10;LA=17+RC;M0,0,7;JP;O:LRD=9;LA=38+RC;M0,0,10;LRW=10;LRZ=40;LA=40+RC;"
       "M0,0,9;LA=38+RC;M0,0,10;LRW=7;LRZ=40;LA=40+RC;M0,0,9;LA=38+RC;M0,0,10;"
       "LRW=10;LRZ=40;LA=40+RC;";
  a += "M0,0,9;LRF=RF-4;LA=39+RC;M0,0,10;LA=42+RC;LRE=11;LRM=1;M0,0,20;LRW=4;"
       "LA=41+RC;M0,0,15;LA=17+RC;M0,0,7;JP;";
  programs.damage = a;

  if (stage == 2) {
    programs.clamp = "A:IX<48JB;IX>288JC;JA;B:LX=48;JA;C:LX=288;JA;";
  } else {
    programs.clamp = "A:IX<32JB;IX>272JC;JA;B:LX=32;JA;C:LX=272;JA;";
  }
  return programs;
}

EnemyPrograms enemy(int imageBase, int type) {
  const int d = imageBase;
  const int r = type;
  EnemyPrograms programs;

  std::string a = "A:IR3=1JU;IX<RAJK;IX>RAJL;M:P;IR8=5JH;P;P;";
  if (r == 2) {
    a += "IRD=4JI;";
  }
  a += "IX+32<RAJB;IX-32>RAJC;O:IY>RBJD;IY<RBJE;IR4|R5JF;IRD<>0JA;LA=" +
       hex(48 + d) +
       "+R2;IR8=0JA;LR9=R8;LRJ=1;IR9=1JP;IR9=2JQ;IR9=3JR;P;JA;B:LR4=5;JO;C:LR4="
       "-5;JO;D:LR5=-4;JF;E:LR5=4;JF;";
  a += "F:LR7=0;LX=X+R4;LY=Y+R5;LR4=0;LR5=0;LR0=R0+1;IR0>7JG;LA=R0/2+" +
       hex(44 + d) + "+R2;JA;G:LR0=0;LA=" + hex(44 + d) +
       "+R0+R2;JA;K:IR2=0JM;LR2=0;LA=" + hex(44 + d) +
       "+R0+R2;JM;L:IR2=$8000JM;LR2=$8000;LA=" + hex(44 + d) + "+R0+R2;JM;";
  a += "P:LA=" + hex(52 + d) + "+R2;M0,0,10;LA=" + hex(47 + d) +
       "+R2;M0,0,5;JT;Q:LA=" + hex(49 + d) + "+R2;M0,0,5;";
  if (r < 2) {
    a +=
        "LA=" + hex(53 + d) + "+R2;M0,0,7;LA=" + hex(49 + d) + "+R2;M0,0,5;JT;";
  } else {
    a += "LA=" + hex(45 + d) + "+$8000+R2;M0,0,5;LA=" + hex(53 + d) +
         "+R2;M0,0,15;LA=" + hex(63 + d) +
         "+R2;LRW=" + hex(12 - 3 * amosBool(d == 25) - 6 * amosBool(d == 50)) +
         ";M0,0,40;JT;";
  }
  a += "R:LA=" + hex(50 + d) + "+R2;M0,0,10;LA=" + hex(51 + d) +
       "+R2;M0,0,10;LRJ=0;LA=" + hex(50 + d) + "+R2;M0,0,10;LA=" + hex(51 + d) +
       "+R2;M0,0,10;LA=" + hex(50 + d) + "+R2;M0,0,10;LA=" + hex(51 + d) +
       "+R2;M0,0,10;LA=" + hex(50 + d) + "+R2;M0,0,10;JT;";
  a += "T:LR1=0;LRJ=0;LA=" + hex(48 + d) +
       "+R2;LR9=0;P;JA;U:IR2=0JV;LR6=-16;JW;V:LR6=16;W:A2,(" + hex(44 + d) +
       "+R2,13)(" + hex(45 + d) + "+R2,13)(" + hex(46 + d) + "+R2,13)(" +
       hex(47 + d) + "+R2,13);MR6,16,16;MR6*4,0,72;MR6,-16,16;LR3=0;JT;";
  a += "H:LRW=" + hex(14 - 3 * amosBool(d == 25) - 6 * amosBool(d == 50)) +
       ";P;JA;";
  if (r == 2) {
    a += "I:LR1=1;LA=" + hex(66 + d) + "+R2;M0,0,60;JT;";
  }
  programs.walk = a;

  const std::string b = ";LRT=R1;LRX=X;LRY=Y;";

  a = "A:P;IR0=0JA;LR5=1;IR0=1JB;IR0=2JC;IR0=3JD;IR0=4JE;IR0=5JE;IR0=6JF;P;JA;"
      "B:LR7=R7-3;LRW=7;LA=" +
      hex(54 + d) + "+R1;M0,0,7;LA=" + hex(55 + d) + "+R1;LRU=60" + b +
      "M0,0,7;LRW=8;LA=" + hex(54 + d) + "+R1;M0,0,7;JT;";
  a += "C:LR7=R7-2;LRW=9;LA=" + hex(61 + d) + "+R1;M0,0,8;LA=" + hex(62 + d) +
       "+R1;LRU=20" + b + "M0,0,8;LA=" + hex(66 + d) + "+R1;M0,0,10;JT;";
  a += "D:LRW=10;LA=" + hex(57 + d) + "+R1;M0,0,25;LA=" + hex(59 + d) +
       "+R1;M0,0,25;LA=" + hex(57 + d) + "+R1;M0,0,20;LA=" + hex(44 + d) +
       "+R1;M0,0,5;JT;";
  a += "E:LR7=R7-6;LRW=9;LRU=100" + b + "A1,(" + hex(61 + d) + "+R1,8)(" +
       hex(62 + d) + "+R1,8)(" + hex(67 + d) +
       "+R1,8);MR3*2,-32,12;MR3*2,32,12;LRM=1;LRW=11;JQ;";
  a += "G:LRL=1;LR9=-16;IR1=0JP;LR9=16;P:FR6=0T2;LRW=10;LA=" + hex(64 + d) +
       "+R1;M0,0,10;LRW=8;LA=" + hex(65 + d) + "+R1" +
       ";LRU=35;LRT=R1;LRX=X+R9;LRY=Y;M0,0,10;NR6;LR4=9;LRL=0;JU;";
  a += "F:LA=" + hex(57 + d) + "+R1;M0,0,5;LA=" + hex(56 + d) +
       "+R1;M0,0,4;LR9=0;O:IRV=1JH;IRV=2JH;IRV=3JJ;IRV=4JK;IRV=5JL;IRV=6JM;";
  a += "P;LR9=R9+1;IR9<29JO;JI;H:LR7=R7-10;FR9=0T2;LA=" + hex(57 + d) +
       "+R1;M0,0,10;LA=" + hex(58 + d) + "+R1;LRU=50" + b +
       "M0,0,10;NR9;LA=" + hex(62 + d) + "+R1;MR3,-16,8;MR3,16,8;";
  if (r < 2) {
    a += "IR7<1JQ;LA=" + hex(67 + d) + "+R1;LRM=1;LRW=11;M0,0,20;JY;";
  } else {
    a += "JY;";
  }
  a += "J:LR7=R7-12;FR9=0T2;LA=" + hex(56 + d) +
       "+R1;M0,0,10;LA=" + hex(57 + d) + "+R1;LRU=70" + b +
       "M0,0,10;NR9;LA=" + hex(62 + d) + "+R1;M0,0,10;";
  if (r < 2) {
    a += "IR7<1JQ;LA=" + hex(67 + d) + "+R1;LRM=1;LRW=11;M0,0,10;JQ;";
  } else {
    a += "JY;";
  }
  a += "K:LR7=R7-14;LX=X+R6;FR9=0T2;LA=" + hex(59 + d) +
       "+R1;M0,0,10;LA=" + hex(60 + d) + "+R1;LRU=50" + b +
       "M0,0,10;NR9;LA=" + hex(62 + d) + "+R1;M0,0,10;";
  if (r < 2) {
    a += "IR7<1JQ;LA=" + hex(67 + d) + "+R1;LRM=1;LRW=11;M0,0,20;JY;";
  } else {
    a += "JY;";
  }
  a += "L:LA=" + hex(61 + d) +
       "+R1;LY=Y-16;M0,0,15;LRE=6;LR9=R6*3;LX=X+R9;LY=Y-72;LR3=30000;LA=" +
       hex(64 + d) + "+R1+$4000;M2*R6,-16,12;LA=" + hex(58 + d) +
       "+R1+$C000;M2*R6,16,12;LY=Y+56;";
  a += "LA=" + hex(67 + d) +
       "+R1;MR6,32,12;LR3=0;LRM=1;MR6/2,-8,4;LRW=11;MR6/2,8,4;LR7=R7-20;LRW="
       "11;JQ;";
  a += "M:LA=" + hex(61 + d) +
       "+R1;LY=Y-8;M0,0,15;LRE=6;LR9=R6*3;LX=X+R9;LY=Y-48;LR3=30000;LA=" +
       hex(67 + d) +
       "+R1;M2*R6,-16,12;M2*R6,16,12;MR6,56,16;LR3=0;LRM=1;MR6/2,-8,4;LRW=11;"
       "MR6/2,8,4;LR7=R7-22;LRW=11;JQ;I:LRW=2;LA=" +
       hex(50 + d) + "+R1;M0,0,10;JT;";
  a += "T:IR7<1JS;LR0=0;LR2=2;LR5=0;LA=" + hex(48 + d) +
       "+R1;JA;Q:IR7<1JU;LR4=1;LR9=0;R:IR4=2JG;P;LR9=R9+1;IR9<100JR;LR4=0;LA=" +
       hex(66 + d) + "+R1;M0,0,10;JT;";
  a += "S:IR1=0JV;LR9=16;JN;V:LR9=-16;N:A1,(" + hex(61 + d) + "+R1,5)(" +
       hex(62 + d) + "+R1,10)(" + hex(67 + d) +
       "+R1,5);MR9,-16,10;LRE=11;MR9,16,10;JU;";
  if (r < 2) {
    a += "Y:LA=" + hex(66 + d) + "+R1;M0,0,10;LA=" + hex(63 + d) +
         "+R1;LRU=" + hex(55 - 24 * r) + ";LRT=$8000-R1;LRX=X;LRY=Y;LRW=" +
         hex(12 - 3 * amosBool(d == 25) - 6 * amosBool(d == 50)) +
         ";M0,0,70;LA=" + hex(59 + d) + "+R1;M0,0,10;LRE=11;LA=" + hex(67 + d) +
         "+R1;JQ;";
  } else {
    a += "Y:LRE=11;LA=" + hex(67 + d) + "+R1;LRM=1;JQ;";
  }
  a += "U:LRE=" + hex(13 - 3 * amosBool(d == 25) - 6 * amosBool(d == 50)) +
       ";LA=" + hex(68 + d) + "+R1;LR6=X;LR7=Y;LR9=1000+" + hex(68 + d) +
       ";LRM=1;M0,0,10;LRN=RN+1;LRI=RI-1;LR9=0;LR3=R6-30;LA=10;LX=160;LY=24;"
       "IR1=0JX;LR3=R6+30;X:LRX=R3;LRU=1;LRT=R1;LRY=R7;M0,0,10;LR9=R9+1;IR9<"
       "8JX;";
  programs.damage = a;
  return programs;
}

std::string idle() { return "A:P;JA;"; }

std::string indicatorArrow(int facing) {
  return "A0,(1+" + hex(facing) + ",10)(10,10);";
}

PlayerPrograms bossPlayer(int stage) {
  const int mirrored = amosBool(stage == 2);
  PlayerPrograms programs;

  std::string a =
      "LR0=0;A:P;P;P;LRB=Y;LR9=J1;IR9=16JH;IR9=18JO;IRC=0JJ;IR9=20JN;IR9=17JP;"
      "IR9=26JR;JU;J:IR9=24JN;IR9=17JQ;IR9=22JR;";
  a += "U:IR9&8JB;IR9&4JC;IR9&2JD;IR9&1JE;LA=17+RC;JA;B:IX>" +
       hex(288 - 16 * mirrored) + "JA;LX=X+6-R1;LRC=0;JL;C:IX<" +
       hex(32 - 16 * mirrored) +
       "JA;LX=X-6+R1;LRC=$8000;JL;D:IY>215JM;LY=Y+4;JF;E:IY<R2JM;LY=Y-4;JF;";
  a += "H:LRD=1;LA=RC+19;LRE=1;M0,0,7;LA=16+RC;M0,0,7;LA=29+RC;LRE=2;M0,0,7;"
       "LRD=0;LA=16+RC;M0,0,7;JK;N:LA=18+RC;M0,0,10;LA=23+RC;LRE=1;LRD=6;M0,0,"
       "10;LRD=0;LA=11+RC;M0,0,15;JK;";
  a += "O:LRE=2;LA=21+RC;M0,0,5;LRD=2;LA=25+RC;M0,0,5;LA=22+RC;M0,0,10;LRD=0;"
       "LA=17+RC;M0,0,10;JK;P:LA=$8014;M-8,-12,4;LRE=3;LA=$801E;LRD=4;M-8,-12,"
       "4;M-64,0,16;M-16,24,8;LRD=0;LA=$8018;M0,0,15;JK;Q:LA=20;M8,-12,4;LRE="
       "3;LA=30;";
  a += "LRD=4;M8,-12,4;M64,0,16;M16,24,8;LRD=0;LA=24;M0,0,15;JK;R:LA=20+RC;M0,"
       "-8,4;LRD=5;LA=30+RC;M0,-8,5;LRE=3;LA=$801E+RC;M0,-8,5;LA=30+RC;M0,0,5;"
       "LA=$801E+RC;M0,8,5;LA=20+RC;M0,16,4;LRD=0;LA=24+RC;M0,0,20;JK;M:IR9&"
       "8JF;IR9&4JF;JA;";
  a += "L:IR9&2JD;IR9&1JE;F:LR0=R0+1;IR0>11JG;LA=R0/2+RC+11;JA;G:LR0=0;LA=R0+"
       "RC+11;JA;K:LA=11+RC;LRD=0;JA;";
  programs.locomotion = a;

  a = "A:P;LRA=X;IR1=1JM;IR1=2JN;IR1=3JJ;IR1=4JK;IR1=5JO;IR5=1JC;IR4=1JB;JA;B:"
      "LR4=3;LA=27+RC;M0,0,10;LA=28+RC;M0,0,10;LA=27+RC;M0,0,10;LA=28+RC;M0,0,"
      "10;LA=27+RC;M0,0,10;LA=28+RC;M0,0,10;LR4=9;JA;C:LA=23+RC;M0,0,10;LR9=0;"
      "T:LR0=J1;IR0=18JG;IR0=17JH;";
  a += "IRC=0JE;IR0=8JI;JU;E:IR0=4JI;U:P;LR9=R9+1;IR9<29JT;JD;G:LRV=2;LA=23+RC;"
       "M0,0,10;LA=28+RC;M0,0,10;LA=23+RC;M0,0,10;LA=28+RC;M0,0,10;LA=23+RC;M0,"
       "0,10;LA=28+RC;M0,0,10;LA=16+RC;M0,0,10;JL;H:LRV=3;LA=26+RC;M0,0,10;LA="
       "27+RC;M0,0,10;LA=26+RC;";
  a += "M0,0,10;LA=27+RC;M0,0,10;LA=26+RC;M0,0,10;LA=27+RC;M0,0,10;LA=16+RC;M0,"
       "0,10;JL;I:LRV=4;LA=24+RC;M0,0,10;LA=31+RC;M0,0,10;LA=24+RC;M0,0,10;LA="
       "31+RC;M0,0,10;LA=24+RC;M0,0,10;LA=31+RC;M0,0,10;LA=16+RC;M0,0,10;JL;L:"
       "LRV=0;LR5=0;LRK=0;LR7=0;";
  a += "LRD=0;LR1=9;JA;D:LA=18+RC;M0,0,15;JL;P:IRF<1JQ;JL;Q:LA=34+RC;M0,0,10;"
       "LA=41+RC;LRM=1;LRW=8;M0,0,10;LRF=64;LRG=RG-1;IRG<0JR;M0,0,80;LRW=4;M0,"
       "0,20;LA=40+RC;M0,0,10;JL;R:LA=42+RC;LRW=5;LRM=1;M0,0,10;LRG=-2;S:JS;M:"
       "LRD=9;LRZ=60;LA=33+RC;";
  a += "MR2,-16,12;LRW=6;LA=34+RC;MR2,16,12;LA=41+RC;LRM=1;LRW=8;M0,0,50;LA=40+"
       "RC;LRF=RF-2;M0,0,20;LA=17+RC;M0,0,15;JP;N:LRD=9;LRZ=60;LA=33+RC;M0,0,"
       "10;LA=32+RC;LRW=7;M0,0,10;LRF=RF-3;LA=33+RC;M0,0,20;LA=17+RC;M0,0,15;"
       "JP;";
  a += "J:LRD=9;LA=33+RC;M0,0,20;LRW=7;LA=32+RC;LRZ=70;M0,0,10;LA=33+RC;M0,0,"
       "20;LRW=7;LA=32+RC;LRZ=70;M0,0,10;LA=33+RC;M0,0,20;LRW=7;LA=32+RC;LRZ="
       "70;M0,0,10;LRF=RF-8;JV;K:LRD=9;LA=33+RC;M0,0,20;LRW=6;LA=35+RC;LRZ=50;"
       "M0,0,10;";
  a += "LA=36+RC;M0,0,20;LRW=6;LA=35+RC;LRZ=50;M0,0,10;LA=36+RC;M0,0,20;LRW=6;"
       "LA=35+RC;LRZ=50;M0,0,10;LRF=RF-6;JV;O:LRD=9;LA=33+RC;M0,0,20;LA=35+RC;"
       "M0,0,20;LA=37+RC;LRW=9;LRM=1;LRZ=40;M0,0,150;LA=40+RC;M0,0,15;LRF=RF-"
       "10;JP;";
  a += "V:LA=34+RC;M0,0,10;LA=41+RC;LRW=8;LRM=1;M0,0,50;LA=40+RC;M0,0,15;JP;";
  programs.damage = a;

  if (stage == 2) {
    programs.clamp = "A:IX<R0JB;IX>288JC;JA;B:LX=R0;JA;C:LX=288;JA;";
  } else {
    programs.clamp = "A:IX<32JB;IX>R0JC;JA;B:LX=32;JA;C:LX=R0;JA;";
  }
  return programs;
}

EnemyPrograms boss(int stage) {
  EnemyPrograms programs;

  std::string a;
  if (stage == 1) {
    a = "LX=470;LY=108;LA=78;X:P;IRX=2JY;IRX=0JX;LX=X-8;LRX=0;JX;Y:M0,0,100;LA="
        "79;M0,0,10;LA=80;M0,0,10;LRX=3;LA=74+$8000;LRW=15;M0,0,20;";
  }
  if (stage == 2) {
    a = "LX=-144;LY=124;A10,(81,15)(82,15)(83,15)(84,15)(85,15);X:P;IRX=2JY;"
        "IRX=0JX;LX=X+8;LRX=0;JX;Y:LA=85;M0,0,100;LRX=3;A1,(74,1);LRW=15;M0,0,"
        "20;";
  }
  if (stage == 3) {
    a = "LX=512;LY=200;A0,(78,15)(79,15);X:P;IRX=2JY;IRX=0JX;LX=X-8;LRU=1;LRX="
        "0;JX;Y:A1,(74,1);";
  }
  a += "A:P;IR9=3JR;IR9=4JS;IR9=5JU;IR9=1JP;IR9=2JQ;IR1=1JI;IR1=2JV;IR1=3JH;IR1"
       "=4JW;IX<RAJK;IX>RAJL;M:P;P;P;IX+32<RAJB;IX-32>RAJC;O:IY>RBJD;IY<RBJE;"
       "IR4|R5JF;LA=47+R2;IR9=0JA;IRD<>0JA;P;JA;B:LR4=8;JO;C:LR4=-8;JO;D:LR5=-"
       "4;JF;E:LR5=4;JF;";
  a += "F:LR7=0;LX=X+R4;LY=Y+R5;LR4=0;LR5=0;LR0=R0+1;IR0>7JG;LA=R0/2+43+R2;JA;"
       "G:LR0=0;LA=43+R0+R2;JA;K:IR2=0JM;LR2=0;LA=43+R0+R2;JM;L:IR2=$8000JM;"
       "LR2=$8000;LA=43+R0+R2;JM;P:LR8=1;LA=48+R2;M0,0,10;LA=50+R2;M0,0,10;LA="
       "48+R2;M0,0,10;JT;";
  a += "Q:LR8=2;LA=49+R2;M0,0,10;LA=57+R2;M0,0,15;LA=49+R2;M0,0,20;JT;R:LR8=3;"
       "LA=52+R2;M0,0,20;LA=51+R2;M0,0,10;LA=52+R2;M0,0,20;LA=51+R2;M0,0,10;"
       "LA=52+R2;M0,0,20;LA=51+R2;M0,0,10;JT;";
  a += "S:LR8=4;LA=52+R2;M0,0,20;LA=54+R2;M0,0,10;LA=53+R2;M0,0,20;LA=54+R2;M0,"
       "0,10;LA=53+R2;M0,0,20;LA=54+R2;M0,0,10;LA=53+R2;M0,0,10;JT;";
  a += "U:LR8=5;LA=52+R2;M0,0,20;A1,(53+R2,10)(55+R2,10)(56+R2,1);M0,-48,12;M0,"
       "48,12;M0,0,20;LA=53+R2;M0,0,10;LA=74+R2;LRE=15;M0,0,20;JT;T:LR7=0;LRJ="
       "0;LA=43+R2;LR8=0;LR9=0;P;JA;";
  a += "W:A2,(43+R2,13)(44+R2,13)(45+R2,13)(46+R2,13);MR6,16,16;MR6*4,0,72;MR6,"
       "-16,16;LR1=0;JT;I:LA=" +
       hex(56 - 5 * amosBool(stage == 2)) +
       "+R2;M0,0,60;LR1=0;JT;V:LA=72+R2;M0,0,10;LRW=14;LA=73+R2;M0,0,20;LA=72+"
       "R2;M0,0,10;LR1=0;JT;";
  a += "H:LA=70+R2;M0,0,10;LRW=13;LA=71+R2;M0,0,20;LA=70+R2;M0,0,10;LR1=0;JT;";
  programs.walk = a;

  const std::string b = ";LRT=$8000-R1;LRX=X;LRY=Y;";

  a = "A:P;IR0=0JA;LR5=1;IR0=1JB;IR0=2JC;IR0=4JE;IR0=5JE;IR0=6JF;P;JA;B:LR7=R7-"
      "4;LA=66+R1;LRW=11;M0,0,7;LA=67+R1;LRU=80" +
      b + "M0,0,7;LRW=11;LA=66+R1;M0,0,7;JT;";
  a += "C:LR7=R7-1;LA=68+R1;LRW=9;MR3,0,4;LA=69+R1;M0,0,10;LA=76+R1;LRW=8;LRM="
       "1;M0,0,40;LA=75+R1;M0,0,40;JT;E:LR7=R7-20;LRU=90" +
       b + "LRW=12;A1,(58+R1,10)(59+R1,15)(76+R1,7);MR3,-64,18;MR3,64,18;JQ;";
  a += "F:LA=60+R1;M0,0,5;LA=63+R1;M0,0,4;LR9=0;O:IRV=2JH;IRV=3JJ;IRV=4JK;P;LR9"
       "=R9+1;IR9<29JO;JI;H:LR7=R7-10;FR9=0T2;LA=61+R1;M0,0,10;LA=62+R1;LRW=10;"
       "LRU=60" +
       b + "M0,0,10;NR9;LA=69+R1;MR3,-16,8;MR3,16,8;JY;";
  a += "J:LR7=R7-12;FR9=0T2;LA=58+R1;M0,0,10;LA=60+R1;LRW=11;M0,0,10;LRU=80" +
       b +
       "NR9;LRY=Y;LA=68+R1;M0,0,10;LA=69+R1;M0,0,10;JY;K:LR7=R7-14;LX=X+R6;"
       "FR9=0T2;LA=64+R1;M0,0,10;LA=65+R1;LRW=12;LRU=60" +
       b + "M0,0,10;NR9;LA=64+R1;M0,0,10;LRY=Y;JY;";
  a += "I:M0,0,2;LRW=15;LA=57+R1;M0,0,10;JT;T:IR7<1JS;LR0=0;LR2=2;LR5=0;LA=46+"
       "R1;JA;Q:IR7<1JU;LRW=8;LRM=1;M0,0,100;LA=75+R1;M0,0,20;JT;S:IR1=0JV;LR9"
       "=16;JN;V:LR9=-16;N:A1,(58+R1,5)(68+R1,10)(69+R1,5);MR9,-16,10;LRW=8;"
       "MR9,16,10;JU;";
  a += "Y:LRW=8;LA=76+R1;JQ;U:LRW=16;LA=77+R1;M0,0,150;LRN=RN+1;LRI=RI-1;LR8=1;"
       "X:";
  programs.damage = a;
  return programs;
}

std::string spectator(int stage) {
  if (stage == 1) {
    return "LX=176;LY=108;LA=10;A:P;IRX=3JB;JA;B:LA=81;M0,0,10;LA=82;M0,0,10;P;"
           "JB;";
  }
  if (stage == 3) {
    return "LX=470;LY=48;A0,(86,45)(87,45);X:P;IRX=2JY;IRU=0JX;LRU=0;LX=X-8;JX;"
           "Y:M0,0,10;";
  }
  return "";
}

DialoguePrograms dialogue(int stage) {
  DialoguePrograms programs;
  if (stage == 1) {
    programs.player =
        "A:P;IRT=2JB;JA;B:LY=RB;LX=RA;LA=92;M0,0,50;C:P;IJ1<>16JC;LRT=3;D:P;"
        "IRT<>4JD;LA=94;M0,0,50;E:P;IJ1<>16JE;LRT=99;LA=10;";
    programs.boss =
        "A:P;IRT=0JA;LA=91;LY=40;LX=176;M0,0,50;B:P;IJ1<>16JB;LRT=2;D:P;IRT<>"
        "3JD;LA=93;M0,0,50;E:P;IJ1<>16JE;LRT=4;F:P;IRT<>99JF;LA=10;";
  }
  if (stage == 2) {
    programs.player =
        "A:P;IRT=2JB;JA;B:LY=RB;LX=RA;LA=91;M0,0,50;C:P;IJ1<>16JC;LRT=3;D:P;"
        "IRT<>4JD;LA=93;M0,0,50;E:P;IJ1<>16JE;LRT=5;F:P;IRT<>6JF;LA=95;M0,0,"
        "50;G:P;IJ1<>16JG;LRT=99;LA=10";
    programs.boss =
        "A:P;IRT=0JA;LY=64;LX=150;LA=90;M0,0,50;B:P;IJ1<>16JB;LRT=2;D:P;IRT<>"
        "3JD;LA=92;M0,0,50;E:P;IJ1<>16JE;LRT=4;F:P;IRT<>5JF;LA=94;M0,0,50;G:P;"
        "IJ1<>16JG;LRT=6;H:P;IRT<>99JH;LA=10";
  }
  if (stage == 3) {
    programs.player =
        "A:P;IRT=0JA;LY=RB;LX=RA;LA=88;M0,0,50;B:P;IJ1<>16JB;LRT=2;D:P;IRT<>"
        "3JD;LA=90;M0,0,50;E:P;IJ1<>16JE;LRT=4;F:P;IRT<>5JF;LA=92;M0,0,50;G:P;"
        "IJ1<>16JG;LRT=99;LA=10";
    programs.boss =
        "A:P;IRT=2JB;JA;B:LY=100;LX=200;LA=89;M0,0,50;C:P;IJ1<>16JC;LRT=3;D:P;"
        "IRT<>4JD;LA=91;M0,0,50;E:P;IJ1<>16JE;LRT=5;G:P;IJ1<>16JG;H:P;IRT<>"
        "99JH;LRT=99;LA=10";
  }
  return programs;
}

std::string walkToBoss() {
  return "A0,($800B+RR,5)($800C+RR,5)($800D+RR,5)($800E+RR,5)($800F+RR,5)($801"
         "0+RR,5);MRU,RT,RS;";
}

std::string finishingPose() { return "A5,(38+RR,10)(39+RR,10);M0,0,1;"; }

std::string finishingBlood() {
  return "A1,(83+RR,5)(84+RR,5)(85+RR,5);M0,0,20;A50,(86+RR,5)(87+RR,5)(88+RR,"
         "5)(89+RR,5);";
}

std::string finishingPoseBack() { return "A5,(39+RR,10)(38+RR,10);"; }

std::string walkOff() {
  return "A0,(11+RC,5)(12+RC,5)(13+RC,5)(14+RC,5)(15+RC,5)(16+RC,5);MRT,0,RU;";
}

std::string bossThrown() {
  return "M0,0,100;LA=78+RR;M0,0,50;LA=79+RR;M0,0,20;LA=80+RR;M0,0,30;LA=77+"
         "RR;";
}

std::string victoryLift() {
  return "LA=38+RR;M0,0,50;LA=39+RR;M0,0,50;LA=86+RR+RT;M0,0,50;LA=87+RR+RT;"
         "M0,0,50;LA=38+RR;";
}

std::string bossRests() {
  return "LA=75+RR;LR0=0;A:P;IR0=0JA;A0,(43+RR,5)(44+RR,5)(45+RR,5)(46+RR,5);"
         "MRU,RS,RT;";
}

std::string bubbleUntilFire() { return "A:P;IJ1<>16JA;LA=10;"; }

std::string walkAway() { return "FR0=3T24;LA=R0;LY=Y+1;M0,0,20;P;NR0;LA=26;"; }

std::string breakDance() {
  const std::string walk = "A6,(4,10)(5,10)(6,10)(7,10)(8,10)(9,10);";
  const std::string spin = "A12,($8012,10)($8013,10)($8014,10)($8015,10)($8016,"
                           "10)($8017,10);";
  const std::string shuffle = "A4,(11,1)(12,1)(13,1)(14,1)(15,1)(16,1);";
  std::string a = "A6,($8004,10)($8005,10)($8006,10)($8007,10)($8008,10)($8009,"
                  "10);M-320,0,320;A1,($800A,1);M0,0,80;A1,($8011,1);M0,0,20;" +
                  walk + "M320,0,320;A1,(10,1);" + spin +
                  "M-340,0,640;A1,(10,1);";
  a += walk +
       "M400,0,400;A1,(10,1);A12,($800B,10)($800C,10)($800D,10)($800E,10)($"
       "800F,10)($8010,10);M-200,0,400;A1,(10,1);" +
       walk + "M140,0,140;A1,(10,1);" + spin +
       "M-330,0,660;A1,(10,1);M0,0,20;LA=17;M0,0,380;";
  a += "A1,(17,50)(11,50);A 4,(11,1)(12,1)(13,1)(14,1)(15,1)(16,1);M32,0,32;" +
       shuffle + "M-16,0,16;" + shuffle + "M16,0,16;" + shuffle + "M-16,0,16;" +
       shuffle +
       "M16,0,16;A1,(4,10)(5,10)(6,10)(7,10)(8,10)(9,10);M64,0,64;A1,(17,1);";
  return a;
}

std::string portraitEntrance(int portrait) {
  switch (portrait) {
  case 1:
    return "M0,0,800;M-310,0,580;M0,0,2080;M320,0,64;";
  case 2:
    return "M0,0,1600;M-310,0,580;M0,0,1250;M320,0,64;";
  case 3:
    return "M0,0,2400;M-310,0,580;M0,0,430;M320,0,64;";
  default:
    return "";
  }
}

std::string danceFinale() {
  std::string a =
      "A0,(27,10)(28,10)(29,10)(31,10)(29,10)(30,10)(31,10)(32,10)(33,10)(34,"
      "10)($801D,10)($801F,10)($801D,10)($801E,10)($801F,10)($8020,10)($8021,"
      "10)($8022,10)(34,10)(35,10)(36,10)(37,10)(38,10)(40,10)(41,10)($8028,"
      "10)($8029,10)(40,10)(41,10)";
  a += "($8028,10)($8029,10)(40,10)(41,10)($8028,10)($8029,10)(40,10)(41,10)($"
       "8028,10)($8029,10)(42,10)(38,10)(39,10)(35,10)(34,10)(24,10)(25,10)(26,"
       "10)(25,10)(24,10)(17,10)($8018,10)($8019,10)($801A,10)($8019,10)($8018,"
       "10)($8011,10);M0,0,1000;";
  return a;
}

std::string portraitShuttle(int portrait) {
  switch (portrait) {
  case 1:
    return "LX=400;M-528,0,528;A:M448,0,448;M-448,0,448;JA;";
  case 2:
    return "LX=550;M-678,0,678;A:M448,0,448;M-448,0,448;JA;";
  case 3:
    return "LX=700;M-828,0,828;A:M448,0,448;M-448,0,448;JA;";
  default:
    return "";
  }
}

std::string pointingHand() {
  return "A:P;IR1=0JA;FR0=0T3;M4,0,2;M-4,0,2;NR0;LR1=0;JA;";
}

std::string pedestrian(int image) {
  return "LR3=A+3;LR2=A;FR0=0T50;LX=X-RT;LA=A+1;IA<R3JA;LA=R2;A:P;LY=Y+4;FR1="
         "1T10;IR4=1JB;NR1;NR0;JD;B:A1,(" +
         hex(image + 3) + ",10)(" + hex(image + 4) +
         ",10);C:P;LX=X-RU;M0,0,7;IX>-80JC;D:";
}

std::string carDriveOff() { return "A0,(1,10)(2,10);M800,0,400;"; }

} // namespace openfranko::src::engine::amal::actors
