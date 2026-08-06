import { useEffect, useRef, useState } from "react";
import { api } from "../api/client.js";

const CANVAS_CSS_WIDTH = 480;

function emptyElement() {
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
  };
}

function toLayoutJson(elements) {
  return {
    elements: elements.map((el, i) => {
      let props;
      if (el.type === "text") {
        props = { text: el.text };
      } else if (el.source === "home_assistant") {
        props = { source: "home_assistant", entity_id: el.entityId };
        if (el.attribute) props.attribute = el.attribute;
      } else {
        props = { source: "static", value: el.value };
      }
      return { id: i + 1, type: el.type, x: el.x, y: el.y, w: el.w, h: el.h, props };
    }),
  };
}

function fromLayoutJson(layout) {
  return (layout.elements || []).map((el) => ({
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
  }));
}

function elementLabel(el) {
  if (el.type === "text") return el.text || "(empty text)";
  if (el.source === "home_assistant") return `⌂ ${el.entityId || "(no entity)"}`;
  return el.value || "(empty value)";
}

function DraggableElement({ element, index, scale, selected, onSelect, onMove }) {
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
      className={`canvas-element ${element.type}${selected ? " selected" : ""}`}
      style={{
        left: element.x * scale,
        top: element.y * scale,
        width: element.w * scale,
        height: element.h * scale,
      }}
      onMouseDown={onMouseDown}
      onTouchStart={onTouchStart}
    >
      {elementLabel(element)}
    </div>
  );
}

