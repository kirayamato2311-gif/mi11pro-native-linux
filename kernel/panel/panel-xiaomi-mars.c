// SPDX-License-Identifier: GPL-2.0-only
/* Experimental panel port from the device's original command-mode panel DT.
 * Tested on one Mars panel variant. Requires the complete MDSS/DSI/DSC
 * power/clock DT; see the repository status and limitations.
 */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <asm/unaligned.h>
#include <drm/drm_connector.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include "mars-panel-commands.h"

struct mars_panel {
    struct drm_panel panel;
    struct mipi_dsi_device *dsi;
    struct regulator_bulk_data supplies[4];
    struct gpio_desc *reset;
    struct drm_dsc_config dsc;
    bool prepared;
};

static struct mars_panel *to_mars(struct drm_panel *panel)
{
    return container_of(panel, struct mars_panel, panel);
}

static int mars_send(struct mars_panel *ctx,
                     const struct mars_panel_command *commands, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        struct mipi_dsi_msg msg = {
            .channel = ctx->dsi->channel,
            .type = commands[i].type,
            .flags = MIPI_DSI_MSG_USE_LPM,
            .tx_buf = commands[i].data,
            .tx_len = commands[i].length,
        };
        ssize_t ret;
        /* Preserve stock long-packet type even for a two-byte payload. */
        if (!ctx->dsi->host->ops || !ctx->dsi->host->ops->transfer)
            return -ENOSYS;
        ret = ctx->dsi->host->ops->transfer(ctx->dsi->host, &msg);
        if (ret < 0)
            return dev_err_probe(&ctx->dsi->dev, ret,
                                 "DSI command %zu (0x%02x) failed\n",
                                 i, commands[i].data[0]);
        /* This MSM tree reports DMA bytes, including header/alignment.
         * Successful writes can therefore exceed the payload length.
         */
        if (ret < commands[i].length)
            return dev_err_probe(&ctx->dsi->dev, -EIO,
                "DSI command %zu incomplete: %zd < %u\n", i, ret, commands[i].length);
        if (commands[i].wait_ms)
            msleep(commands[i].wait_ms);
    }
    return 0;
}

/* Restore encoder parameters from the same PPS sent by the original panel.
 * Repacking must reproduce all 128 bytes before the driver can attach.
 */
static int mars_decode_pps(struct mars_panel *ctx)
{
    struct drm_dsc_config *d = &ctx->dsc;
    struct drm_dsc_picture_parameter_set packed;
    const u8 *p = NULL;
    size_t i;
    for (i = 0; i < ARRAY_SIZE(mars_60hz_on); i++) {
        if (mars_60hz_on[i].data[0] == 0x9e && mars_60hz_on[i].length == 129) {
            p = mars_60hz_on[i].data + 1;
            break;
        }
    }
    if (!p)
        return -EINVAL;
    d->dsc_version_major = p[0] >> 4;
    d->dsc_version_minor = p[0] & 15;
    d->line_buf_depth = p[3] & 15;
    d->bits_per_component = p[3] >> 4;
    d->bits_per_pixel = ((p[4] & 3) << 8) | p[5];
    d->vbr_enable = !!(p[4] & BIT(2));
    d->simple_422 = !!(p[4] & BIT(3));
    d->convert_rgb = !!(p[4] & BIT(4));
    d->block_pred_enable = !!(p[4] & BIT(5));
    d->pic_height = get_unaligned_be16(p + 6);
    d->pic_width = get_unaligned_be16(p + 8);
    d->slice_height = get_unaligned_be16(p + 10);
    d->slice_width = get_unaligned_be16(p + 12);
    d->slice_count = d->pic_width / d->slice_width;
    d->slice_chunk_size = get_unaligned_be16(p + 14);
    d->initial_xmit_delay = get_unaligned_be16(p + 16);
    d->initial_dec_delay = get_unaligned_be16(p + 18);
    d->initial_scale_value = p[21];
    d->scale_increment_interval = get_unaligned_be16(p + 22);
    d->scale_decrement_interval = get_unaligned_be16(p + 24);
    d->first_line_bpg_offset = p[27];
    d->nfl_bpg_offset = get_unaligned_be16(p + 28);
    d->slice_bpg_offset = get_unaligned_be16(p + 30);
    d->initial_offset = get_unaligned_be16(p + 32);
    d->final_offset = get_unaligned_be16(p + 34);
    d->flatness_min_qp = p[36];
    d->flatness_max_qp = p[37];
    d->rc_model_size = get_unaligned_be16(p + 38);
    d->rc_edge_factor = p[40];
    d->rc_quant_incr_limit0 = p[41];
    d->rc_quant_incr_limit1 = p[42];
    d->rc_tgt_offset_high = p[43] >> 4;
    d->rc_tgt_offset_low = p[43] & 15;
    for (i = 0; i < DSC_NUM_BUF_RANGES - 1; i++)
        d->rc_buf_thresh[i] = p[44 + i];
    for (i = 0; i < DSC_NUM_BUF_RANGES; i++) {
        u16 range = get_unaligned_be16(p + 58 + 2 * i);
        d->rc_range_params[i].range_min_qp = range >> 11;
        d->rc_range_params[i].range_max_qp = (range >> 6) & 31;
        d->rc_range_params[i].range_bpg_offset = range & 63;
    }
    d->mux_word_size = 48;
    drm_dsc_pps_payload_pack(&packed, d);
    if (memcmp(&packed, p, sizeof(packed))) {
        for (i = 0; i < sizeof(packed); i++)
            if (((const u8 *)&packed)[i] != p[i])
                pr_err("mars-panel: PPS byte %zu stock=%02x packed=%02x\n",
                       i, p[i], ((const u8 *)&packed)[i]);
        return -EINVAL;
    }
    if (d->pic_width != 1440 || d->pic_height != 3200 || d->slice_width != 720)
        return -EINVAL;
    return 0;
}

