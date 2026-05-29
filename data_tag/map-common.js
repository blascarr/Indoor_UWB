/* Utilidades compartidas para vistas de mapa (/map, /map2D, /map3D). */

const MAP_POLL_MS = 300;
const MAP_TRAIL_MAX = 40;

async function fetchMapData() {
  const res = await fetch('/api/map', { cache: 'no-store' });
  if (!res.ok) {
    throw new Error(`HTTP ${res.status}`);
  }
  return res.json();
}

function computeViewBounds(data, paddingM = 1) {
  let minX = 0;
  let maxX = 0;
  let minY = 0;
  let maxY = 0;
  let init = false;

  const consider = (x, y) => {
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return;
    }
    if (!init) {
      minX = maxX = x;
      minY = maxY = y;
      init = true;
    } else {
      minX = Math.min(minX, x);
      maxX = Math.max(maxX, x);
      minY = Math.min(minY, y);
      maxY = Math.max(maxY, y);
    }
  };

  (data.anchors || []).forEach((a) => consider(a.x, a.y));
  if (data.position?.valid) {
    consider(data.position.x, data.position.y);
  }
  if (data.bounds) {
    return {
      minX: data.bounds.minX,
      maxX: data.bounds.maxX,
      minY: data.bounds.minY,
      maxY: data.bounds.maxY,
    };
  }
  if (!init) {
    return { minX: -2, maxX: 2, minY: -2, maxY: 2 };
  }
  return {
    minX: minX - paddingM,
    maxX: maxX + paddingM,
    minY: minY - paddingM,
    maxY: maxY + paddingM,
  };
}

