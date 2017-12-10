/*
 * Assembler for Emu10k1 
 * Copyright (C) 4Front Technologies 2001. All rights reserved 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <soundcard.h>
#ifdef USERLAND
#define oss_native_word unsigned long
#define oss_mutex_t unsigned long
#define oss_device_t unsigned long
#define ac97_devc unsigned long
#define oss_midi_inputbyte_t char
#define uart401_devc unsigned long
typedef int oss_mutex;
typedef void *oss_dma_handle_t;
#else
#include "../../../kernel/framework/include/os.h"
#endif /* USERLAND */
#include "../../../kernel/drv/oss_sblive/sblive.h"

#define MAX_NAME 64
#define MAX_SYMBOLS	1024

int is_audigy = 0;

int gpr_base = 0x100;
int input_base = 0x10;
int output_base = 0x20;

static char line[4096] = "", *lineptr = line;
static char lastline[4096];

typedef struct
{
  char name[MAX_NAME];
  int type;
#define SY_DUMMY	0
#define SY_GPR		1
#define SY_INPUT	2
#define SY_OUTPUT	3
#define SY_CONST	4
#define SY_FX		5
#define SY_ACCUM	6
#define SY_PARM		7
  int arg;
}
sym_t;

typedef struct
{
  char *name;
  int opcode;
}
instruction_t;

static instruction_t instructions[] = {
  {"MACS", 0x0},
  {"MACS1", 0x1},
  {"MACW", 0x2},
  {"MACW1", 0x3},
  {"MACINTS", 0x4},
  {"MACIINTW", 0x5},
  {"SUM", 0x6},
  {"ACC3", 0x6},
  {"MACMV", 0x7},
  {"ANDXOR", 0x8},
  {"TSTNEG", 0x9},
  {"LIMIT", 0xa},
  {"LIMIT1", 0xb},
  {"LOG", 0xc},
  {"EXP", 0xd},
  {"INTERP", 0xe},
  {"SKIP", 0xf},
  {NULL, 0}
};

static sym_t symtab[MAX_SYMBOLS];
static int nsyms = 0;
static int group_created = 0;

int lineno = 0, errors = 0;
static emu10k1_file fle;
static int pc;

static int ngpr = 0;

static char *
agetline (char *buf, int len)
{
  char *s;

  if (*lineptr == 0)
    {
      lineptr = line;

      if (fgets (line, sizeof (line), stdin) == NULL)
	return NULL;

      if (*line != '#')
	lineno++;

      strcpy(lastline, line);
    }

  s = buf;

  while (*lineptr && *lineptr != ';')
    {
      *s++ = *lineptr++;
    }

  *s = 0;

  if (*lineptr == ';')
    *lineptr++ = 0;

/* printf("%s\n", buf); */
  return buf;
}

static void
error (char *msg)
{
  fprintf (stderr, "%s\n", lastline);
  fprintf (stderr, "Error '%s' on line %d\n", msg, lineno);
  errors++;
}

static sym_t *
find_symbol (char *name)
{
  int i;

  for (i = 0; i < nsyms; i++)
    if (strcmp (symtab[i].name, name) == 0)
      {
	return &symtab[i];
      }

  return NULL;
}

static void
add_symbol (char *name, int type, int arg)
{
  sym_t *sym;

#if 0
  /* This is not needed any more */
  if (is_audigy)
    {
      if (type == SY_CONST)
	arg += 0x80;
    }
#endif

  /* printf("'%s' = %d/%x\n", name, type, arg);  */

  if (nsyms >= MAX_SYMBOLS)
    {
      error ("Symbol table full");
      exit (-1);
    }

  if (find_symbol (name) != NULL)
    {
      error ("Dublicate symbol");
      return;
    }

  if (strlen (name) >= MAX_NAME)
    {
      error ("Too long symbol name");
      exit (-1);
    }

  sym = &symtab[nsyms++];

  strcpy (sym->name, name);
  sym->type = type;
  sym->arg = arg;
}

