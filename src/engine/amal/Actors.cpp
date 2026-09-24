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

} // namespace openfranko::src::engine::amal::actors
