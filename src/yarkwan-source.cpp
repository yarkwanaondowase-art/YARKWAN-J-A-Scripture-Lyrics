#include "yarkwan-source.hpp"
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QString>
#include <mutex>
#include <vector>

struct YarkwanSource {
    std::mutex mutex;
    QString text = QStringLiteral("For where two or three are gathered together in My name, I am there in the midst of them.");
    QString reference = QStringLiteral("Matthew 18:20");
    QString mode = QStringLiteral("lower-third");
    QString font = QStringLiteral("Arial");
    int fontSize = 48;
    uint32_t textColor = 0xFFFFFFFF;
    uint32_t bgColor = 0xB0000000;
    int margin = 60;
    bool outline = true;
    gs_texture_t *texture = nullptr;
    int texWidth = 1920;
    int texHeight = 1080;
    bool dirty = true;
};

static uint32_t parseColor(const char *s, uint32_t fallback)
{
    if (!s || !*s) return fallback;
    QString v = QString::fromUtf8(s).trimmed();
    if (v.startsWith('#')) v.remove(0, 1);
    bool ok = false;
    uint32_t x = v.toUInt(&ok, 16);
    return ok ? (v.size() <= 6 ? (0xFF000000u | x) : x) : fallback;
}

static void renderImage(YarkwanSource *d)
{
    std::lock_guard<std::mutex> lock(d->mutex);
    const int w = 1920, h = 1080;
    QImage image(w, h, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const QColor bg((d->bgColor >> 16) & 255, (d->bgColor >> 8) & 255, d->bgColor & 255, (d->bgColor >> 24) & 255);
    const QColor fg((d->textColor >> 16) & 255, (d->textColor >> 8) & 255, d->textColor & 255, (d->textColor >> 24) & 255);
    QFont font(d->font, d->fontSize, QFont::Normal);
    font.setHintingPreference(QFont::PreferNoHinting);
    p.setFont(font);

    QRect textRect;
    if (d->mode == "full-screen") {
        p.fillRect(image.rect(), bg);
        textRect = QRect(d->margin, d->margin, w - 2*d->margin, h - 2*d->margin);
    } else {
        const int barH = qMin(330, qMax(180, d->fontSize * 5));
        p.fillRect(QRect(0, h - barH, w, barH), bg);
        textRect = QRect(d->margin, h - barH + 28, w - 2*d->margin, barH - 45);
    }

    if (d->outline) {
        p.setPen(QColor(0, 0, 0, 220));
        const int o = qMax(1, d->fontSize / 18);
        for (int dx=-o; dx<=o; ++dx)
            for (int dy=-o; dy<=o; ++dy)
                if (dx || dy) p.drawText(textRect.translated(dx, dy), Qt::AlignCenter | Qt::TextWordWrap, d->text + (d->reference.isEmpty() ? "" : "\n" + d->reference));
    }
    p.setPen(fg);
    p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, d->text + (d->reference.isEmpty() ? "" : "\n" + d->reference));
    p.end();

    if (d->texture) gs_texture_destroy(d->texture);
    const uint8_t *data = image.constBits();
    d->texture = gs_texture_create(w, h, GS_RGBA, 1, &data, GS_DYNAMIC);
    d->texWidth = w; d->texHeight = h; d->dirty = false;
}

