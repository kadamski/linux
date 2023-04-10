// SPDX-License-Identifier: GPL-2.0-only
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>

struct blfb_clk_data {
	const char *name;
	const char **parents;
	size_t num_parents;
	const u32 *table;
	u8 sel_shift;
	u32 sel_mask;
	u32 sel_reg;
};

static const char *uart_mux_p[] = { "mm_b", "muxpll_160m", "xtal" };
static const u32 uart_mux_t[] = { 0, 1, 2};

static const char *i2c_mux_p[] = { "mm_b", "xtal" };
static const u32 i2c_mux_t[] = { 0, 1};

static const char *spi_mux_p[] = { "muxpll_160m", "xtal" };
static const u32 spi_mux_t[] = { 0, 1};

static struct blfb_clk_data bl808_mm_blb_clks[] = {
	{
		.name = "mm_uart",
		.parents = uart_mux_p,
		.num_parents = ARRAY_SIZE(uart_mux_p),
		.table = uart_mux_t,
		.sel_shift = 4, // MM_GLB_REG_I2C_CLK_SEL_POS / MM_GLB_REG_I2C_CLK_SEL
		.sel_mask = 2,
		.sel_reg = 0x0, // MM_GLB_MM_CLK_CTRL_CPU_OFFSET / MM_GLB_MM_CLK_CTRL_CPU
	},
	{
		.name = "mm_i2c",
		.parents = i2c_mux_p,
		.num_parents = ARRAY_SIZE(i2c_mux_p),
		.table = i2c_mux_t,
		.sel_shift = 6, // MM_GLB_REG_I2C_CLK_SEL_POS / MM_GLB_REG_I2C_CLK_SEL
		.sel_mask = 1,
		.sel_reg = 0x0, // MM_GLB_MM_CLK_CTRL_CPU_OFFSET / MM_GLB_MM_CLK_CTRL_CPU
	},
	{
		.name = "mm_spi",
		.parents = spi_mux_p,
		.num_parents = ARRAY_SIZE(spi_mux_p),
		.table = spi_mux_t,
		.sel_shift = 7,
		.sel_mask = 1,
		.sel_reg = 0x0,
	},
};

static DEFINE_SPINLOCK(bl_clk_lock);

static void bl_mux_clk_setup(struct device_node *node)
{
	void __iomem *base;
	struct clk **clks;
	struct clk_onecell_data *clk_data;
	int i;

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return; // -ENOMEM;

	clks = kmalloc_array(ARRAY_SIZE(bl808_mm_blb_clks), sizeof(*clks), GFP_KERNEL);
	if (!clk_data) {
		kfree(clk_data);
		return; // -ENOMEM;
	}

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s: %pOFn iomap failed\n", __func__, node);

	}
	pr_err("%s: %pOFn TEST: 0x%x\n", __func__, node, readl(base));

	for (i = 0; i < ARRAY_SIZE(bl808_mm_blb_clks); i++) {
		struct blfb_clk_data *c = &bl808_mm_blb_clks[i];

		clks[i] = clk_register_mux_table(NULL, c->name, c->parents, c->num_parents, CLK_MUX_ROUND_CLOSEST, base + c->sel_reg, c->sel_shift, c->sel_mask, 0, c->table, &bl_clk_lock);
		if (IS_ERR(clks[i])) {
			pr_err("%s: failed to register clock %s\n", __func__, c->name);
			return;
		}
	}

	clk_data->clks = clks;
	clk_data->clk_num = ARRAY_SIZE(bl808_mm_blb_clks);

	of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
}

CLK_OF_DECLARE(bl_mux_clk, "bflb,mux-clock", bl_mux_clk_setup);
