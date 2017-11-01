
#ifndef _JZMXU128_H_
#define _JZMXU128_H_

#define BNEZ(fmt,vrt,s10)					\
	do{							\
		__asm__ __volatile ("bnez"#fmt"  %0, " #s10	\
				    ::"q" ((vrt)));		\
	}while (0)

#define BNEZ1Q(vrt,s10)					\
	do{						\
		__asm__ __volatile ("bnez1q  %0, " #s10	\
				    ::"q" ((vrt)));	\
	}while (0)

#define BEQZ(fmt,vrt,s10)					\
	do{							\
		__asm__ __volatile ("beqz"#fmt"  %0, " #s10	\
				    ::"q" ((vrt)));		\
	}while(0)

#define BEQZ1Q(vrt,s10)					\
	do{						\
		__asm__ __volatile ("beqz1q  %0, " #s10	\
				    ::"q" ((vrt)));	\
	}while(0)

#define CEQ(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("ceq"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define CEQZ(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("ceqz"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define CNE(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("cne"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define CNEZ(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("cnez"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define CLES(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("cles"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define CLEU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("cleu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define CLEZ(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("clez"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define CLTS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("clts"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define CLTU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("cltu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define CLTZ(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("cltz"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define ADDA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("adda"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define ADDAS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("addas"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define ADDSS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("addss"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define ADDUU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("adduu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define ADD(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("add"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SUBSA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("subsa"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SUBUA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("subua"#fmt"  %0, %1,%2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SUBSS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("subss"#fmt"  %0, %1,%2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SUBUU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("subuu"#fmt"  %0, %1,%2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SUBUS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("subus"#fmt"  %0, %1,%2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SUB(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("sub"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define AVES(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("aves"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define AVEU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("aveu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define AVERS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("avers"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define AVERU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("averu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DIVS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("divs"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DIVU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("divu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MODS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("mods"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MODU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("modu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MADD(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("madd"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MSUB(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("msub"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MUL(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("mul"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MAXA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("maxa"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MAXS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("maxs"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MAXU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("maxu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MINA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("mina"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MINS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("mins"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MINU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("minu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SATS(fmt,vrd,vrs,m)					\
	do{							\
		__asm__ __volatile ("sats"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((m)));	\
	}while(0)

