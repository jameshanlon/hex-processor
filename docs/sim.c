#include "stdio.h"
#define true      1
#define false    0
#define i_ldam   0x0
#define i_ldbm   0x1
#define i_stam   0x2
#define i_ldac   0x3
#define i_ldbc   0x4
#define i_ldap   0x5
#define i_ldai   0x6
#define i_ldbi   0x7
#define i_stai   0x8
#define i_br     0x9
#define i_brz    0xA
#define i_brn    0xB
#define i_opr    0xC
#define i_pfix   0xD
#define i_nfix   0xE
#define o_add    0x0
#define o_sub    0x1
#define o_brb    0x2
#define o_svc    0x3

FILE *codefile;
FILE *simio[8];
char connected[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int mem[200000];
unsigned char *pmem = (unsigned char *) mem;
unsigned int pc;
unsigned int sp;
unsigned int areg;
unsigned int breg;
unsigned int oreg;
unsigned int inst;
unsigned int running;

inbin(d)
{ int lowbits;
  int highbits;
  lowbits = fgetc(codefile);
  highbits = fgetc(codefile);
  return (highbits << 8) | lowbits;
};

load()
{ int low;
  int length;
  int n = 0;
  codefile = fopen("a.bin", "rb");
  while(!feof(codefile)) {
    pmem[n] = fgetc(codefile);
    n++;
  }
  printf("Loaded %d bytes\n", n);
};

simout(b, s)
{ char fname[] = {'s', 'i', 'm', ' ', 0};
  int f;
  if (s < 256)
    putchar(b);
  else
  { f = (s >> 8) & 7;
    if (! connected[f])
    { fname[3] = f + '0';
       simio[f] = fopen(fname, "w");
       connected[f] = true;
      };
      fputc(b, simio[f]);
  };
};

simin(s)
{ char fname[] = {'s', 'i', 'm', ' ', 0};
  int f;
  if (s < 256)
    return getchar();
  else
  { f = (s >> 8) & 7;
    fname[3] = f + '0';
    if (! connected[f])
    { simio[f] = fopen(fname, "r");
      connected[f] = true;
    };
    return fgetc(simio[f]);
  };
};

svc()
{ sp = mem[1];
  switch (areg)
  { case 0: /*running = false;*/ break;
    case 1: simout(mem[sp + 2], mem[sp + 3]); break;
    case 2: mem[sp + 1] = simin(mem[sp + 2]) & 0xFF; break;
  }
}

main()
{ load();
  running = true; oreg = 0; pc = 0;
  unsigned cycles = 0;
  while (running)
  {
    inst = pmem[pc];
    pc = pc + 1;
    oreg = oreg | (inst & 0xf);
    printf("%d %x %d\n", cycles++, inst, (inst >> 4) & 0xf);
    switch ((inst >> 4) & 0xf)
    {
      case i_ldam: areg = mem[oreg]; oreg = 0; break;
      case i_ldbm: breg = mem[oreg]; oreg = 0; break;
      case i_stam: mem[oreg] = areg; oreg = 0; break;
      case i_ldac: areg = oreg; oreg = 0; break;
      case i_ldbc: breg = oreg; oreg = 0; break;
      case i_ldap: areg = pc + oreg; oreg = 0; break;
      case i_ldai: areg = mem[areg + oreg]; oreg = 0; break;
      case i_ldbi: breg = mem[breg + oreg]; oreg = 0; break;
      case i_stai: mem[breg + oreg] = areg; oreg = 0; break;
      case i_br:  pc = pc + oreg; oreg = 0; break;
      case i_brz: if (areg == 0) pc = pc + oreg; oreg = 0;break;
      case i_brn: if ((int)areg < 0) pc = pc + oreg; oreg = 0;break;
      case i_pfix: oreg = oreg << 4; break;
      case i_nfix: oreg = 0xFFFFFF00 | (oreg << 4); break;
      case i_opr:
        switch (oreg)
        { case o_brb: pc = breg; oreg = 0; break;
          case o_add: areg = areg + breg; oreg = 0; break;
          case o_sub: areg = areg - breg; oreg = 0; break;
          case o_svc:  svc(); break;
        };
        oreg = 0;
        break;
   };
  }
}