static void
compile_gpr (char *parms)
{
  char *p = parms;

  while (*p && *p != ' ')
    p++;
  if (*p == ' ')
    {
      error ("Too many parameters");
      return;
    }

  if (ngpr >= MAX_GPR)
    error ("Too many GPR variables");

  add_symbol (parms, SY_GPR, gpr_base + ngpr++);
}

static void
declare_const (unsigned int gpr, char *value)
{
  int n, intv;
  float v;

  n = fle.consts.nconst;

  if (n >= MAX_CONST_PARMS)
    {
      error ("Too many constant parameters");
      return;
    }

  if (*value == 'I')
    {
      if (sscanf (&value[1], "%g", &v) != 1)
	{
	  error ("Bad floating point value");
	  return;
	}
      intv = (int) v;
      /* printf("%s/%g -> %08x -> %g\n", value, v, intv, (float)intv); */
    }
  else if (*value == '0' && value[1] == 'x')
    {
      if (sscanf (&value[2], "%x", &intv) != 1)
	{
	  error ("Bad hexadecimal value");
	  return;
	}
      /* printf("%s/%g -> %08x -> %g\n", value, v, intv, (float)intv); */
    }
  else
    {
      if (sscanf (value, "%g", &v) != 1)
	{
	  error ("Bad floating point value");
	  return;
	}
      intv = (int) (v * 0x7fffffff);
      /* printf("%s/%g -> %08x -> %g\n", value, v, intv, (float)intv / (float)0x7fffffff); */
    }

  fle.consts.consts[n].gpr = gpr;
  fle.consts.consts[n].value = intv;
  fle.consts.nconst = n + 1;
}

static void
compile_const (char *parms)
{
  char *p = parms;
  char *value;

  while (*p && *p != ' ')
    p++;

  if (*p == 0)
    {
      error ("Too few parameters");
      return;
    }

  *p++ = 0;
  while (*p == ' ')
    p++;

  if (*p == 0)
    {
      error ("Too few parameters");
      return;
    }

  value = p;

  while (*p && *p != ' ')
    p++;

  if (*p == ' ')
    {
      error ("Too many parameters");
      return;
    }

  if (ngpr >= MAX_GPR)
    error ("Too many GPR variables");

  declare_const (ngpr, value);

  add_symbol (parms, SY_GPR, gpr_base + ngpr++);
}

