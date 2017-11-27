#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/bsearch.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>

#include "clk.h"

static DEFINE_SPINLOCK(cpm_pll_lock);

static u32 frac_to_value(u32 frac)
{
	u32 t = frac * 125;
	u32 v = t / 0x200000 + (((t % 0x200000) * 2 >= 0x200000) ? 1 : 0);

	return v;
}

static int pll_set_rate(struct clk *clk,unsigned long rate) {
	return -1;
}

static unsigned long pll_get_rate(struct clk *clk) {
	unsigned long cpxpcr;
	unsigned long m,n,o1,o0;
	unsigned long rate;
	unsigned long flags;
	unsigned long frac, frac_value;

	spin_lock_irqsave(&cpm_pll_lock,flags);

	cpxpcr = cpm_inl(CLK_PLL_NO(clk->flags));
	if(cpxpcr & 1){
		clk->flags |= CLK_FLG_ENABLE;
		m = ((cpxpcr >> 20) & 0xfff);
		n = ((cpxpcr >> 14) & 0x3f);
		o1 = ((cpxpcr >> 11) & 0x07);
		o0 = ((cpxpcr >> 8) & 0x07);
		frac = cpm_inl(CLK_PLL_NO(clk->flags) + 8);
		frac_value = frac_to_value(frac);
		rate = ((clk->parent->rate / 4000) * m / n / o1 / o0) * 4000
			+ ((clk->parent->rate / 4000) * frac_value / n / o1 / o0) * 4;

	}else  {
		clk->flags &= ~(CLK_FLG_ENABLE);
		rate = 0;
	}
	spin_unlock_irqrestore(&cpm_pll_lock,flags);
	return rate;
}
static struct clk_ops clk_pll_ops = {
	.get_rate = pll_get_rate,
	.set_rate = pll_set_rate,
};
void __init init_ext_pll(struct clk *clk)
{
	switch (get_clk_id(clk)) {
	case CLK_ID_EXT0:
		clk->rate = JZ_EXTAL_RTC;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	case CLK_ID_EXT1:
		clk->rate = JZ_EXTAL;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	case CLK_ID_OTGPHY:
		clk->rate = 48 * 1000 * 1000;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	default:
		clk->parent = get_clk_from_id(CLK_ID_EXT1);
		clk->rate = pll_get_rate(clk);
		clk->ops = &clk_pll_ops;
		break;
	}
}