static const char *source_name(void *) { return "YARKWAN J. A. Scripture & Lyrics"; }
static void *source_create(obs_data_t *settings, obs_source_t *)
{
    auto *d = new YarkwanSource;
    const char *t = obs_data_get_string(settings, "text"); if (t && *t) d->text = QString::fromUtf8(t);
    const char *r = obs_data_get_string(settings, "reference"); if (r) d->reference = QString::fromUtf8(r);
    const char *m = obs_data_get_string(settings, "mode"); if (m && *m) d->mode = QString::fromUtf8(m);
    const char *f = obs_data_get_string(settings, "font"); if (f && *f) d->font = QString::fromUtf8(f);
    d->fontSize = (int)obs_data_get_int(settings, "font_size");
    if (d->fontSize <= 0) d->fontSize = 48;
    d->textColor = parseColor(obs_data_get_string(settings, "text_color"), d->textColor);
    d->bgColor = parseColor(obs_data_get_string(settings, "background_color"), d->bgColor);
    d->outline = obs_data_get_bool(settings, "outline");
    return d;
}
static void source_destroy(void *data)
{
    auto *d = static_cast<YarkwanSource *>(data);
    if (d->texture) { obs_enter_graphics(); gs_texture_destroy(d->texture); obs_leave_graphics(); }
    delete d;
}
static void source_update(void *data, obs_data_t *settings)
{
    auto *d = static_cast<YarkwanSource *>(data);
    std::lock_guard<std::mutex> lock(d->mutex);
    d->text = QString::fromUtf8(obs_data_get_string(settings, "text"));
    d->reference = QString::fromUtf8(obs_data_get_string(settings, "reference"));
    d->mode = QString::fromUtf8(obs_data_get_string(settings, "mode"));
    d->font = QString::fromUtf8(obs_data_get_string(settings, "font"));
    d->fontSize = (int)obs_data_get_int(settings, "font_size");
    d->textColor = parseColor(obs_data_get_string(settings, "text_color"), d->textColor);
    d->bgColor = parseColor(obs_data_get_string(settings, "background_color"), d->bgColor);
    d->outline = obs_data_get_bool(settings, "outline");
    d->dirty = true;
}
static void source_defaults(obs_data_t *s)
{
    obs_data_set_default_string(s, "text", "For where two or three are gathered together in My name, I am there in the midst of them.");
    obs_data_set_default_string(s, "reference", "Matthew 18:20");
    obs_data_set_default_string(s, "mode", "lower-third");
    obs_data_set_default_string(s, "font", "Arial");
    obs_data_set_default_int(s, "font_size", 48);
    obs_data_set_default_string(s, "text_color", "#FFFFFF");
    obs_data_set_default_string(s, "background_color", "#B0000000");
    obs_data_set_default_bool(s, "outline", true);
}
static obs_properties_t *source_properties(void *)
{
    auto *p = obs_properties_create();
    obs_property_t *mode = obs_properties_add_list(p, "mode", "Display mode", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(mode, "Lower Third", "lower-third");
    obs_property_list_add_string(mode, "Full Screen", "full-screen");
    obs_properties_add_text(p, "text", "Text", OBS_TEXT_MULTILINE);
    obs_properties_add_text(p, "reference", "Reference", OBS_TEXT_DEFAULT);
    obs_properties_add_text(p, "font", "Font", OBS_TEXT_DEFAULT);
    obs_properties_add_int(p, "font_size", "Font size", 12, 180, 1);
    obs_properties_add_text(p, "text_color", "Text color (#RRGGBB)", OBS_TEXT_DEFAULT);
    obs_properties_add_text(p, "background_color", "Background (#AARRGGBB)", OBS_TEXT_DEFAULT);
    obs_properties_add_bool(p, "outline", "Text outline");
    return p;
}
static uint32_t source_width(void *) { return 1920; }
static uint32_t source_height(void *) { return 1080; }
static void source_render(void *data, gs_effect_t *)
{
    auto *d = static_cast<YarkwanSource *>(data);
    if (d->dirty || !d->texture) renderImage(d);
    if (!d->texture) return;
    gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    while (gs_effect_loop(effect, "Draw")) gs_draw_sprite(d->texture, 0, 1920, 1080);
}

struct obs_source_info yarkwan_scripture_source_info = {
    .id = "yarkwan_scripture_lyrics_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = source_name,
    .create = source_create,
    .destroy = source_destroy,
    .update = source_update,
    .get_defaults = source_defaults,
    .get_properties = source_properties,
    .get_width = source_width,
    .get_height = source_height,
    .video_render = source_render,
};