static void
define_parm (char *parms, char *name, int num, char *typ)
{
  int n;

/* printf("Parameter '%s/%s' = GPR %d (%d), typ=%s\n", parms, name, ngpr, num, typ); */

  n = fle.parms.ngpr;

  if (n >= MAX_GPR_PARMS)
    {
      error ("Too many GPR parameters");
      return;
    }

  if (strcmp (typ, "group") == 0)
    {
      strcpy (fle.parms.gpr[n].name, parms);
      fle.parms.gpr[n].num = 0;
      fle.parms.gpr[n].type = MIXT_GROUP;
      fle.parms.gpr[n].def = 0;
      fle.parms.ngpr = n + 1;
      group_created = 1;
      return;
    }

#if 0
  if (!group_created)
    {
      strcpy (fle.parms.gpr[n].name, "EFX");
      fle.parms.gpr[n].num = 0;
      fle.parms.gpr[n].type = MIXT_GROUP;
      fle.parms.gpr[n].def = 0;
      fle.parms.ngpr = n + 1;
      group_created = 1;
    }
#endif

  if (strcmp (typ, "stereo") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = MIXT_STEREOSLIDER;
      fle.parms.gpr[n].def = num | (num << 8);
      fle.parms.ngpr = n + 1;

      sprintf (tmp, "%s_L", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_R", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  if (strcmp (typ, "stereopeak") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = MIXT_STEREOPEAK;
      fle.parms.gpr[n].def = num | (num << 8);
      fle.parms.ngpr = n + 1;

      sprintf (tmp, "%s_L", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_R", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  if (strcmp (typ, "onoff") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = MIXT_ONOFF;
      fle.parms.gpr[n].def = num;
      fle.parms.ngpr = n + 1;

      sprintf (tmp, "%s_ON", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_OFF", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  if (strcmp (typ, "eq1") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = EMU_MIXT_EQ1;
      fle.parms.gpr[n].def = num;
      fle.parms.ngpr = n + 1;

/* printf("EQ1, GPR=%d\n", ngpr); */
      sprintf (tmp, "%s_CST_0", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_1", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_2", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_3", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_4", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  if (strcmp (typ, "eq2") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = EMU_MIXT_EQ2;
      fle.parms.gpr[n].def = num;
      fle.parms.ngpr = n + 1;

/* printf("EQ2, GPR=%d\n", ngpr); */
      sprintf (tmp, "%s_CST_0", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_1", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_2", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_3", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_4", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  if (strcmp (typ, "eq3") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = EMU_MIXT_EQ3;
      fle.parms.gpr[n].def = num;
      fle.parms.ngpr = n + 1;
/* printf("EQ3, GPR=%d\n", ngpr); */

      sprintf (tmp, "%s_CST_0", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_1", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_2", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_3", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_4", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  if (strcmp (typ, "eq4") == 0)
    {
      char tmp[128];

      strcpy (fle.parms.gpr[n].name, name);
      fle.parms.gpr[n].num = ngpr;
      fle.parms.gpr[n].type = EMU_MIXT_EQ4;
      fle.parms.gpr[n].def = num;
      fle.parms.ngpr = n + 1;
/* printf("EQ4, GPR=%d\n", ngpr); */

      sprintf (tmp, "%s_CST_0", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_1", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_2", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_3", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      sprintf (tmp, "%s_CST_4", parms);
      add_symbol (tmp, SY_PARM, gpr_base + ngpr++);
      return;
    }

  strcpy (fle.parms.gpr[n].name, name);
  fle.parms.gpr[n].num = ngpr;
  fle.parms.gpr[n].type = MIXT_SLIDER;
  fle.parms.gpr[n].def = num;
  fle.parms.ngpr = n + 1;
  add_symbol (parms, SY_PARM, gpr_base + ngpr++);
}

static void
compile_parm (char *parms)
{
  char *p = parms, *def, *typ, *name;
  int n;
  int num;

  while (*p && *p != ' ')
    p++;
  if (*p != ' ')
    {
      error ("Too few parameters");
      return;
    }

  *p++ = 0;
  while (*p && *p == ' ')
    p++;

  if (*p == 0)
    {
      error ("Too few parameters");
      return;
    }

  def = p;
  while (*p && *p != ' ')
    p++;

  if (*p != 0)
    {
      *p++ = 0;

      while (*p == ' ' || *p == '\t')
	p++;
    }

  if (*p == 0)
    typ = "";
  else
    {
      typ = p;

      while (*p && *p != ' ' && *p != '\t')
	p++;
      if (*p != 0)
	*p++ = 0;
    }

  while (*p == ' ' || *p == '\t')
    p++;
  name = p;
  while (*p && *p != ' ' && *p != '\t')
    p++;
  *p = 0;

  if (*name == 0)
    name = parms;

  if (sscanf (def, "%d", &num) != 1)
    {
      error ("Bad integer value");
      return;
    }

  define_parm (parms, name, num, typ);
}

static void
compile_input (char *parms)
{
  char *p = parms, *s;
  int num;

  while (*p && *p != ' ')
    p++;
  if (*p != ' ')
    {
      error ("Too few parameters");
      return;
    }

  *p++ = 0;
  while (*p && *p == ' ')
    p++;

  if (*p == 0)
    {
      error ("Too few parameters");
      return;
    }

  s = p;
  while (*p && *p != ' ')
    p++;
  if (*p == ' ')
    {
      error ("Too many parameters");
      return;
    }

  if (sscanf (s, "%d", &num) != 1)
    {
      error ("Bad integer value");
      return;
    }

  add_symbol (parms, SY_INPUT, input_base + num);
}

static void
compile_send (char *parms)
{
  char *p = parms, *s;
  int num;

  while (*p && *p != ' ')
    p++;
  if (*p != ' ')
    {
      error ("Too few parameters");
      return;
    }

  *p++ = 0;
  while (*p && *p == ' ')
    p++;

  if (*p == 0)
    {
      error ("Too few parameters");
      return;
    }

  s = p;
  while (*p && *p != ' ')
    p++;
  if (*p == ' ')
    {
      error ("Too many parameters");
      return;
    }

  if (sscanf (s, "%d", &num) != 1)
    {
      error ("Bad integer value");
      return;
    }

  add_symbol (parms, SY_FX, num);
}

static void
compile_output (char *parms)
{
  char *p = parms, *s;
  int num;

  while (*p && *p != ' ')
    p++;
  if (*p != ' ')
    {
      error ("Too few parameters");
      return;
    }

  *p++ = 0;
  while (*p && *p == ' ')
    p++;

  if (*p == 0)
    {
      error ("Too few parameters");
      return;
    }

  s = p;
  while (*p && *p != ' ')
    p++;
  if (*p == ' ')
    {
      error ("Too many parameters");
      return;
    }

  if (sscanf (s, "%d", &num) != 1)
    {
      error ("Bad integer value");
      return;
    }

  add_symbol (parms, SY_OUTPUT, output_base + num);
}

static void
compile_directive (char *line)
{
  char *p = line;

  while (*p && *p != ' ')
    p++;
  if (*p != ' ')
    {
      error ("Missing parameter");
      return;
    }

  *p++ = 0;

  while (*p == ' ')
    p++;

  if (strcmp (line, ".gpr") == 0)
    {
      compile_gpr (p);
      return;
    }

  if (strcmp (line, ".const") == 0)
    {
      compile_const (p);
      return;
    }

  if (strcmp (line, ".parm") == 0)
    {
      compile_parm (p);
      return;
    }

  if (strcmp (line, ".input") == 0)
    {
      compile_input (p);
      return;
    }

  if (strcmp (line, ".send") == 0)
    {
      compile_send (p);
      return;
    }

  if (strcmp (line, ".output") == 0)
    {
      compile_output (p);
      return;
    }

  error ("Unknown directive");
}

static void
compile_asm (char *iline)
{
  char line[4096], *s, *p;
  char *parms[4];
  sym_t *symbols[4];
#define EMIT(o, r, a, x, y) \
    fle.code[pc*2] =  ((x) << 10) | (y); \
    fle.code[pc*2+1] = ((o) << 20) | ((r) << 10) | a;pc++
#define EMIT_AUDIGY(o, r, a, x, y) \
    fle.code[pc*2] =  ((x) << 12) | (y); \
    fle.code[pc*2+1] = ((o) << 24) | ((r) << 12) | a;pc++

  int i, n = 0, nerr = 0;
  int ninputs = 0;

  s = iline;
  p = line;

  /* Remove all spaces  and the trailing ')' */
  while (*s)
    {
      if (*s != ' ' && *s != ')' && *s != '#')
	*p++ = *s;
      s++;
    }

  *p = 0;

  /* Replace ',' and '(' characters by spaces */
  p = line;
  while (*p)
    {
      if (*p == ',' || *p == '(')
	*p = ' ';
      p++;
    }

  p = line;
  while (*p && *p != ' ')
    p++;
  if (*p != ' ')
    {
      error ("Too few parameters");
      return;
    }

  *p++ = 0;

  while (*p && n < 4)
    {
      parms[n++] = p;

      while (*p && *p != ' ')
	p++;
      if (*p == ' ')
	*p++ = 0;
    }

  if (*p != 0)
    {
      error ("Too many parameters");
      return;
    }

  if (n != 4)
    {
      error ("Too few parameters");
      return;
    }


  for (i = 0; i < 4; i++)
    {
      if ((symbols[i] = find_symbol (parms[i])) == NULL)
	{
	  fprintf (stderr, "%s\n", parms[i]);
	  nerr++;
	  error ("Undefined symbol");
	  continue;
	}

      if (symbols[i]->type == SY_INPUT)
	ninputs++;

      if (symbols[i]->type == SY_ACCUM && i != 1)
	error ("Bad usage of 'accum' operand.");
    }

  if (nerr > 0)
    return;

  if (ninputs > 1)
    {
      error
	("Attempt to access more than one input GPRs by the same instruction");
    }

  for (i = 0; i < strlen (line); i++)
    if (line[i] >= 'a' && line[i] <= 'z')
      line[i] -= 32;		/* Upshift */

  for (i = 0; instructions[i].name != NULL; i++)
    if (strcmp (line, instructions[i].name) == 0)
      {
#if 0
	printf ("Instruction %x: ", instructions[i].opcode);
	printf ("%03x ", symbols[0]->arg);
	printf ("%03x ", symbols[1]->arg);
	printf ("%03x ", symbols[2]->arg);
	printf ("%03x ", symbols[3]->arg);
	printf ("\n");
#endif

	if (is_audigy)
	  {
	    EMIT_AUDIGY (instructions[i].opcode,
			 symbols[0]->arg,
			 symbols[1]->arg, symbols[2]->arg, symbols[3]->arg);
	  }
	else
	  {
	    EMIT (instructions[i].opcode,
		  symbols[0]->arg,
		  symbols[1]->arg, symbols[2]->arg, symbols[3]->arg);
	  }

	return;
      }

  fprintf (stderr, "%s\n", line);
  error ("Unrecognized instruction");
}

static void
init_compiler (void)
{
  char tmp[100];
  int i;

  memset (&fle, 0, sizeof (fle));
/*
 * Initialize few predefined GPR parameter registers. These definitions
 * have to be in sync with the GPR_* macros in <sblive.h>.
 */

/*
;.parm AC97VOL		0		mono		AC97
;.parm AC97MONVU		0		stereopeak	-
;.parm PCM			100		stereo
;.parm MAIN			100		stereo		VOL
;.parm VU			0		stereopeak	-
*/
  add_symbol ("NULL", SY_DUMMY, gpr_base + ngpr++);
  add_symbol ("NULL_", SY_DUMMY, gpr_base + ngpr++);
  define_parm ("PCM", "PCM", 100, "stereo");
  define_parm ("MAIN", "VOL", 100, "stereo");

  pc = 0;

  if (is_audigy)
    {
      /* Initialize the code array with NOPs (AUDIGY) */
      for (i = 0; i < 512; i++)
	{
	  fle.code[i * 2 + 0] = (0xc0 << 12) | 0xc0;
	  fle.code[i * 2 + 1] = (0x06 << 24) | (0xc0 << 12) | 0xc0;
	}

      for (i = 0; i < 32; i++)
	{
	  sprintf (tmp, "fx%d", i);
	  add_symbol (tmp, SY_FX, i);
	}
    }
  else
    {
      /* Initialize the code array with NOPs (LIVE) */
      for (i = 0; i < 512; i++)
	{
	  fle.code[i * 2 + 0] = 0x10040;
	  fle.code[i * 2 + 1] = 0x610040;
	}

      for (i = 0; i < 16; i++)
	{
	  sprintf (tmp, "fx%d", i);
	  add_symbol (tmp, SY_FX, i);
	}
    }

/*
 * Constants
 */

  if (is_audigy)
    {
      /* Audigy symbols */
      add_symbol ("0", SY_CONST, 0x0c0);
      add_symbol ("1", SY_CONST, 0x0c1);
      add_symbol ("2", SY_CONST, 0x0c2);
      add_symbol ("3", SY_CONST, 0x0c3);
      add_symbol ("4", SY_CONST, 0x0c4);
      add_symbol ("8", SY_CONST, 0x0c5);
      add_symbol ("16", SY_CONST, 0x0c6);
      add_symbol ("32", SY_CONST, 0x0c7);
      add_symbol ("256", SY_CONST, 0x0c8);
      add_symbol ("65536", SY_CONST, 0x0c9);

      add_symbol ("2048", SY_CONST, 0x0ca);
      add_symbol ("0x800", SY_CONST, 0x0ca);

      add_symbol ("2^28", SY_CONST, 0x0cb);
      add_symbol ("0x10000000", SY_CONST, 0x0cb);

      add_symbol ("2^29", SY_CONST, 0x0cc);
      add_symbol ("0x20000000", SY_CONST, 0x0cc);

      add_symbol ("2^30", SY_CONST, 0x0cd);
      add_symbol ("0x40000000", SY_CONST, 0x0cd);

      add_symbol ("2^31", SY_CONST, 0x0ce);
      add_symbol ("0x80000000", SY_CONST, 0x0ce);

      add_symbol ("0x7fffffff", SY_CONST, 0x0cf);

      add_symbol ("0xffffffff", SY_CONST, 0x0d0);
      add_symbol ("-1", SY_CONST, 0x0d0);

      add_symbol ("0xfffffffe", SY_CONST, 0x0d1);
      add_symbol ("-2", SY_CONST, 0x0d1);

      add_symbol ("0xc0000000", SY_CONST, 0x0d2);

      add_symbol ("0x4f1bbcdc", SY_CONST, 0x0d3);

      add_symbol ("0x5a7ef9db", SY_CONST, 0x0d4);

      add_symbol ("0x100000", SY_CONST, 0x0d5);

      add_symbol ("accum", SY_ACCUM, 0x0d6);
      add_symbol ("CCR", SY_CONST, 0x0d7);

      add_symbol ("noise_L", SY_CONST, 0x0d8);
      add_symbol ("noise_R", SY_CONST, 0x0d9);
      add_symbol ("IRQREQ", SY_CONST, 0x0da);
    }
  else
    {
      /* SB Live symbols */
      add_symbol ("0", SY_CONST, 0x040);
      add_symbol ("1", SY_CONST, 0x041);
      add_symbol ("2", SY_CONST, 0x042);
      add_symbol ("3", SY_CONST, 0x043);
      add_symbol ("4", SY_CONST, 0x044);
      add_symbol ("8", SY_CONST, 0x045);
      add_symbol ("16", SY_CONST, 0x046);
      add_symbol ("32", SY_CONST, 0x047);
      add_symbol ("256", SY_CONST, 0x048);
      add_symbol ("65536", SY_CONST, 0x049);

      add_symbol ("2^23", SY_CONST, 0x04a);
      add_symbol ("0x80000", SY_CONST, 0x04a);

      add_symbol ("2^28", SY_CONST, 0x04b);
      add_symbol ("0x10000000", SY_CONST, 0x04b);

      add_symbol ("2^29", SY_CONST, 0x04c);
      add_symbol ("0x20000000", SY_CONST, 0x04c);

      add_symbol ("2^30", SY_CONST, 0x04d);
      add_symbol ("0x40000000", SY_CONST, 0x04d);

      add_symbol ("2^31", SY_CONST, 0x04e);
      add_symbol ("0x80000000", SY_CONST, 0x04e);

      add_symbol ("0x7fffffff", SY_CONST, 0x04f);

      add_symbol ("0xffffffff", SY_CONST, 0x050);
      add_symbol ("-1", SY_CONST, 0x050);

      add_symbol ("0xfffffffe", SY_CONST, 0x051);
      add_symbol ("-2", SY_CONST, 0x051);

      add_symbol ("accum", SY_ACCUM, 0x056);
      add_symbol ("CCR", SY_CONST, 0x057);

      add_symbol ("noise_L", SY_CONST, 0x058);
      add_symbol ("noise_R", SY_CONST, 0x059);
      add_symbol ("IRQREQ", SY_CONST, 0x05a);
    }
}

static void
produce_map (char *name)
{
  char fname[1024];
  int i;
  FILE *f;

  sprintf (fname, "%s.map", name);

  if ((f = fopen (fname, "w")) == NULL)
    {
      perror (fname);
      return;
    }

  fprintf (f, "%d\n", fle.size);

  for (i = 0; i < nsyms; i++)
    {
      fprintf (f, "%04x %x %s\n", symtab[i].arg, symtab[i].type,
	       symtab[i].name);
    }

  fclose (f);
}

static void
produce_output (char *fname)
{
  int fd;

  if ((fd = creat (fname, 0644)) == -1)
    {
      perror (fname);
      exit (-1);
    }

  if (is_audigy)
    {
      fle.magic = EMU10K2_MAGIC;
      fle.feature_mask = SB_AUDIGY;
    }
  else
    {
      fle.magic = EMU10K1_MAGIC;
      fle.feature_mask = SB_LIVE;
    }
  fle.size = pc;
  fprintf (stderr, "%d instructions out of 512 produced\n", pc);

  if (write (fd, &fle, sizeof (fle)) != sizeof (fle))
    {
      perror (fname);
      exit (-1);
    }

  close (fd);
}

int
main (int argc, char *argv[])
{
  char iline[4096], line[4096], *p, *s, *outfile;

  if (argc < 2)
    {
      fprintf (stderr, "No output\n");
      exit (-1);
    }

  outfile = argv[1];

  if (argc > 2)
    if (strcmp (argv[2], "-2") == 0)
      {
	is_audigy = 1;
	gpr_base = 0x400;
	input_base = 0x40;
	output_base = 0x60;
	printf ("Compiling for SB Audigy\n");
      }
    else
      printf ("Compiling for SB Live family\n");

  init_compiler ();

  while (agetline (iline, sizeof (iline) - 1) != NULL)
    {
      if (*iline == '#')
	{
	  if (iline[1] == ' ')
	    if (sscanf (&iline[1], "%d", &lineno) == 1)
	      lineno--;
	  continue;
	}
/*
 * Fix for HP-UX cpp. Strip all '#' characters from input.
 */

      s = iline;
      p = line;

      while (*s)
	{
	  if (*s != '#')
	    *p++ = *s;
	  s++;
	}
      *p = 0;

      p = line;

      while (*p && *p != '\n' && *p != '/')
	p++;
      *p = 0;

      if (*line == 0)		/* Empty line */
	continue;

      p = &line[strlen (line) - 1];

      while (p > line && (*p == ' ' || *p == '\t'))
	p--;
      p[1] = 0;

      p = line;
      while (*p)
	{
	  if (*p == '\t')
	    *p = ' ';
	  p++;
	}

      p = line;

      while (*p && (*p == ' ' || *p == '\t'))
	p++;

      if (*p == 0)		/* Empty line */
	continue;

      if (*p == '.')
	compile_directive (p);
      else
	compile_asm (p);
    }

  if (lineno < 1)
    {
      fprintf (stderr, "Empty input\n");
      errors++;
    }

  if (errors == 0)
    {
      produce_output (outfile);
      produce_map (outfile);
    }

  if (errors > 0)
    {
      fprintf (stderr, "%d errors - compile failed\n", errors);
      exit (-1);
    }

  fprintf (stderr, "No errors detected - Output written to %s\n", outfile);
  exit (0);
}