class MapViewport {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.bounds = { minX: -2, maxX: 2, minY: -2, maxY: 2 };
    this.zoom = 1;
    this.panX = 0;
    this.panY = 0;
    this.dragging = false;
    this.lastX = 0;
    this.lastY = 0;
    this._bindEvents();
  }

  _bindEvents() {
    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const factor = e.deltaY > 0 ? 0.9 : 1.1;
      this.zoom = Math.min(8, Math.max(0.2, this.zoom * factor));
    });
    this.canvas.addEventListener('mousedown', (e) => {
      this.dragging = true;
      this.lastX = e.clientX;
      this.lastY = e.clientY;
    });
    window.addEventListener('mouseup', () => {
      this.dragging = false;
    });
    window.addEventListener('mousemove', (e) => {
      if (!this.dragging) {
        return;
      }
      this.panX += e.clientX - this.lastX;
      this.panY += e.clientY - this.lastY;
      this.lastX = e.clientX;
      this.lastY = e.clientY;
    });
    this.canvas.addEventListener(
      'touchstart',
      (e) => {
        if (e.touches.length !== 1) {
          return;
        }
        this.dragging = true;
        this.lastX = e.touches[0].clientX;
        this.lastY = e.touches[0].clientY;
      },
      { passive: true },
    );
    this.canvas.addEventListener(
      'touchmove',
      (e) => {
        if (!this.dragging || e.touches.length !== 1) {
          return;
        }
        this.panX += e.touches[0].clientX - this.lastX;
        this.panY += e.touches[0].clientY - this.lastY;
        this.lastX = e.touches[0].clientX;
        this.lastY = e.touches[0].clientY;
      },
      { passive: true },
    );
    window.addEventListener('touchend', () => {
      this.dragging = false;
    });
  }

  resize() {
    const rect = this.canvas.parentElement.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.canvas.width = Math.floor(rect.width * dpr);
    this.canvas.height = Math.floor(rect.height * dpr);
    this.canvas.style.width = `${rect.width}px`;
    this.canvas.style.height = `${rect.height}px`;
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.w = rect.width;
    this.h = rect.height;
  }

  setBounds(bounds) {
    this.bounds = bounds;
  }

  resetView() {
    this.zoom = 1;
    this.panX = 0;
    this.panY = 0;
  }

  worldToScreen(x, y) {
    const { minX, maxX, minY, maxY } = this.bounds;
    const spanX = maxX - minX || 1;
    const spanY = maxY - minY || 1;
    const scale = Math.min(this.w / spanX, this.h / spanY) * 0.85 * this.zoom;
    const cx = (minX + maxX) / 2;
    const cy = (minY + maxY) / 2;
    const sx = this.w / 2 + (x - cx) * scale + this.panX;
    const sy = this.h / 2 - (y - cy) * scale + this.panY;
    return { x: sx, y: sy, scale };
  }

  metersToPixels(m) {
    const { minX, maxX, minY, maxY } = this.bounds;
    const spanX = maxX - minX || 1;
    const spanY = maxY - minY || 1;
    const scale = Math.min(this.w / spanX, this.h / spanY) * 0.85 * this.zoom;
    return m * scale;
  }

  clear() {
    this.ctx.fillStyle = '#0b1220';
    this.ctx.fillRect(0, 0, this.w, this.h);
  }

  drawGrid() {
    const { minX, maxX, minY, maxY } = this.bounds;
    const step = pickGridStep(Math.max(maxX - minX, maxY - minY));
    this.ctx.strokeStyle = '#1e293b';
    this.ctx.lineWidth = 1;
    for (let x = Math.floor(minX / step) * step; x <= maxX; x += step) {
      const a = this.worldToScreen(x, minY);
      const b = this.worldToScreen(x, maxY);
      this.ctx.beginPath();
      this.ctx.moveTo(a.x, a.y);
      this.ctx.lineTo(b.x, b.y);
      this.ctx.stroke();
    }
    for (let y = Math.floor(minY / step) * step; y <= maxY; y += step) {
      const a = this.worldToScreen(minX, y);
      const b = this.worldToScreen(maxX, y);
      this.ctx.beginPath();
      this.ctx.moveTo(a.x, a.y);
      this.ctx.lineTo(b.x, b.y);
      this.ctx.stroke();
    }
  }

  drawOrigin(origin) {
    const o = origin || { x: 0, y: 0 };
    const p = this.worldToScreen(o.x, o.y);
    const len = 28;
    this.ctx.lineWidth = 2;
    this.ctx.strokeStyle = '#ef4444';
    this.ctx.beginPath();
    this.ctx.moveTo(p.x, p.y);
    this.ctx.lineTo(p.x + len, p.y);
    this.ctx.stroke();
    this.ctx.strokeStyle = '#22c55e';
    this.ctx.beginPath();
    this.ctx.moveTo(p.x, p.y);
    this.ctx.lineTo(p.x, p.y - len);
    this.ctx.stroke();
    this.ctx.fillStyle = '#f8fafc';
    this.ctx.font = '11px system-ui,sans-serif';
    this.ctx.fillText('Origen', p.x + 6, p.y - 6);
    this.ctx.beginPath();
    this.ctx.arc(p.x, p.y, 4, 0, Math.PI * 2);
    this.ctx.fillStyle = '#fbbf24';
    this.ctx.fill();
  }

  drawAnchor(anchor) {
    const p = this.worldToScreen(anchor.x, anchor.y);
    const r = anchor.isOrigin ? 9 : 7;
    this.ctx.beginPath();
    this.ctx.arc(p.x, p.y, r, 0, Math.PI * 2);
    this.ctx.fillStyle = anchor.active ? '#3b82f6' : '#475569';
    this.ctx.fill();
    this.ctx.strokeStyle = anchor.isOrigin ? '#fbbf24' : '#93c5fd';
    this.ctx.lineWidth = anchor.isOrigin ? 2.5 : 1.5;
    this.ctx.stroke();
    this.ctx.fillStyle = '#e2e8f0';
    this.ctx.font = '12px system-ui,sans-serif';
    this.ctx.fillText(anchor.name || 'Anchor', p.x + 10, p.y - 8);

    if (anchor.liveRange > 0) {
      const rad = this.metersToPixels(anchor.liveRange);
      this.ctx.beginPath();
      this.ctx.arc(p.x, p.y, rad, 0, Math.PI * 2);
      this.ctx.strokeStyle = 'rgba(59,130,246,0.25)';
      this.ctx.lineWidth = 1;
      this.ctx.stroke();
    }
  }

  drawTag(position, trail) {
    if (trail?.length > 1) {
      this.ctx.strokeStyle = 'rgba(34,197,94,0.45)';
      this.ctx.lineWidth = 2;
      this.ctx.beginPath();
      trail.forEach((pt, i) => {
        const p = this.worldToScreen(pt.x, pt.y);
        if (i === 0) {
          this.ctx.moveTo(p.x, p.y);
        } else {
          this.ctx.lineTo(p.x, p.y);
        }
      });
      this.ctx.stroke();
    }
    if (!position?.valid) {
      return;
    }
    const p = this.worldToScreen(position.x, position.y);
    this.ctx.beginPath();
    this.ctx.arc(p.x, p.y, 8, 0, Math.PI * 2);
    this.ctx.fillStyle = '#22c55e';
    this.ctx.fill();
    this.ctx.strokeStyle = '#bbf7d0';
    this.ctx.lineWidth = 2;
    this.ctx.stroke();
    this.ctx.fillStyle = '#bbf7d0';
    this.ctx.font = '11px system-ui,sans-serif';
    this.ctx.fillText('Tag', p.x + 10, p.y + 4);
  }
}

function pickGridStep(span) {
  if (span <= 4) {
    return 0.5;
  }
  if (span <= 10) {
    return 1;
  }
  if (span <= 25) {
    return 2;
  }
  return 5;
}

function formatPositionStatus(data) {
  const p = data.position;
  if (!p?.valid) {
    return `Sin posición (${p?.anchorsUsed || 0} anchors UWB)`;
  }
  return `Tag: ${p.x.toFixed(2)}, ${p.y.toFixed(2)}, ${p.z.toFixed(2)} m · residual ${p.residual?.toFixed(3) ?? '—'} m`;
}

function pushTrail(trail, position) {
  if (!position?.valid) {
    return trail;
  }
  const next = [...trail, { x: position.x, y: position.y }];
  if (next.length > MAP_TRAIL_MAX) {
    next.shift();
  }
  return next;
}

if (typeof window !== 'undefined') {
  window.MAP_POLL_MS = MAP_POLL_MS;
  window.MAP_TRAIL_MAX = MAP_TRAIL_MAX;
  window.fetchMapData = fetchMapData;
  window.computeViewBounds = computeViewBounds;
  window.MapViewport = MapViewport;
  window.formatPositionStatus = formatPositionStatus;
  window.pushTrail = pushTrail;
}
