#ifndef CHARLIST_H
#define CHARLIST_H

#define INT_LETTERS 1

static const char Chars[] =
#ifdef INT_LETTERS
  "abcdefghijklmnopqrstuvwxyz0123456789?.,:והצü£)/=";
#else
  "abcdefghijklmnopqrstuvwxyz0123456789?.,:)/=";
#endif

static const char *Codes[] =
  { ".-", "-...", "-.-.", "-..", ".", "..-.", "--.",
  "....", "..", ".---", "-.-", ".-..", "--", "-.", "---",
  ".--.", "--.-", ".-.", "...", "-", "..-", "...-",
  ".--", "-..-", "-.--", "--..",	/* A..Z */

  "-----", ".----", "..---", "...--", "....-",
  ".....", "-....", "--...", "---..", "----.",	/* 0..9 */

  "..--..", ".-.-.-", "--..--", "---...",	/*  ?|.,: */

#ifdef INT_LETTERS
  ".--.-", ".-.-", "---.", "..--",	/* International letters */
#endif
  "........",			/* Error */
  "-.--.-",			/* () */
  "-..-.",			/* / */
  "-...-"			/* = */
};

static int
parse_charlist (char *ch)
{
  int prev = 0, i, j;

  if (strcmp (ch, "-1") == 0)
    return parse_charlist ("aeost");
  if (strcmp (ch, "-2") == 0)
    return parse_charlist ("hilnr");
  if (strcmp (ch, "-3") == 0)
    return parse_charlist ("cfgu");
  if (strcmp (ch, "-4") == 0)
    return parse_charlist ("dkmp");
  if (strcmp (ch, "-5") == 0)
    return parse_charlist ("bqvy");
  if (strcmp (ch, "-6") == 0)
    return parse_charlist ("jvxz");
#ifdef INT_LETTERS
  if (strcmp (ch, "-7") == 0)
    return parse_charlist ("הצו");
#endif
  if (strcmp (ch, "-8") == 0)
    return parse_charlist ("50146");
  if (strcmp (ch, "-9") == 0)
    return parse_charlist ("27389");
  if (strcmp (ch, "-10") == 0)
    return parse_charlist ("/=?");
  if (strcmp (ch, "-11") == 0)
    return parse_charlist (").,-");
#ifdef INT_LETTERS
  if (strcmp (ch, "-a") == 0)	/* Finnish CWH module */
    return parse_charlist ("a-zוהצ0-9/=?");
  if (strcmp (ch, "-d") == 0)	/* Some very difficult characters */
    return parse_charlist ("1j/l4bh569צ");
#endif

  for (i = 0; i < strlen (ch); i++)
    if (ch[i] == '-')
      {
	prev = -prev;
      }
    else
      {
	if (prev > 0)
	  {
	    for (j = prev; j <= ch[i]; j++)
	      {
		randomlist[nrandom++] = j;
		randomlist[nrandom] = 0;
	      }
	  }
	else
	  {
	    randomlist[nrandom++] = ch[i];
	    randomlist[nrandom] = 0;
	  }

	prev = -((unsigned char) ch[i] + 1);
      }

  return 0;
}

#endif