static int mars_prepare(struct drm_panel *panel)
{
    struct mars_panel *ctx = to_mars(panel);
    int ret;
    if (ctx->prepared)
        return 0;
    ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret)
        return dev_err_probe(&ctx->dsi->dev, ret, "Panel power enable failed\n");
    /* DT GPIO_ACTIVE_LOW maps these logical values to stock low->high. */
    gpiod_set_value_cansleep(ctx->reset, 1);
    msleep(10);
    gpiod_set_value_cansleep(ctx->reset, 0);
    msleep(10);
    ret = mars_send(ctx, mars_60hz_on, ARRAY_SIZE(mars_60hz_on));
    if (ret) {
        gpiod_set_value_cansleep(ctx->reset, 1);
        regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
        return ret;
    }
    ctx->prepared = true;
    dev_info(&ctx->dsi->dev, "Factory panel initialization completed: %zu commands\n",
             ARRAY_SIZE(mars_60hz_on));
    return 0;
}

static int mars_disable(struct drm_panel *panel)
{
    struct mars_panel *ctx = to_mars(panel);
    if (!ctx->prepared)
        return 0;
    /* The host is still powered during disable; post-disable is too late. */
    return mars_send(ctx, mars_60hz_off, ARRAY_SIZE(mars_60hz_off));
}

static int mars_unprepare(struct drm_panel *panel)
{
    struct mars_panel *ctx = to_mars(panel);
    int ret;
    if (!ctx->prepared)
        return 0;
    gpiod_set_value_cansleep(ctx->reset, 1);
    msleep(10);
    ret = regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret)
        return dev_err_probe(&ctx->dsi->dev, ret, "Panel power disable failed\n");
    ctx->prepared = false;
    dev_info(&ctx->dsi->dev, "Panel unprepared after display-off sequence\n");
    return 0;
}

static const struct drm_display_mode mars_mode = {
    .clock = (1440 + 64 + 120 + 64) * (3200 + 632 + 680 + 680) * 60 / 1000,
    .hdisplay = 1440, .hsync_start = 1504, .hsync_end = 1624, .htotal = 1688,
    .vdisplay = 3200, .vsync_start = 3832, .vsync_end = 4512, .vtotal = 5192,
};

static int mars_modes(struct drm_panel *panel, struct drm_connector *connector)
{
    struct drm_display_mode *mode = drm_mode_duplicate(connector->dev, &mars_mode);
    if (!mode)
        return -ENOMEM;
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_set_name(mode);
    drm_mode_probed_add(connector, mode);
    return 1;
}

