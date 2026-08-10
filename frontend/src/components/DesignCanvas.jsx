import { useRef } from "react";
import { formatEntityValue } from "../lib/format.js";
import { DEFAULT_FONT_SIZE } from "../lib/layout.js";
import { entityValueKey } from "../lib/useEntityValues.js";
import GlyphText from "./GlyphText.jsx";

export function elementLabel(el, entityValues) {
  if (el.type === "text") return el.text || "(empty text)";
  if (el.source === "home_assistant") {
    if (!el.entityId) return "⌂ (no entity)";
    const cached = entityValues[entityValueKey(el.entityId, el.attribute)];
    if (!cached) return "⌂ …";
    return cached.error ? `⌂ ${cached.error}` : formatEntityValue(cached.value, el);
  }
  return el.value || "(empty value)";
}

function CanvasElement({ element, index, scale, selected, interactive, onSelect, onMove, entityValues }) {
  const dragRef = useRef(null);

  // Window-level mouse/touch listeners rather than pointer capture: works the same for
  // real users, but doesn't depend on the browser's active-pointer bookkeeping — more
  // robust against automated/synthetic input, and handles the pointer leaving the
  // element (or the window) mid-drag without losing the drag.
  const startDrag = (clientX, clientY, onMoveClient, onEnd) => {
    onSelect(index);
    dragRef.current = { startX: clientX, startY: clientY, origX: element.x, origY: element.y };

    const handleMove = (moveClientX, moveClientY) => {
      if (!dragRef.current) return;
      const dx = Math.round((moveClientX - dragRef.current.startX) / scale);
      const dy = Math.round((moveClientY - dragRef.current.startY) / scale);
      onMove(index, dragRef.current.origX + dx, dragRef.current.origY + dy);
    };
    const handleEnd = () => {
      dragRef.current = null;
      onEnd();
    };

    onMoveClient(handleMove, handleEnd);
  };

  const onMouseDown = (e) => {
    if (!interactive) return;
    e.stopPropagation();
    e.preventDefault();
    startDrag(e.clientX, e.clientY, (handleMove, handleEnd) => {
      const mouseMove = (moveEvent) => handleMove(moveEvent.clientX, moveEvent.clientY);
      const mouseUp = () => {
        handleEnd();
        window.removeEventListener("mousemove", mouseMove);
        window.removeEventListener("mouseup", mouseUp);
      };
      window.addEventListener("mousemove", mouseMove);
      window.addEventListener("mouseup", mouseUp);
    });
  };

  const onTouchStart = (e) => {
    if (!interactive) return;
    e.stopPropagation();
    const touch = e.touches[0];
    startDrag(touch.clientX, touch.clientY, (handleMove, handleEnd) => {
      const touchMove = (moveEvent) => {
        moveEvent.preventDefault();
        handleMove(moveEvent.touches[0].clientX, moveEvent.touches[0].clientY);
      };
      const touchEnd = () => {
        handleEnd();
        window.removeEventListener("touchmove", touchMove);
        window.removeEventListener("touchend", touchEnd);
      };
      window.addEventListener("touchmove", touchMove, { passive: false });
      window.addEventListener("touchend", touchEnd);
    });
  };

  return (
    <div
      className={`canvas-element ${element.type}${selected ? " selected" : ""}${interactive ? "" : " static"}`}
      style={{
        left: element.x * scale,
        top: element.y * scale,
        width: element.w * scale,
        height: element.h * scale,
      }}
      onMouseDown={onMouseDown}
      onTouchStart={onTouchStart}
    >
      <GlyphText
        text={elementLabel(element, entityValues)}
        fontSize={element.fontSize || DEFAULT_FONT_SIZE}
        bold={!!element.bold}
        align={element.align || "left"}
        underline={!!element.underline}
        strikethrough={!!element.strikethrough}
        width={element.w}
        height={element.h}
        scale={scale}
      />
    </div>
  );
}

// Renders a design's elements onto a scaled preview surface. `interactive=false` gives a static
// read-only thumbnail (Display cards); `interactive=true` keeps drag-to-reposition and selection
// (the design editor) — both modes share the same positioning/label logic so a display's
// thumbnail always matches what the editor shows.
export default function DesignCanvas({
  elements,
  width,
  height,
  cssWidth,
  entityValues = {},
  interactive = false,
  selectedIndex = null,
  onSelectIndex,
  onMoveElement,
  className = "",
}) {
  const scale = cssWidth / (width || 400);
  const cssHeight = (height || 300) * scale;

  return (
    <div
      className={`canvas-preview ${className}`}
      style={{ width: cssWidth, height: cssHeight }}
      onMouseDown={interactive ? () => onSelectIndex(null) : undefined}
    >
      {elements.map((el, i) => (
        <CanvasElement
          key={i}
          element={el}
          index={i}
          scale={scale}
          interactive={interactive}
          selected={interactive && i === selectedIndex}
          onSelect={onSelectIndex}
          onMove={onMoveElement}
          entityValues={entityValues}
        />
      ))}
    </div>
  );
}
