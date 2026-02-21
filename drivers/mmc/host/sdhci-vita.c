// SPDX-License-Identifier: GPL-2.0-only
/*
 * SDHCI driver for the PlayStation Vita (SDIF controllers)
 *
 * The Vita SoC has 4 SDIF hosts using standard SDHCI registers:
 *   SDIF0 (0xE0B00000) - eMMC
 *   SDIF1 (0xE0C00000) - Game card
 *   SDIF2 (0xE0C10000) - WLAN/BT (Marvell SD8787 SDIO)
 *   SDIF3 (0xE0C20000) - microSD (SD2Vita mod)
 *
 * Clock gating and reset are managed via the "pervasive" registers:
 *   Gate:  0xE3102000 + 0xA0 + bus*4  (set bit 0 to enable)
 *   Reset: 0xE3101000 + 0xA0 + bus*4  (clear bit 0 to deassert)
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "sdhci.h"

#define PERVASIVE_GATE_BASE	0xE3102000
#define PERVASIVE_RESET_BASE	0xE3101000
#define PERVASIVE_SDIF_OFFSET	0xA0

static void sdhci_vita_pervasive_init(struct device *dev, u32 bus_index)
{
	void __iomem *gate_reg, *reset_reg;
	u32 val;

	gate_reg = devm_ioremap(dev,
		PERVASIVE_GATE_BASE + PERVASIVE_SDIF_OFFSET + bus_index * 4, 4);
	reset_reg = devm_ioremap(dev,
		PERVASIVE_RESET_BASE + PERVASIVE_SDIF_OFFSET + bus_index * 4, 4);
	if (!gate_reg || !reset_reg) {
		dev_warn(dev, "SDIF%u: could not map pervasive regs\n", bus_index);
		return;
	}

	/* Enable clock gate (set bit 0) */
	val = readl(gate_reg);
	val |= 1;
	writel(val, gate_reg);

	/* Deassert reset (clear bit 0) */
	val = readl(reset_reg);
	val &= ~1;
	writel(val, reset_reg);
}

static unsigned int sdhci_vita_get_max_clock(struct sdhci_host *host)
{
	return 48000000;
}

static const struct sdhci_ops sdhci_vita_ops = {
	.set_clock = sdhci_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
	.get_max_clock = sdhci_vita_get_max_clock,
};

static int sdhci_vita_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct resource *iomem;
	void __iomem *ioaddr;
	u32 bus_index;
	u16 version;
	u32 caps, present;
	int irq, ret;

	ret = of_property_read_u32(pdev->dev.of_node, "vita,bus-index", &bus_index);
	if (ret) {
		dev_err(&pdev->dev, "missing vita,bus-index property\n");
		return ret;
	}
	if (bus_index > 3) {
		dev_err(&pdev->dev, "invalid bus-index %u\n", bus_index);
		return -EINVAL;
	}

	/* Enable clock and deassert reset before touching SDHCI registers */
	sdhci_vita_pervasive_init(&pdev->dev, bus_index);

	/* Map SDHCI registers to read diagnostics regardless of IRQ */
	ioaddr = devm_platform_get_and_ioremap_resource(pdev, 0, &iomem);
	if (IS_ERR(ioaddr))
		return PTR_ERR(ioaddr);

	version = readw(ioaddr + SDHCI_HOST_VERSION);
	caps = readl(ioaddr + SDHCI_CAPABILITIES);
	present = readl(ioaddr + SDHCI_PRESENT_STATE);

	dev_info(&pdev->dev,
		 "SDIF%u @ 0x%08x: version 0x%04x, caps 0x%08x, caps1 0x%08x, present 0x%08x%s\n",
		 bus_index, (u32)iomem->start, version, caps,
		 readl(ioaddr + SDHCI_CAPABILITIES_1),
		 present,
		 (present & (1 << 16)) ? " [card present]" : " [no card]");
	dev_info(&pdev->dev,
		 "SDIF%u: base_clk=%uMHz timeout_clk=%u%s max_blk=%u%s%s%s%s%s\n",
		 bus_index,
		 (caps >> 8) & 0x3f,
		 caps & 0x3f,
		 (caps & (1 << 7)) ? "MHz" : "kHz",
		 512 << ((caps >> 16) & 0x3),
		 (caps & (1 << 21)) ? " HS" : "",
		 (caps & (1 << 22)) ? " SDMA" : "",
		 (caps & (1 << 19)) ? " ADMA2" : "",
		 (caps & (1 << 24)) ? " 3.3V" : "",
		 (caps & (1 << 26)) ? " 1.8V" : "");

	/* Check if we have an interrupt — full SDHCI needs one */
	irq = platform_get_irq_optional(pdev, 0);
	if (irq < 0) {
		dev_info(&pdev->dev,
			 "SDIF%u: no IRQ assigned, hardware probe only\n",
			 bus_index);
		return 0;
	}

	/* Full SDHCI host registration */
	host = sdhci_alloc_host(&pdev->dev, 0);
	if (IS_ERR(host))
		return PTR_ERR(host);

	host->ioaddr = ioaddr;
	host->irq = irq;
	host->ops = &sdhci_vita_ops;
	host->quirks = SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER |
		       SDHCI_QUIRK_BROKEN_TIMEOUT_VAL |
		       SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
		       SDHCI_QUIRK_BROKEN_CARD_DETECTION;

	ret = mmc_of_parse(host->mmc);
	if (ret)
		goto err_free;

	ret = sdhci_add_host(host);
	if (ret)
		goto err_free;

	platform_set_drvdata(pdev, host);
	return 0;

err_free:
	sdhci_free_host(host);
	return ret;
}

static void sdhci_vita_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);

	if (host) {
		sdhci_remove_host(host, 0);
		sdhci_free_host(host);
	}
}

static const struct of_device_id sdhci_vita_of_match[] = {
	{ .compatible = "vita,sdhci" },
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_vita_of_match);

static struct platform_driver sdhci_vita_driver = {
	.driver = {
		.name = "sdhci-vita",
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
		.of_match_table = sdhci_vita_of_match,
	},
	.probe = sdhci_vita_probe,
	.remove_new = sdhci_vita_remove,
};
module_platform_driver(sdhci_vita_driver);

MODULE_DESCRIPTION("SDHCI driver for PlayStation Vita");
MODULE_LICENSE("GPL v2");
