import { useEffect, useRef, useState } from "react";
import { getFont, glyphPositions, lineMetrics, measureText, snapFontSize } from "../lib/fontMetrics.js";

// Draws one element's text the way the panel draws it.
//
// This exists because CSS cannot reproduce the device's text layout, no matter how well
// the font matches. LVGL rounds every glyph's advance to a whole pixel and sums integers
// (firmware/src/fonts/hs_fonts.h, and lv_font_fmt_txt.c:251 for the rounding); browsers
// advance fractionally and accumulate sub-pixel error, so a CSS text run drifts a pixel
// or two across a long string. That drift is exactly the thing this project set out to
// eliminate, so the preview places each glyph itself at the offset the firmware will use.
//
// Everything positional here is therefore driven by font_metrics.json — generated in the
// same run of tools/fonts/generate.mjs that produced the fonts the firmware compiled. The
// browser is only asked to draw glyph *shapes*, never to decide where they go.
//
// Residual difference: the shapes themselves. The panel renders 1-bit glyphs from
// FreeType; the browser renders anti-aliased outlines. Positions, widths, line height,
// alignment and decorations match exactly — individual glyph edges do not. Closing that
// last gap would mean shipping the generated 1bpp glyph bitmaps to the browser and
// blitting them.

/** Font loading is async, and a canvas drawn before the face is ready silently falls back
 *  to a system font — visibly wrong, with no error. Redraw once the faces resolve. */
function useFontsReady() {
  const [ready, setReady] = useState(() => document.fonts?.status === "loaded");

  useEffect(() => {
    if (!document.fonts) return undefined;
    let cancelled = false;
    // Ask for both weights explicitly: document.fonts.ready resolves for fonts already
    // in use, and a weight that nothing has requested yet may not be loaded.
    Promise.all([
      document.fonts.load('16px "HomeScreen Montserrat"'),
      document.fonts.load('600 16px "HomeScreen Montserrat"'),
    ])
      .then(() => !cancelled && setReady(true))
      .catch(() => !cancelled && setReady(true)); // draw with whatever we have
    return () => {
      cancelled = true;
    };
  }, []);

  return ready;
}

/** Where the pen starts, given the element's box width and alignment — the same rule
 *  LVGL applies when a label has a fixed width. */
function alignOffset(align, boxWidth, textWidth) {
  if (!boxWidth || boxWidth <= textWidth) return 0;
  if (align === "center") return Math.floor((boxWidth - textWidth) / 2);
  if (align === "right") return boxWidth - textWidth;
  return 0;
}

export default function GlyphText({ text, fontSize, bold, align, underline, strikethrough, width, height, scale }) {
  const canvasRef = useRef(null);
  const fontsReady = useFontsReady();

  const size = snapFontSize(fontSize);
  const label = String(text ?? "");

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext("2d");
    const { lineHeight, baselineFromTop, underlinePosition, underlineThickness } = lineMetrics(size, bold);

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Weight 600 is Montserrat SemiBold, the face generate.mjs used for the bold ladder.
    ctx.font = `${bold ? 600 : 400} ${size}px "HomeScreen Montserrat", sans-serif`;
    ctx.textBaseline = "alphabetic";
    ctx.fillStyle = "#000";

    const textWidth = measureText(label, size, bold);
    const startX = alignOffset(align, canvas.width, textWidth);
    const baselineY = baselineFromTop;

    // One fillText per glyph, at the offset the device will use. Drawing the whole string
    // in a single fillText would let the browser apply its own advances and kerning --
    // which is precisely what must not happen here.
    for (const g of glyphPositions(label, size, bold)) {
      ctx.fillText(g.char, startX + g.x, baselineY);
    }

    // Decoration geometry copied from LVGL's own label renderer (lv_draw_label.c:497 and
    // :507), not approximated — both are measured from the TOP of the line box, and both
    // use integer truncation, which puts them on a different pixel than any "looks about
    // right" formula would.
    const rule = underlineThickness || 1;

    if (underline) {
      ctx.fillRect(startX, baselineY - underlinePosition, textWidth, rule);
    }
    if (strikethrough) {
      // (line_height - base_line) * 2 / 3 + underline_thickness / 2, i.e. two thirds of
      // the ascent. C integer division truncates, so Math.floor, not Math.round.
      const y = Math.floor((baselineY * 2) / 3) + Math.floor(rule / 2);
      ctx.fillRect(startX, y, textWidth, rule);
    }
  }, [label, size, bold, align, underline, strikethrough, width, height, fontsReady]);

  const font = getFont(size, bold);
  // Backing store is in DEVICE pixels; CSS scales it to the preview's zoom. That keeps
  // one canvas pixel equal to one panel pixel, so what is drawn here is literally the
  // panel's pixel grid rather than a re-rendering at preview resolution.
  const deviceWidth = Math.max(1, Math.round(width || 1));
  const deviceHeight = Math.max(1, Math.round(height || font.lineHeight));

  return (
    <canvas
      ref={canvasRef}
      width={deviceWidth}
      height={deviceHeight}
      style={{
        width: deviceWidth * scale,
        height: deviceHeight * scale,
        display: "block",
        // Show the real pixel grid when zoomed in rather than smoothing it away -- this
        // is a preview of a 1-bit panel, and softened edges would misrepresent it.
        imageRendering: "pixelated",
      }}
    />
  );
}
