char patch_names[][9] = {
  /*   0 */ "acpiano",
  /*   1 */ "britepno",
  /*   2 */ "synpiano",
  /*   3 */ "honky",
  /*   4 */ "epiano1",
  /*   5 */ "epiano2",
  /*   6 */ "hrpschrd",
  /*   7 */ "clavinet",
  /*   8 */ "celeste",
  /*   9 */ "glocken",
  /*  10 */ "musicbox",
  /*  11 */ "vibes",
  /*  12 */ "marimba",
  /*  13 */ "xylophon",
  /*  14 */ "tubebell",
  /*  15 */ "santur",
  /*  16 */ "homeorg",
  /*  17 */ "percorg",
  /*  18 */ "rockorg",
  /*  19 */ "church",
  /*  20 */ "reedorg",
  /*  21 */ "accordn",
  /*  22 */ "harmonca",
  /*  23 */ "concrtna",
  /*  24 */ "nyguitar",
  /*  25 */ "acguitar",
  /*  26 */ "jazzgtr",
  /*  27 */ "cleangtr",
  /*  28 */ "mutegtr",
  /*  29 */ "odguitar",
  /*  30 */ "distgtr",
  /*  31 */ "gtrharm",
  /*  32 */ "acbass",
  /*  33 */ "fngrbass",
  /*  34 */ "pickbass",
  /*  35 */ "fretless",
  /*  36 */ "slapbas1",
  /*  37 */ "slapbas2",
  /*  38 */ "synbass1",
  /*  39 */ "synbass2",
  /*  40 */ "violin",
  /*  41 */ "viola",
  /*  42 */ "cello",
  /*  43 */ "contraba",
  /*  44 */ "marcato",
  /*  45 */ "pizzcato",
  /*  46 */ "harp",
  /*  47 */ "timpani",
  /*  48 */ "marcato",
  /*  49 */ "slowstr",
  /*  50 */ "synstr1",
  /*  51 */ "synstr2",
  /*  52 */ "choir",
  /*  53 */ "doo",
  /*  54 */ "voices",
  /*  55 */ "orchhit",
  /*  56 */ "trumpet",
  /*  57 */ "trombone",
  /*  58 */ "tuba",
  /*  59 */ "mutetrum",
  /*  60 */ "frenchrn",
  /*  61 */ "hitbrass",
  /*  62 */ "synbras1",
  /*  63 */ "synbras2",
  /*  64 */ "sprnosax",
  /*  65 */ "altosax",
  /*  66 */ "tenorsax",
  /*  67 */ "barisax",
  /*  68 */ "oboe",
  /*  69 */ "englhorn",
  /*  70 */ "bassoon",
  /*  71 */ "clarinet",
  /*  72 */ "piccolo",
  /*  73 */ "flute",
  /*  74 */ "recorder",
  /*  75 */ "woodflut",
  /*  76 */ "bottle",
  /*  77 */ "shakazul",
  /*  78 */ "whistle",
  /*  79 */ "ocarina",
  /*  80 */ "sqrwave",
  /*  81 */ "sawwave",
  /*  82 */ "calliope",
  /*  83 */ "chiflead",
  /*  84 */ "voxlead",
  /*  85 */ "voxlead",
  /*  86 */ "lead5th",
  /*  87 */ "basslead",
  /*  88 */ "fantasia",
  /*  89 */ "warmpad",
  /*  90 */ "polysyn",
  /*  91 */ "ghostie",
  /*  92 */ "bowglass",
  /*  93 */ "metalpad",
  /*  94 */ "halopad",
  /*  95 */ "sweeper",
  /*  96 */ "aurora",
  /*  97 */ "soundtrk",
  /*  98 */ "crystal",
  /*  99 */ "atmosphr",
  /* 100 */ "freshair",
  /* 101 */ "unicorn",
  /* 102 */ "sweeper",
  /* 103 */ "startrak",
  /* 104 */ "sitar",
  /* 105 */ "banjo",
  /* 106 */ "shamisen",
  /* 107 */ "koto",
  /* 108 */ "kalimba",
  /* 109 */ "bagpipes",
  /* 110 */ "fiddle",
  /* 111 */ "shannai",
  /* 112 */ "carillon",
  /* 113 */ "agogo",
  /* 114 */ "steeldrm",
  /* 115 */ "woodblk",
  /* 116 */ "taiko",
  /* 117 */ "toms",
  /* 118 */ "syntom",
  /* 119 */ "revcym",
  /* 120 */ "fx-fret",
  /* 121 */ "fx-blow",
  /* 122 */ "seashore",
  /* 123 */ "jungle",
  /* 124 */ "telephon",
  /* 125 */ "helicptr",
  /* 126 */ "applause",
  /* 127 */ "pistol",

  "",				/* 128 = drum 0 */
  "",				/* 129 = drum 1 */
  "",				/* 130 = drum 2 */
  "",				/* 131 = drum 3 */
  "",				/* 132 = drum 4 */
  "",				/* 133 = drum 5 */
  "",				/* 134 = drum 6 */
  "",				/* 135 = drum 7 */
  "",				/* 136 = drum 8 */
  "",				/* 137 = drum 9 */
  "",				/* 138 = drum 10 */
  "",				/* 139 = drum 11 */
  "",				/* 140 = drum 12 */
  "",				/* 141 = drum 13 */
  "",				/* 142 = drum 14 */
  "",				/* 143 = drum 15 */
  "",				/* 144 = drum 16 */
  "",				/* 145 = drum 17 */
  "",				/* 146 = drum 18 */
  "",				/* 147 = drum 19 */
  "",				/* 148 = drum 20 */
  "",				/* 149 = drum 21 */
  "",				/* 150 = drum 22 */
  "",				/* 151 = drum 23 */
  "",				/* 152 = drum 24 */
  "",				/* 153 = drum 25 */
  "",				/* 154 = drum 26 */
  "highq",			/* 155 = drum 27 */
  "slap",			/* 156 = drum 28 */
  "scratch1",			/* 157 = drum 29 */
  "scratch2",			/* 158 = drum 30 */
  "sticks",			/* 159 = drum 31 */
  "sqrclick",			/* 160 = drum 32 */
  "metclick",			/* 161 = drum 33 */
  "metbell",			/* 162 = drum 34 */
  "kick1",			/* 163 = drum 35 */
  "kick2",			/* 164 = drum 36 */
  "stickrim",			/* 165 = drum 37 */
  "snare1",			/* 166 = drum 38 */
  "claps",			/* 167 = drum 39 */
  "snare2",			/* 168 = drum 40 */
  "tomlo2",			/* 169 = drum 41 */
  "hihatcl",			/* 170 = drum 42 */
  "tomlo1",			/* 171 = drum 43 */
  "hihatpd",			/* 172 = drum 44 */
  "tommid2",			/* 173 = drum 45 */
  "hihatop",			/* 174 = drum 46 */
  "tommid1",			/* 175 = drum 47 */
  "tomhi2",			/* 176 = drum 48 */
  "cymcrsh1",			/* 177 = drum 49 */
  "tomhi1",			/* 178 = drum 50 */
  "cymride1",			/* 179 = drum 51 */
  "cymchina",			/* 180 = drum 52 */
  "cymbell",			/* 181 = drum 53 */
  "tamborin",			/* 182 = drum 54 */
  "cymsplsh",			/* 183 = drum 55 */
  "cowbell",			/* 184 = drum 56 */
  "cymcrsh2",			/* 185 = drum 57 */
  "vibslap",			/* 186 = drum 58 */
  "cymride2",			/* 187 = drum 59 */
  "bongohi",			/* 188 = drum 60 */
  "bongolo",			/* 189 = drum 61 */
  "congahi1",			/* 190 = drum 62 */
  "congahi2",			/* 191 = drum 63 */
  "congalo",			/* 192 = drum 64 */
  "timbaleh",			/* 193 = drum 65 */
  "timbalel",			/* 194 = drum 66 */
  "agogohi",			/* 195 = drum 67 */
  "agogolo",			/* 196 = drum 68 */
  "cabasa",			/* 197 = drum 69 */
  "maracas",			/* 198 = drum 70 */
  "whistle1",			/* 199 = drum 71 */
  "whistle2",			/* 200 = drum 72 */
  "guiro1",			/* 201 = drum 73 */
  "guiro2",			/* 202 = drum 74 */
  "clave",			/* 203 = drum 75 */
  "woodblk1",			/* 204 = drum 76 */
  "woodblk2",			/* 205 = drum 77 */
  "cuica1",			/* 206 = drum 78 */
  "cuica2",			/* 207 = drum 79 */
  "triangl1",			/* 208 = drum 80 */
  "triangl2",			/* 209 = drum 81 */
  "shaker",			/* 210 = drum 82 */
  "jingles",			/* 211 = drum 83 */
  "belltree",			/* 212 = drum 84 */
  "castinet",			/* 213 = drum 85 */
  "surdo1",			/* 214 = drum 86 */
  "surdo2",			/* 215 = drum 87 */
  "",				/* 216 = drum 88 */
  "",				/* 217 = drum 89 */
  "",				/* 218 = drum 90 */
  "",				/* 219 = drum 91 */
  "",				/* 220 = drum 92 */
  "",				/* 221 = drum 93 */
  "",				/* 222 = drum 94 */
  "",				/* 223 = drum 95 */
  "",				/* 224 = drum 96 */
  "",				/* 225 = drum 97 */
  "",				/* 226 = drum 98 */
  "",				/* 227 = drum 99 */
  "",				/* 228 = drum 100 */
  "",				/* 229 = drum 101 */
  "",				/* 230 = drum 102 */
  "",				/* 231 = drum 103 */
  "",				/* 232 = drum 104 */
  "",				/* 233 = drum 105 */
  "",				/* 234 = drum 106 */
  "",				/* 235 = drum 107 */
  "",				/* 236 = drum 108 */
  "",				/* 237 = drum 109 */
  "",				/* 238 = drum 110 */
  "",				/* 239 = drum 111 */
  "",				/* 240 = drum 112 */
  "",				/* 241 = drum 113 */
  "",				/* 242 = drum 114 */
  "",				/* 243 = drum 115 */
  "",				/* 244 = drum 116 */
  "",				/* 245 = drum 117 */
  "",				/* 246 = drum 118 */
  "",				/* 247 = drum 119 */
  "",				/* 248 = drum 120 */
  "",				/* 249 = drum 121 */
  "",				/* 250 = drum 122 */
  "",				/* 251 = drum 123 */
  "",				/* 252 = drum 124 */
  "",				/* 253 = drum 125 */
  "",				/* 254 = drum 126 */
  ""				/* 255 = drum 127 */
};
