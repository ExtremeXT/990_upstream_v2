// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

static int gpu_force_freq_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const __be32 *dvfs_table;
    int table_size;
    int i;

    pr_info("gpu_force_freq: Forçando leitura das frequências do Device Tree\n");

    // Busca a tabela 'gpu_dvfs_table' no Device Tree
    dvfs_table = of_get_property(np, "gpu_dvfs_table", &table_size);
    if (!dvfs_table) {
        pr_err("gpu_force_freq: Não foi possível localizar 'gpu_dvfs_table' no Device Tree\n");
        return -EINVAL;
    }

    // Converte o tamanho de bytes para o número de entradas (freq, down, up, etc.)
    table_size /= sizeof(u32);

    pr_info("gpu_force_freq: Tabela de Frequências encontrada com %d entradas\n", table_size / 8);

    // Itera sobre a tabela e imprime as frequências
    for (i = 0; i < table_size; i += 8) {
        u32 freq = be32_to_cpu(dvfs_table[i]);
        pr_info("gpu_force_freq: Frequência #%d: %u kHz\n", i / 8, freq);
    }

    return 0;
}

static const struct of_device_id gpu_force_freq_match[] = {
    { .compatible = "arm,mali" },
    {},
};
MODULE_DEVICE_TABLE(of, gpu_force_freq_match);

static struct platform_driver gpu_force_freq_driver = {
    .driver = {
        .name = "gpu_force_freq",
        .of_match_table = gpu_force_freq_match,
    },
    .probe = gpu_force_freq_probe,
};

module_platform_driver(gpu_force_freq_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leod3c11");
MODULE_DESCRIPTION("Força a leitura das frequências GPU do Device Tree");