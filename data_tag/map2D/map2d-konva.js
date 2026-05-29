(function () {
  const statusEl = document.getElementById('status');
  const root = document.getElementById('konva-root');
  const autoRefreshEl = document.getElementById('auto-refresh');
  const btnReset = document.getElementById('btn-reset');
  const nav3d = document.getElementById('nav-3d');
  const banner = document.getElementById('banner');

  if (typeof Konva === 'undefined') {
    statusEl.textContent = 'Konva no disponible (sin CDN).';
    return;
  }

  let stage;
  let worldLayer;
  let uiLayer;
  let pollTimer = null;
  let lastData = null;
  let trail = [];
  let pan = { x: 0, y: 0 };
  let scale = 1;

  function stageSize() {
    const rect = root.getBoundingClientRect();
    return { width: rect.width, height: rect.height };
  }

  function initStage() {
    const size = stageSize();
    stage = new Konva.Stage({
      container: root,
      width: size.width,
      height: size.height,
      draggable: true,
    });
    worldLayer = new Konva.Layer();
    uiLayer = new Konva.Layer();
    stage.add(worldLayer);
    stage.add(uiLayer);
  }

  function worldToStage(x, y, bounds, w, h) {
    const spanX = bounds.maxX - bounds.minX || 1;
    const spanY = bounds.maxY - bounds.minY || 1;
    const s = Math.min(w / spanX, h / spanY) * 0.85 * scale;
    const cx = (bounds.minX + bounds.maxX) / 2;
    const cy = (bounds.minY + bounds.maxY) / 2;
    return {
      x: w / 2 + (x - cx) * s + pan.x,
      y: h / 2 - (y - cy) * s + pan.y,
      s,
    };
  }

  function render() {
    if (!lastData || !stage) {
      return;
    }
    const size = stageSize();
    stage.width(size.width);
    stage.height(size.height);
    worldLayer.destroyChildren();
    uiLayer.destroyChildren();

    const bounds = computeViewBounds(lastData);
    const bg = new Konva.Rect({
      x: 0,
      y: 0,
      width: size.width,
      height: size.height,
      fill: '#0b1220',
    });
    worldLayer.add(bg);

    const origin = lastData.origin || { x: 0, y: 0 };
    const op = worldToStage(origin.x, origin.y, bounds, size.width, size.height);
    const axisLen = 30;
    worldLayer.add(new Konva.Arrow({
      points: [op.x, op.y, op.x + axisLen, op.y],
      stroke: '#ef4444',
      fill: '#ef4444',
      strokeWidth: 2,
    }));
    worldLayer.add(new Konva.Arrow({
      points: [op.x, op.y, op.x, op.y - axisLen],
      stroke: '#22c55e',
      fill: '#22c55e',
      strokeWidth: 2,
    }));
    worldLayer.add(new Konva.Circle({
      x: op.x,
      y: op.y,
      radius: 4,
      fill: '#fbbf24',
    }));
    uiLayer.add(new Konva.Text({
      x: op.x + 6,
      y: op.y - 18,
      text: 'Origen',
      fontSize: 11,
      fill: '#e2e8f0',
    }));

    (lastData.anchors || []).forEach((anchor) => {
      const p = worldToStage(anchor.x, anchor.y, bounds, size.width, size.height);
      if (anchor.liveRange > 0) {
        worldLayer.add(new Konva.Circle({
          x: p.x,
          y: p.y,
          radius: anchor.liveRange * p.s,
          stroke: 'rgba(59,130,246,0.35)',
          strokeWidth: 1,
        }));
      }
      worldLayer.add(new Konva.Circle({
        x: p.x,
        y: p.y,
        radius: anchor.isOrigin ? 9 : 7,
        fill: anchor.active ? '#3b82f6' : '#475569',
        stroke: anchor.isOrigin ? '#fbbf24' : '#93c5fd',
        strokeWidth: anchor.isOrigin ? 2.5 : 1.5,
      }));
      uiLayer.add(new Konva.Text({
        x: p.x + 10,
        y: p.y - 16,
        text: anchor.name || 'Anchor',
        fontSize: 12,
        fill: '#e2e8f0',
      }));
    });

    if (trail.length > 1) {
      const pts = [];
      trail.forEach((pt) => {
        const p = worldToStage(pt.x, pt.y, bounds, size.width, size.height);
        pts.push(p.x, p.y);
      });
      worldLayer.add(new Konva.Line({
        points: pts,
        stroke: 'rgba(34,197,94,0.5)',
        strokeWidth: 2,
        lineCap: 'round',
        lineJoin: 'round',
      }));
    }

    if (lastData.position?.valid) {
      const tp = worldToStage(
        lastData.position.x,
        lastData.position.y,
        bounds,
        size.width,
        size.height,
      );
      worldLayer.add(new Konva.Circle({
        x: tp.x,
        y: tp.y,
        radius: 8,
        fill: '#22c55e',
        stroke: '#bbf7d0',
        strokeWidth: 2,
      }));
      uiLayer.add(new Konva.Text({
        x: tp.x + 10,
        y: tp.y,
        text: 'Tag',
        fontSize: 11,
        fill: '#bbf7d0',
      }));
    }

    worldLayer.batchDraw();
    uiLayer.batchDraw();
  }

  async function refresh() {
    try {
      lastData = await fetchMapData();
      if (lastData.trilateration_mode !== '3d' && nav3d) {
        nav3d.hidden = true;
      }
      trail = pushTrail(trail, lastData.position);
      statusEl.textContent = formatPositionStatus(lastData);
      banner.classList.add('hidden');
      render();
    } catch (err) {
      statusEl.textContent = `Error: ${err.message}`;
      banner.textContent = 'No se pudo obtener /api/map.';
      banner.classList.remove('hidden');
    }
  }

  function startPoll() {
    if (pollTimer) {
      clearInterval(pollTimer);
    }
    if (autoRefreshEl.checked) {
      pollTimer = setInterval(refresh, MAP_POLL_MS);
    }
  }

  btnReset.addEventListener('click', () => {
    pan = { x: 0, y: 0 };
    scale = 1;
    stage.position({ x: 0, y: 0 });
    stage.scale({ x: 1, y: 1 });
    render();
  });

  autoRefreshEl.addEventListener('change', startPoll);
  window.addEventListener('resize', render);

  initStage();
  stage.on('wheel', (e) => {
    e.evt.preventDefault();
    const dir = e.evt.deltaY > 0 ? -1 : 1;
    scale = Math.min(4, Math.max(0.3, scale + dir * 0.08));
    render();
  });

  refresh();
  startPoll();
})();
