// Text measurement that matches the panel exactly, rather than approximately.
//
// The device lays out a string by summing per-glyph advances that LVGL has already
// rounded to whole pixels (firmware/src/fonts/hs_fonts.h explains the ladder;
// lv_font_fmt_txt.c:251 is where the rounding happens). A browser laying out the same
// string with the same TTF would advance fractionally and end up a pixel or two off over
// a long run — which is precisely the mismatch this whole font pipeline exists to remove.
//
// So: never measure design text with the DOM or canvas measureText(). Measure it here,
// from the table tools/fonts/generate.mjs emitted alongside the fonts the firmware
// compiled.

import metrics from "./font_metrics.json";

export const FONT_SIZES = metrics.sizes;
export const DEFAULT_FONT_SIZE = 16;

/** Every byte the wire format can carry is Latin-1, and the generated fonts cover
 *  0x20-0x7E plus 0xA0-0xFF. Anything else cannot be rendered on the device at all. */
const REPLACEMENT = "?".codePointAt(0);

function fontKey(size, bold) {
  return `${bold ? "semibold" : "regular"}-${size}`;
}

/** Snap an arbitrary px size to the nearest size the device actually has a font for.
 *  The editor should only ever offer ladder values, but existing saved designs predate
 *  the ladder and can hold anything. */
export function snapFontSize(size) {
  const target = Number(size) || DEFAULT_FONT_SIZE;
  return FONT_SIZES.reduce((best, s) =>
    Math.abs(s - target) < Math.abs(best - target) ? s : best,
  );
}

export function getFont(size, bold = false) {
  return metrics.fonts[fontKey(snapFontSize(size), bold)];
}

/** Per-glyph advance in whole pixels, matching what the device will do. */
export function glyphAdvance(codePoint, size, bold = false) {
  const font = getFont(size, bold);
  const advance = font.advance[codePoint];
  if (advance !== undefined) return advance;
  // Outside Latin-1 or outside the generated subset: the device renders "?" here too,
  // because the gateway encodes text as Latin-1 with errors="replace".
  return font.advance[REPLACEMENT] ?? 0;
}

/**
 * Width in device pixels of `text` at this size/weight — the exact column the panel's
 * pen lands on after drawing it.
 */
export function measureText(text, size, bold = false) {
  let width = 0;
  for (const ch of String(text ?? "")) {
    width += glyphAdvance(ch.codePointAt(0), size, bold);
  }
  return width;
}

/**
 * The x offset of every glyph in `text`, for a preview that positions glyphs itself
 * instead of letting the browser lay out a text run. Returns one entry per code point.
 */
export function glyphPositions(text, size, bold = false) {
  const positions = [];
  let x = 0;
  for (const ch of String(text ?? "")) {
    const codePoint = ch.codePointAt(0);
    positions.push({ char: ch, codePoint, x, advance: glyphAdvance(codePoint, size, bold) });
    x += positions[positions.length - 1].advance;
  }
  return positions;
}

/** Total vertical space one line occupies, and where its baseline sits within that. */
export function lineMetrics(size, bold = false) {
  const font = getFont(size, bold);
  return {
    lineHeight: font.lineHeight,
    // LVGL measures base_line from the BOTTOM of the line box.
    baselineFromTop: font.lineHeight - font.baseLine,
    underlinePosition: font.underlinePosition,
    underlineThickness: font.underlineThickness,
  };
}