static int mars_brightness(struct backlight_device *bl)
{
    struct mars_panel *ctx = bl_get_data(bl);
    unsigned int level = backlight_get_brightness(bl);
    u8 command[] = { 0x51, level >> 8, level & 0xff };
    if (!ctx->prepared)
        return 0;
    return mipi_dsi_dcs_write_buffer(ctx->dsi, command, sizeof(command)) < 0 ? -EIO : 0;
}

static const struct backlight_ops mars_bl_ops = {
    .update_status = mars_brightness,
};

static const struct drm_panel_funcs mars_funcs = {
    .prepare = mars_prepare,
    .disable = mars_disable,
    .unprepare = mars_unprepare,
    .get_modes = mars_modes,
};

static int mars_probe(struct mipi_dsi_device *dsi)
{
    struct mars_panel *ctx;
    struct backlight_properties props = { .type = BACKLIGHT_RAW,
        .max_brightness = 2047, .brightness = 276 };
    int ret;
    ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;
    ctx->dsi = dsi;
    ctx->supplies[0].supply = "vddio";
    ctx->supplies[1].supply = "vdd";
    ctx->supplies[2].supply = "avdd";
    ctx->supplies[3].supply = "vddd";
    ret = devm_regulator_bulk_get(&dsi->dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret)
        return dev_err_probe(&dsi->dev, ret, "Missing panel power rails\n");
    ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(ctx->reset))
        return dev_err_probe(&dsi->dev, PTR_ERR(ctx->reset), "Missing reset GPIO\n");
    ret = mars_decode_pps(ctx);
    if (ret)
        return ret;
    dsi->lanes = 4;
    dsi->format = MIPI_DSI_FMT_RGB888;
    dsi->mode_flags = MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;
    dsi->slice_per_pkt = 2;
    dsi->dsc = &ctx->dsc;
    mipi_dsi_set_drvdata(dsi, ctx);
    drm_panel_init(&ctx->panel, &dsi->dev, &mars_funcs, DRM_MODE_CONNECTOR_DSI);
    ctx->panel.prepare_prev_first = true;
    ctx->panel.backlight = devm_backlight_device_register(&dsi->dev,
        dev_name(&dsi->dev), &dsi->dev, ctx, &mars_bl_ops, &props);
    if (IS_ERR(ctx->panel.backlight))
        return PTR_ERR(ctx->panel.backlight);
    drm_panel_add(&ctx->panel);
    ret = mipi_dsi_attach(dsi);
    if (ret)
        drm_panel_remove(&ctx->panel);
    return ret;
}

static void mars_remove(struct mipi_dsi_device *dsi)
{
    struct mars_panel *ctx = mipi_dsi_get_drvdata(dsi);
    mars_disable(&ctx->panel);
    mars_unprepare(&ctx->panel);
    mipi_dsi_detach(dsi);
    drm_panel_remove(&ctx->panel);
}

static const struct of_device_id mars_of_match[] = {
    { .compatible = "xiaomi,mars-k2-38-08-0a" }, { }
};
MODULE_DEVICE_TABLE(of, mars_of_match);
static struct mipi_dsi_driver mars_driver = {
    .probe = mars_probe, .remove = mars_remove,
    .driver = { .name = "panel-xiaomi-mars-experimental", .of_match_table = mars_of_match },
};
static int __init mars_init(void)
{
    struct mars_panel test = { 0 };
    int ret = mars_decode_pps(&test);
    if (ret) {
        pr_err("mars-panel: factory DSC PPS self-check failed: %d\n", ret);
        return ret;
    }
    pr_info("mars-panel: factory DSC PPS self-check passed (128 bytes)\n");
    return mipi_dsi_driver_register(&mars_driver);
}
module_init(mars_init);
static void __exit mars_exit(void)
{
    mipi_dsi_driver_unregister(&mars_driver);
}
module_exit(mars_exit);
MODULE_DESCRIPTION("Experimental Xiaomi mars K2 command-mode DSC panel");
MODULE_LICENSE("GPL");
