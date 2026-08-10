// Re-exported so existing importers keep working, but the ladder is defined by the font
// pipeline, not here: FONT_SIZES is exactly the set of sizes tools/fonts/generate.mjs
// produced a device font for. Offering a size outside it would let someone author text
// the panel cannot render faithfully.
export { FONT_SIZES, DEFAULT_FONT_SIZE, snapFontSize } from "./fontMetrics.js";
import { DEFAULT_FONT_SIZE, snapFontSize } from "./fontMetrics.js";

export function emptyElement() {
  return {
    type: "text",
    x: 10,
    y: 10,
    w: 100,
    h: 20,
    text: "",
    source: "static",
    value: "",
    entityId: "",
    attribute: "",
    unit: "",
    precision: "",
    buttonIndex: 0,
    fontSize: DEFAULT_FONT_SIZE,
    bold: false,
    align: "left",
    underline: false,
    strikethrough: false,
  };
}

export function toLayoutJson(elements) {
  return {
    elements: elements.map((el, i) => {
      let props;
      if (el.type === "text") {
        props = { text: el.text };
      } else if (el.source === "button") {
        props = { source: "button", value: el.value, button_index: el.buttonIndex };
      } else if (el.source === "home_assistant") {
        props = { source: "home_assistant", entity_id: el.entityId };
        if (el.attribute) props.attribute = el.attribute;
        if (el.unit) props.unit = el.unit;
        if (el.precision !== "" && el.precision != null) props.precision = Number(el.precision);
      } else {
        props = { source: "static", value: el.value };
      }
      props.fontSize = el.fontSize || DEFAULT_FONT_SIZE;
      if (el.bold) props.bold = true;
      if (el.align && el.align !== "left") props.align = el.align;
      if (el.underline) props.underline = true;
      if (el.strikethrough) props.strikethrough = true;
      return { id: i + 1, type: el.type, x: el.x, y: el.y, w: el.w, h: el.h, props };
    }),
  };
}

export function fromLayoutJson(layout) {
  return (layout?.elements || []).map((el) => ({
    type: el.type,
    x: el.x,
    y: el.y,
    w: el.w,
    h: el.h,
    text: el.props?.text || "",
    source: el.props?.source || "static",
    value: el.props?.value || "",
    entityId: el.props?.entity_id || "",
    attribute: el.props?.attribute || "",
    unit: el.props?.unit || "",
    precision: el.props?.precision ?? "",
    buttonIndex: el.props?.button_index ?? 0,
    // Snapped, not taken verbatim: designs saved before the ladder existed can hold any
    // px value, and the device has no font for an off-ladder size.
    fontSize: snapFontSize(el.props?.fontSize || DEFAULT_FONT_SIZE),
    bold: !!el.props?.bold,
    align: el.props?.align || "left",
    underline: !!el.props?.underline,
    strikethrough: !!el.props?.strikethrough,
  }));
}

// Unique {entityId, attribute} refs for every Home Assistant-bound value element in a list of
// (already-normalized, camelCase) elements. Shared by anything that needs to resolve live values
// for a design's elements via useEntityValues.
export function haEntityRefs(elements) {
  const seen = new Set();
  const refs = [];
  for (const el of elements) {
    if (el.type !== "value" || el.source !== "home_assistant" || !el.entityId) continue;
    const key = `${el.entityId}::${el.attribute || ""}`;
    if (seen.has(key)) continue;
    seen.add(key);
    refs.push({ entityId: el.entityId, attribute: el.attribute });
  }
  return refs;
}