#define SATU(fmt,vrd,vrs,m)					\
	do{							\
		__asm__ __volatile ("satu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((m)));	\
	}while(0)

#define DOTPS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("dotps"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DOTPU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("dotpu"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DADDS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("dadds"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DADDU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("daddu"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DSUBS(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("dsubs"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define DSUBU(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("dsubu"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define LOC(fmt,vrd,vrs)				\
	do{						\
		__asm__ __volatile ("loc"#fmt"  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define LZC(fmt,vrd,vrs)				\
	do{						\
		__asm__ __volatile ("lzc"#fmt"  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define BCNT(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("bcnt"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define ANDV(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("andv  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define ANDIB(vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("andib  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define NORV(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("norv  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define NORIB(vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("norib  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define ORV(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("orv  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define ORIB(vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("orib  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define XORV(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("xorv  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define XORIB(vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("xorib  %0, %1, %2"		\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define BSELV(vrd,vrs,vrt,vrr)						\
	do{								\
		__asm__ __volatile ("bselv  %0, %1, %2, %3"		\
				    :"=q"((vrd))			\
				    :"q"((vrs)),"q"((vrt)),"q"((vrr))); \
	}while(0)

#define FADD(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fadd"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FSUB(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fsub"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FMUL(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmul"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FDIV(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fdiv"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FSQRT(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("fsqrt"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define FMADD(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmadd"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FMSUB(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmsub"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FMAX(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmax"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FMAXA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmaxa"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FMIN(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmin"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FMINA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fmina"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FCLASS(fmt,vrd,vrs)					\
	do{							\
		__asm__ __volatile ("fclass"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)));		\
	}while(0)

#define FCEQ(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fceq"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FCLE(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fcle"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FCLT(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fclt"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define FCOR(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("fcor"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define VCVTHS(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("vcvths  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define VCVTSD(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("vcvtsd  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define VCVTESH(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtesh  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTEDS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvteds  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTOSH(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtosh  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTODS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtods  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTSSW(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtssw  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTSDL(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtsdl  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTUSW(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtusw  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTUDL(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtudl  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTSWS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtsws  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTSLD(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtsld  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTUWS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtuws  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTULD(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtuld  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTRWS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtrws  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTRLD(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtrld  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VTRUNCSWS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vtruncsws  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VTRUNCSLD(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vtruncsld  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VTRUNCUWS(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vtruncuws  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VTRUNCULD(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vtrunculd  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTQESH(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtqesh  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTQEDW(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtqedw  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTQOSH(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtqosh  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTQODW(vrd,vrs)				\
	do{						\
		__asm__ __volatile ("vcvtqodw  %0, %1"	\
				    :"=q"((vrd))	\
				    :"q"((vrs)));	\
	}while(0)

#define VCVTQHS(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("vcvtqhs  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define VCVTQWD(vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("vcvtqwd  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MADDQ(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("maddq"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MADDQR(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("maddqr"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MSUBQ(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("msubq"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MSUBQR(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("msubqr"#fmt"  %0, %1, %2"	\
				    :"+q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MULQ(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("mulq"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define MULQR(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("mulqr"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SLL(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("sll"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SLLI(fmt,vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("slli"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define SRA(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("sra"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SRAI(fmt,vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("srai"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define SRAR(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("srar"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SRARI(fmt,vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("srari"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define SRL(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("srl"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SRLI(fmt,vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("srli"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define SRLR(fmt,vrd,vrs,vrt)					\
	do{							\
		__asm__ __volatile ("srlr"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"q"((vrt)));	\
	}while(0)

#define SRLRI(fmt,vrd,vrs,i8)					\
	do{							\
		__asm__ __volatile ("srlri"#fmt"  %0, %1, %2"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((i8)));	\
	}while(0)

#define SHUFV(vrd,vrs,vrt,vrr)						\
	do{								\
		__asm__ __volatile ("shufv  %0, %1, %2,%3"		\
				    :"=q"((vrd))			\
				    :"q"((vrs)),"q"((vrt)),"q"((vrr))); \
	}while(0)

#define INSFCPU(fmt,vrd,imm,rs)						\
	do {								\
		__asm__ __volatile ("insfcpu"#fmt"  %0[%2], %z1"	\
				    :"+q"((vrd))			\
				    :"d"((rs)),"i"((imm)));		\
	} while (0)

#define INSFFPU(fmt,vrd,imm,fs)					\
	do {							\
		__asm__ __volatile ("insffpu"#fmt"  %0[%2], %1"	\
				    :"+q"((vrd))		\
				    :"f"((fs)),"i"((imm)));	\
	} while (0)

#define INSFMXU(fmt,vrd,imm,vrs)					\
	do {								\
		__asm__ __volatile ("insfmxu"#fmt"  %0[%2], %1[0]"	\
				    :"+q"((vrd))			\
				    :"q"((vrs)),"i"((imm)));		\
	} while (0)

#define REPX(fmt,vrd,vrs,rt)					\
	do {							\
		__asm__ __volatile ("repx"#fmt"  %0, %1[%2]"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"d"((rt)));	\
	} while (0)

#define REPI(fmt,vrd,vrs,imm)					\
	do {							\
		__asm__ __volatile ("repi"#fmt"  %0, %1[%2]"	\
				    :"=q"((vrd))		\
				    :"q"((vrs)),"i"((imm)));	\
	} while (0)

#define MTCPUS(fmt,rd,vrs,imm)					\
	do {							\
		__asm__ __volatile ("mtcpus"#fmt"  %0, %1[%2]"	\
				    :"=d"((rd))			\
				    :"q"((vrs)),"i"((imm)));	\
	} while (0)

#define MTCPUU(fmt,rd,vrs,imm)					\
	do {							\
		__asm__ __volatile ("mtcpuu"#fmt"  %0, %1[%2]"	\
				    :"=d"((rd))			\
				    :"q"((vrs)),"i"((imm)));	\
	} while (0)

#define MFCPU(fmt,vrd,rs)					\
	do {							\
		__asm__ __volatile ("mfcpu"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"d"((rs)));		\
	} while (0)

#define MTFPU(fmt,fd,vrs,imm)					\
	do {							\
		__asm__ __volatile ("mtfpu"#fmt" %0, %1[%2]"	\
				    :"=f"((fd))			\
				    :"q"((vrs)),"i"((imm)));	\
	} while (0)

#define MFFPU(fmt,vrd,fs)					\
	do {							\
		__asm__ __volatile ("mffpu"#fmt"  %0, %1"	\
				    :"=q"((vrd))		\
				    :"f"((fs)));		\
	} while (0)

#define CTCMXU(cd,rs)						\
	do {							\
		__asm__ __volatile ("ctcmxu  $%0, %1"		\
				    ::"K"((cd)),"d"((rs)));	\
	} while (0)

#define CFCMXU(rd,cs)					\
	do {						\
		__asm__ __volatile ("cfcmxu  %0, $%1"	\
				    :"=d"((rd))		\
				    :"K"((cs)));	\
	} while (0)

#define LU1Q(vrd,base,offset)						\
	do {								\
		__asm__ __volatile ("lu1q  %0, %1(%2)"			\
				    :"=q"((vrd))			\
				    :"i"((offset)),"d"((base)):"memory"); \
	} while (0)

#define LU1QX(vrd,base,rs)						\
	do {								\
		__asm__ __volatile ("lu1qx  %0, %1(%2)"			\
				    :"=q"((vrd))			\
				    :"d"((rs)),"d"((base)):"memory");	\
	} while (0)

#define LA1Q(vrd,base,offset)						\
	do {								\
		__asm__ __volatile ("la1q  %0, %1(%2)"			\
				    :"=q"((vrd))			\
				    :"i"((offset)),"d"((base)):"memory"); \
	} while (0)

#define LA1QX(vrd,base,rs)						\
	do {								\
		__asm__ __volatile ("la1qx  %0, %1(%2)"			\
				    :"=q"((vrd))			\
				    :"d" ((rs)),"d"((base)):"memory");	\
	} while (0)

#define SU1Q(vrd,base,offset)						\
	do {								\
		__asm__ __volatile ("su1q  %0, %1(%2)"			\
				    :					\
				    :"q"((vrd)),"i"((offset)),"d"((base)):"memory"); \
	} while (0)

#define SU1QX(vrd,base,rs)						\
	do {								\
		__asm__ __volatile ("su1qx  %0, %1(%2)"			\
				    :					\
				    :"q"((vrd)),"d" ((rs)),"d"((base)):"memory"); \
	} while (0)

#define SA1Q(vrd,base,offset)						\
	do {								\
		__asm__ __volatile ("sa1q  %0, %1(%2)"			\
				    :					\
				    :"q"((vrd)),"i"((offset)),"d"((base)):"memory"); \
	} while (0)

#define SA1QX(vrd,rs,base)						\
	do {								\
		__asm__ __volatile ("sa1qx  %0, %1(%2)"			\
				    :					\
				    :"q"((vrd)),"d" ((rs)),"d"((base)):"memory"); \
	} while (0)

#define LI(fmt,vrd,imm)					\
	do {						\
		__asm__ __volatile ("li"#fmt"  %0, %1"	\
				    :"=q"((vrd))	\
				    :"i"((imm)));	\
	} while (0)

#define LABEL(l)				\
	do {					\
		__asm__ __volatile(#l":");	\
	} while(0)

#endif
