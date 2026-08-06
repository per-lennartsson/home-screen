import { useEffect, useMemo, useState } from "react";
import { api } from "../api/client.js";
import DesignCanvas, { elementLabel } from "../components/DesignCanvas.jsx";
import { relativeTime } from "../lib/format.js";
import { DEFAULT_FONT_SIZE, emptyElement, fromLayoutJson, haEntityRefs, toLayoutJson } from "../lib/layout.js";
import { entityValueKey, useEntityValues } from "../lib/useEntityValues.js";

const CANVAS_CSS_WIDTH = 480;

function snapshotOf(name, width, height, elements) {
  return JSON.stringify({ name, width, height, elements });
}

export default function DesignsPage() {
  const [designs, setDesigns] = useState([]);
  const [displays, setDisplays] = useState([]);
  const [error, setError] = useState(null);
  const [view, setView] = useState("list");

  const [editingId, setEditingId] = useState(null);
  const [name, setName] = useState("");
  const [width, setWidth] = useState(400);
  const [height, setHeight] = useState(300);
  const [elements, setElements] = useState([]);
  const [snapshot, setSnapshot] = useState(snapshotOf("", 400, 300, []));
  const [selectedIndex, setSelectedIndex] = useState(null);
  const [snap, setSnap] = useState(true);
  const [editorTab, setEditorTab] = useState("edit");

  const refresh = () => {
    api.listDesigns().then(setDesigns).catch((e) => setError(e.message));
    api.listDisplays().then(setDisplays).catch(() => {});
  };

  useEffect(() => {
    refresh();
  }, []);

  const entityRefs = useMemo(() => haEntityRefs(elements), [elements]);
  const [entityValues] = useEntityValues(entityRefs);

  const dirty = snapshotOf(name, width, height, elements) !== snapshot;
  const scale = CANVAS_CSS_WIDTH / (width || 400);

  const openNew = () => {
    setEditingId(null);
    setName("");
    setWidth(400);
    setHeight(300);
    setElements([]);
    setSnapshot(snapshotOf("", 400, 300, []));
    setSelectedIndex(null);
    setEditorTab("edit");
    setView("editor");
  };

  const openEdit = (design) => {
    const els = fromLayoutJson(design.layout_json);
    setEditingId(design.id);
    setName(design.name);
    setWidth(design.width);
    setHeight(design.height);
    setElements(els);
    setSnapshot(snapshotOf(design.name, design.width, design.height, els));
    setSelectedIndex(null);
    setEditorTab("edit");
    setView("editor");
  };

  const backToList = () => {
    if (dirty && !window.confirm("Discard unsaved changes to this design?")) return;
    setView("list");
    refresh();
  };

  const discard = () => {
    const parsed = JSON.parse(snapshot);
    setName(parsed.name);
    setWidth(parsed.width);
    setHeight(parsed.height);
    setElements(parsed.elements);
    setSelectedIndex(null);
  };

  const save = async () => {
    setError(null);
    try {
      const payload = { name, width, height, layout_json: toLayoutJson(elements) };
      let saved;
      if (editingId) {
        saved = await api.updateDesign(editingId, payload);
      } else {
        saved = await api.createDesign(payload);
      }
      setEditingId(saved.id);
      setSnapshot(snapshotOf(name, width, height, elements));
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const addElement = (type) => {
    const el = { ...emptyElement(), type };
    setElements((prev) => {
      setSelectedIndex(prev.length);
      return [...prev, el];
    });
  };

  const removeElement = (index) => {
    setElements((prev) => prev.filter((_, i) => i !== index));
    if (selectedIndex === index) setSelectedIndex(null);
    else if (selectedIndex != null && index < selectedIndex) setSelectedIndex(selectedIndex - 1);
  };

  const moveElement = (index, x, y) => {
    setElements((prev) => {
      const next = [...prev];
      const el = next[index];
      let nx = x;
      let ny = y;
      if (snap) {
        nx = Math.round(nx / 4) * 4;
        ny = Math.round(ny / 4) * 4;
      }
      nx = Math.max(0, Math.min(nx, width - el.w));
      ny = Math.max(0, Math.min(ny, height - el.h));
      next[index] = { ...el, x: nx, y: ny };
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

  const align = (edge) => {
    if (selectedIndex === null) return;
    const el = elements[selectedIndex];
    let x;
    if (edge === "left") x = 0;
    else if (edge === "right") x = width - el.w;
    else x = Math.round((width - el.w) / 2);
    updateSelected({ x: Math.max(0, x) });
  };

  const selected = selectedIndex !== null ? elements[selectedIndex] : null;
  const usedByCount = editingId ? displays.filter((d) => d.design_id === editingId).length : 0;

  if (view === "list") {
    return (
      <>
        <header className="page-header">
          <div className="page-header-titles">
            <h1>Designs</h1>
            <span className="page-header-meta">
              {designs.length} design{designs.length === 1 ? "" : "s"}
            </span>
          </div>
          <div className="page-header-actions">
            <button type="button" className="btn" onClick={openNew}>
              New design
            </button>
          </div>
        </header>
        <div className="page-body">
          {error && <div className="error">{error}</div>}
          <div className="card-grid narrow">
            {designs.map((d) => (
              <div key={d.id} className="entity-card">
                <div className="entity-card-head">
                  <div>
                    <div className="entity-card-title">{d.name}</div>
                    <div className="entity-card-sub">
                      {d.width}×{d.height} · {(d.layout_json.elements || []).length} element
                      {(d.layout_json.elements || []).length === 1 ? "" : "s"}
                    </div>
                  </div>
                </div>
                <div className="card-stats" style={{ marginTop: 0 }}>
                  <div>
                    <div className="card-stat-label">Updated</div>
                    <div className="card-stat-value">{relativeTime(d.updated_at)}</div>
                  </div>
                </div>
                <div className="entity-card-footer">
                  <button type="button" className="btn secondary small" onClick={() => openEdit(d)}>
                    Edit
                  </button>
                </div>
              </div>
            ))}
          </div>
          {designs.length === 0 && <div className="muted">No designs yet.</div>}
        </div>
      </>
    );
  }

  return (
    <>
      <header className="page-header">
        <div className="breadcrumb">
          <a
            href="#"
            onClick={(e) => {
              e.preventDefault();
              backToList();
            }}
          >
            Designs
          </a>
          <span className="breadcrumb-sep">/</span>
          <input className="name-input" value={name} onChange={(e) => setName(e.target.value)} placeholder="design name" />
          <span className="chip">
            {width} × {height}
          </span>
        </div>
        <div className="page-header-actions">
          {dirty && <span className="unsaved-flag">Unsaved changes</span>}
          <button type="button" className="btn ghost" onClick={discard} disabled={!dirty}>
            Discard
          </button>
          <button type="button" className="btn" onClick={save} disabled={!name || elements.length === 0}>
            Save &amp; push
          </button>
        </div>
      </header>

      {error && (
        <div className="error" style={{ padding: "8px 28px 0" }}>
          {error}
        </div>
      )}

      <div className="editor-shell">
        <aside className="editor-aside-left">
          <div className="editor-add-row">
            <button type="button" className="btn secondary small" onClick={() => addElement("text")}>
              ＋ Text
            </button>
            <button type="button" className="btn secondary small" onClick={() => addElement("value")}>
              ＋ Value
            </button>
          </div>
          <div className="sidebar-section-label">Layers</div>
          <div className="layers-list">
            {elements.map((el, i) => {
              const cached =
                el.type === "value" && el.source === "home_assistant" && el.entityId
                  ? entityValues[entityValueKey(el.entityId, el.attribute)]
                  : null;
              return (
                <div
                  key={i}
                  className={`layer-item${i === selectedIndex ? " selected" : ""}`}
                  onClick={() => setSelectedIndex(i)}
                >
                  <span className={`layer-icon${cached?.error ? " warn" : ""}`}>{el.type === "text" ? "T" : "◈"}</span>
                  <div style={{ minWidth: 0, flex: 1 }}>
                    <div className="layer-title">{elementLabel(el, entityValues)}</div>
                    {el.type === "value" && el.source === "home_assistant" ? (
                      <div className="layer-sub">{el.entityId || "no entity"}</div>
                    ) : (
                      <div className="layer-sub">{el.type === "text" ? "Static text" : "Static value"}</div>
                    )}
                  </div>
                  <button
                    type="button"
                    className="btn ghost"
                    style={{ padding: "2px 6px" }}
                    onClick={(e) => {
                      e.stopPropagation();
                      removeElement(i);
                    }}
                  >
                    ✕
                  </button>
                </div>
              );
            })}
            {elements.length === 0 && <div className="muted" style={{ padding: "8px 9px" }}>No elements yet.</div>}
          </div>
        </aside>

        <div className="editor-canvas-area">
          <div className="editor-toolbar">
            <div className="tabbar" style={{ flexShrink: 0, width: "auto" }}>
              <button type="button" className={`tab${editorTab === "edit" ? " active" : ""}`} onClick={() => setEditorTab("edit")}>
                Edit
              </button>
              <button type="button" className={`tab${editorTab === "preview" ? " active" : ""}`} onClick={() => setEditorTab("preview")}>
                Preview
              </button>
            </div>
            <div className="editor-toolbar-info">
              <label className="snap-toggle">
                <input type="checkbox" checked={snap} onChange={(e) => setSnap(e.target.checked)} />
                Snap to 4 px grid
              </label>
              <span className="sep">|</span>
              <span>Live values from Home Assistant</span>
            </div>
          </div>

          <div className="canvas-scroll">
            <div>
              <div className="canvas-frame">
                <DesignCanvas
                  elements={elements}
                  width={width}
                  height={height}
                  cssWidth={CANVAS_CSS_WIDTH}
                  entityValues={entityValues}
                  interactive={editorTab === "edit"}
                  selectedIndex={selectedIndex}
                  onSelectIndex={setSelectedIndex}
                  onMoveElement={moveElement}
                />
              </div>
              <div className="canvas-meta-row">
                <span>
                  {width} × {height} px · 1-bit greyscale
                  {editingId ? ` · used by ${usedByCount} display${usedByCount === 1 ? "" : "s"}` : " · not saved yet"}
                </span>
                <span>Shown at {Math.round(scale * 100)}%</span>
              </div>
            </div>
          </div>
        </div>

        <aside className="editor-aside-right">
          {!selected ? (
            <div className="muted" style={{ padding: 16 }}>
              Select an element to edit its properties, or add one from the left.
            </div>
          ) : (
            <>
              <div className="inspector-head">
                <div className="inspector-head-title">{selected.type === "text" ? "Text element" : "Value element"}</div>
                <button type="button" className="btn ghost" onClick={() => removeElement(selectedIndex)}>
                  Delete
                </button>
              </div>

              {selected.type === "text" ? (
                <div className="inspector-section">
                  <div className="inspector-label">Content</div>
                  <input value={selected.text} onChange={(e) => updateSelected({ text: e.target.value })} />
                </div>
              ) : (
                <div className="inspector-section">
                  <div className="inspector-label">Source</div>
                  <div className="tabbar">
                    <button
                      type="button"
                      className={`tab${selected.source === "static" ? " active" : ""}`}
                      onClick={() => updateSelected({ source: "static" })}
                    >
                      Static
                    </button>
                    <button
                      type="button"
                      className={`tab${selected.source === "home_assistant" ? " active" : ""}`}
                      onClick={() => updateSelected({ source: "home_assistant" })}
                    >
                      Home Assistant
                    </button>
                  </div>

                  {selected.source === "static" ? (
                    <div className="field" style={{ marginBottom: 0 }}>
                      <label>Value</label>
                      <input value={selected.value} onChange={(e) => updateSelected({ value: e.target.value })} />
                    </div>
                  ) : (
                    <>
                      <div className="field" style={{ marginBottom: 0 }}>
                        <label>Entity</label>
                        <input
                          className="entity-lookup"
                          value={selected.entityId}
                          onChange={(e) => updateSelected({ entityId: e.target.value })}
                          placeholder="sensor.study_temperature"
                        />
                      </div>
                      <div className="field" style={{ marginBottom: 0 }}>
                        <label>Attribute (optional)</label>
                        <input
                          value={selected.attribute}
                          onChange={(e) => updateSelected({ attribute: e.target.value })}
                          placeholder="defaults to state"
                        />
                      </div>
                      {selected.entityId &&
                        (() => {
                          const cached = entityValues[entityValueKey(selected.entityId, selected.attribute)];
                          if (!cached) return <div className="inspector-live">Checking…</div>;
                          if (cached.error) return <div className="error" style={{ margin: 0 }}>{cached.error}</div>;
                          return (
                            <div className="inspector-live">
                              <span className="status-dot ok" />
                              <span>
                                Reads {cached.value} · updated {relativeTime(cached.fetchedAt)}
                              </span>
                            </div>
                          );
                        })()}
                    </>
                  )}
                </div>
              )}

              <div className="inspector-section">
                <div className="inspector-label">Position &amp; size</div>
                <div className="pos-grid">
                  <div className="pos-field">
                    <span className="axis">X</span>
                    <input type="number" value={selected.x} onChange={(e) => updateSelected({ x: +e.target.value })} />
                  </div>
                  <div className="pos-field">
                    <span className="axis">Y</span>
                    <input type="number" value={selected.y} onChange={(e) => updateSelected({ y: +e.target.value })} />
                  </div>
                  <div className="pos-field">
                    <span className="axis">W</span>
                    <input type="number" value={selected.w} onChange={(e) => updateSelected({ w: +e.target.value })} />
                  </div>
                  <div className="pos-field">
                    <span className="axis">H</span>
                    <input type="number" value={selected.h} onChange={(e) => updateSelected({ h: +e.target.value })} />
                  </div>
                </div>
                <div className="align-row">
                  <button type="button" className="btn secondary small" onClick={() => align("left")}>
                    Left
                  </button>
                  <button type="button" className="btn secondary small" onClick={() => align("centre")}>
                    Centre
                  </button>
                  <button type="button" className="btn secondary small" onClick={() => align("right")}>
                    Right
                  </button>
                </div>
              </div>

              <div className="inspector-section">
                <div className="inspector-label">Type</div>
                <div className="inspector-row">
                  <label className="muted">Size</label>
                  <div className="stepper">
                    <button type="button" onClick={() => updateSelected({ fontSize: Math.max(8, (selected.fontSize || DEFAULT_FONT_SIZE) - 2) })}>
                      −
                    </button>
                    <span className="value">{selected.fontSize || DEFAULT_FONT_SIZE}</span>
                    <button type="button" onClick={() => updateSelected({ fontSize: Math.min(72, (selected.fontSize || DEFAULT_FONT_SIZE) + 2) })}>
                      ＋
                    </button>
                  </div>
                </div>
                <div className="inspector-row">
                  <label className="muted">Weight</label>
                  <div className="tabbar">
                    <button type="button" className={`tab${!selected.bold ? " active" : ""}`} onClick={() => updateSelected({ bold: false })}>
                      Regular
                    </button>
                    <button type="button" className={`tab${selected.bold ? " active" : ""}`} onClick={() => updateSelected({ bold: true })}>
                      Bold
                    </button>
                  </div>
                </div>
                <div className="inspector-row">
                  <label className="muted">Align</label>
                  <div className="tabbar">
                    <button
                      type="button"
                      className={`tab${(selected.align || "left") === "left" ? " active" : ""}`}
                      onClick={() => updateSelected({ align: "left" })}
                    >
                      Left
                    </button>
                    <button
                      type="button"
                      className={`tab${selected.align === "center" ? " active" : ""}`}
                      onClick={() => updateSelected({ align: "center" })}
                    >
                      Center
                    </button>
                    <button
                      type="button"
                      className={`tab${selected.align === "right" ? " active" : ""}`}
                      onClick={() => updateSelected({ align: "right" })}
                    >
                      Right
                    </button>
                  </div>
                </div>
                <div className="inspector-row">
                  <label className="muted">Style</label>
                  <div className="tabbar">
                    <button
                      type="button"
                      className={`tab${selected.underline ? " active" : ""}`}
                      onClick={() => updateSelected({ underline: !selected.underline })}
                    >
                      Underline
                    </button>
                    <button
                      type="button"
                      className={`tab${selected.strikethrough ? " active" : ""}`}
                      onClick={() => updateSelected({ strikethrough: !selected.strikethrough })}
                    >
                      Strikethrough
                    </button>
                  </div>
                </div>
              </div>
            </>
          )}
        </aside>
      </div>
    </>
  );
}