export default function DesignsPage() {
  const [designs, setDesigns] = useState([]);
  const [name, setName] = useState("");
  const [width, setWidth] = useState(400);
  const [height, setHeight] = useState(300);
  const [elements, setElements] = useState([]);
  const [selectedIndex, setSelectedIndex] = useState(null);
  const [draft, setDraft] = useState(emptyElement());
  const [error, setError] = useState(null);
  const [editingId, setEditingId] = useState(null);
  const [preview, setPreview] = useState(null);
  const [previewing, setPreviewing] = useState(false);

  const refresh = () => api.listDesigns().then(setDesigns).catch((e) => setError(e.message));

  useEffect(() => {
    refresh();
  }, []);

  const scale = CANVAS_CSS_WIDTH / (width || 400);
  const canvasCssHeight = (height || 300) * scale;

  const addElement = () => {
    setElements([...elements, draft]);
    setDraft(emptyElement());
    setPreview(null);
  };

  const removeElement = (index) => {
    setElements(elements.filter((_, i) => i !== index));
    if (selectedIndex === index) setSelectedIndex(null);
  };

  const moveElement = (index, x, y) => {
    setElements((prev) => {
      const next = [...prev];
      const el = next[index];
      const clampedX = Math.max(0, Math.min(x, width - el.w));
      const clampedY = Math.max(0, Math.min(y, height - el.h));
      next[index] = { ...el, x: clampedX, y: clampedY };
      return next;
    });
  };

  const updateSelected = (patch) => {
    if (selectedIndex === null) return;
    setElements((prev) => {
      const next = [...prev];
      next[selectedIndex] = { ...next[selectedIndex], ...patch };
      return next;
    });
  };

  const resetForm = () => {
    setEditingId(null);
    setName("");
    setWidth(400);
    setHeight(300);
    setElements([]);
    setSelectedIndex(null);
  };

  const save = async () => {
    setError(null);
    try {
      const payload = { name, width, height, layout_json: toLayoutJson(elements) };
      if (editingId) {
        await api.updateDesign(editingId, payload);
      } else {
        await api.createDesign(payload);
      }
      resetForm();
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const edit = (design) => {
    setEditingId(design.id);
    setName(design.name);
    setWidth(design.width);
    setHeight(design.height);
    setElements(fromLayoutJson(design.layout_json));
    setSelectedIndex(null);
  };

  const previewEntity = async () => {
    if (!draft.entityId) return;
    setPreviewing(true);
    setPreview(null);
    try {
      const result = await api.previewEntity(draft.entityId, draft.attribute || undefined);
      setPreview(result);
    } catch (e) {
      setPreview({ error: e.message });
    } finally {
      setPreviewing(false);
    }
  };

  const selected = selectedIndex !== null ? elements[selectedIndex] : null;

  return (
    <>
      <h2>Designs</h2>
      {error && <div className="error">{error}</div>}

      <div className="card">
        <div className="row">
          <div className="field">
            <label>Design name</label>
            <input value={name} onChange={(e) => setName(e.target.value)} placeholder="e.g. desk-status" />
          </div>
          <div className="field">
            <label>Canvas width</label>
            <input type="number" value={width} onChange={(e) => setWidth(+e.target.value)} style={{ width: 70 }} />
          </div>
          <div className="field">
            <label>Canvas height</label>
            <input type="number" value={height} onChange={(e) => setHeight(+e.target.value)} style={{ width: 70 }} />
          </div>
          <div className="muted">400×300 matches the only panel this system currently drives.</div>
        </div>

        <div className="row" style={{ alignItems: "flex-start", gap: 24 }}>
          <div style={{ flex: 1 }}>
            <div className="row">
              <div className="field">
                <label>Type</label>
                <select value={draft.type} onChange={(e) => setDraft({ ...draft, type: e.target.value })}>
                  <option value="text">text (static)</option>
                  <option value="value">value (bound)</option>
                </select>
              </div>
              {draft.type === "value" && (
                <div className="field">
                  <label>Source</label>
                  <select value={draft.source} onChange={(e) => setDraft({ ...draft, source: e.target.value })}>
                    <option value="static">static text</option>
                    <option value="home_assistant">Home Assistant entity</option>
                  </select>
                </div>
              )}
            </div>

            {draft.type === "text" && (
              <div className="field">
                <label>Text</label>
                <input value={draft.text} onChange={(e) => setDraft({ ...draft, text: e.target.value })} />
              </div>
            )}

            {draft.type === "value" && draft.source === "static" && (
              <div className="field">
                <label>Value</label>
                <input value={draft.value} onChange={(e) => setDraft({ ...draft, value: e.target.value })} />
              </div>
            )}

            {draft.type === "value" && draft.source === "home_assistant" && (
              <>
                <div className="row">
                  <div className="field">
                    <label>Entity ID</label>
                    <input
                      value={draft.entityId}
                      onChange={(e) => setDraft({ ...draft, entityId: e.target.value })}
                      placeholder="sensor.living_room_temperature"
                    />
                  </div>
                  <div className="field">
                    <label>Attribute (optional)</label>
                    <input
                      value={draft.attribute}
                      onChange={(e) => setDraft({ ...draft, attribute: e.target.value })}
                      placeholder="defaults to state"
                    />
                  </div>
                  <button type="button" className="secondary" onClick={previewEntity} disabled={!draft.entityId || previewing}>
                    {previewing ? "Checking…" : "Preview"}
                  </button>
                </div>
                {preview &&
                  (preview.error ? (
                    <div className="error">{preview.error}</div>
                  ) : (
                    <div className="muted">Current value: {preview.value}</div>
                  ))}
              </>
            )}

            <div className="row">
              <div className="field">
                <label>x</label>
                <input type="number" value={draft.x} onChange={(e) => setDraft({ ...draft, x: +e.target.value })} style={{ width: 60 }} />
              </div>
              <div className="field">
                <label>y</label>
                <input type="number" value={draft.y} onChange={(e) => setDraft({ ...draft, y: +e.target.value })} style={{ width: 60 }} />
              </div>
              <div className="field">
                <label>w</label>
                <input type="number" value={draft.w} onChange={(e) => setDraft({ ...draft, w: +e.target.value })} style={{ width: 60 }} />
              </div>
              <div className="field">
                <label>h</label>
                <input type="number" value={draft.h} onChange={(e) => setDraft({ ...draft, h: +e.target.value })} style={{ width: 60 }} />
              </div>
              <button type="button" className="secondary" onClick={addElement}>
                Add element
              </button>
            </div>

            <ul style={{ paddingLeft: 16, fontSize: 13 }}>
              {elements.map((el, i) => (
                <li
                  key={i}
                  onClick={() => setSelectedIndex(i)}
                  style={{ cursor: "pointer", fontWeight: i === selectedIndex ? 600 : 400 }}
                >
                  {el.type} "{elementLabel(el)}" @ ({el.x},{el.y})
                  <button
                    type="button"
                    className="secondary"
                    style={{ marginLeft: 8, padding: "2px 6px" }}
                    onClick={(e) => {
                      e.stopPropagation();
                      removeElement(i);
                    }}
                  >
                    remove
                  </button>
                </li>
              ))}
            </ul>

            {selected && (
              <div className="card">
                <div className="muted" style={{ marginBottom: 6 }}>
                  Editing selected element — drag it on the canvas to reposition
                </div>
                {selected.type === "text" ? (
                  <div className="field">
                    <label>Text</label>
                    <input value={selected.text} onChange={(e) => updateSelected({ text: e.target.value })} />
                  </div>
                ) : selected.source === "static" ? (
                  <div className="field">
                    <label>Value</label>
                    <input value={selected.value} onChange={(e) => updateSelected({ value: e.target.value })} />
                  </div>
                ) : (
                  <div className="field">
                    <label>Entity ID</label>
                    <input value={selected.entityId} onChange={(e) => updateSelected({ entityId: e.target.value })} />
                  </div>
                )}
                <div className="row">
                  <div className="field">
                    <label>w</label>
                    <input type="number" value={selected.w} onChange={(e) => updateSelected({ w: +e.target.value })} style={{ width: 60 }} />
                  </div>
                  <div className="field">
                    <label>h</label>
                    <input type="number" value={selected.h} onChange={(e) => updateSelected({ h: +e.target.value })} style={{ width: 60 }} />
                  </div>
                </div>
              </div>
            )}
          </div>

          <div>
            <div className="muted" style={{ marginBottom: 6 }}>
              Live preview ({width}×{height}) — drag elements to reposition
            </div>
            <div
              className="canvas-preview"
              style={{ width: CANVAS_CSS_WIDTH, height: canvasCssHeight }}
              onMouseDown={() => setSelectedIndex(null)}
            >
              {elements.map((el, i) => (
                <DraggableElement
                  key={i}
                  element={el}
                  index={i}
                  scale={scale}
                  selected={i === selectedIndex}
                  onSelect={setSelectedIndex}
                  onMove={moveElement}
                />
              ))}
            </div>
          </div>
        </div>

        <button type="button" onClick={save} disabled={!name || elements.length === 0} style={{ marginTop: 12 }}>
          {editingId ? "Save changes" : "Create design"}
        </button>
        {editingId && (
          <button type="button" className="secondary" style={{ marginLeft: 8 }} onClick={resetForm}>
            Cancel edit
          </button>
        )}
      </div>

      <div className="card">
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>Resolution</th>
              <th>Elements</th>
              <th>Updated</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {designs.map((d) => (
              <tr key={d.id}>
                <td>{d.id}</td>
                <td>{d.name}</td>
                <td>
                  {d.width}×{d.height}
                </td>
                <td>{(d.layout_json.elements || []).length}</td>
                <td>{new Date(d.updated_at).toLocaleString()}</td>
                <td>
                  <button type="button" className="secondary" onClick={() => edit(d)}>
                    Edit
                  </button>
                </td>
              </tr>
            ))}
            {designs.length === 0 && (
              <tr>
                <td colSpan={6} className="muted">
                  No designs yet.
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </>
  );
}
