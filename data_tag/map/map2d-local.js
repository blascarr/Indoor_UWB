(function () {
  const statusEl = document.getElementById('status');
  const canvas = document.getElementById('map-canvas');
  const autoRefreshEl = document.getElementById('auto-refresh');
  const btnReset = document.getElementById('btn-reset');
  const nav3d = document.getElementById('nav-3d');
  const banner = document.getElementById('banner');

  const viewport = new MapViewport(canvas);
  let trail = [];
  let pollTimer = null;
  let lastData = null;

  function render() {
    if (!lastData) {
      return;
    }
    viewport.resize();
    viewport.setBounds(computeViewBounds(lastData));
    viewport.clear();
    viewport.drawGrid();
    viewport.drawOrigin(lastData.origin);
    (lastData.anchors || []).forEach((a) => viewport.drawAnchor(a));
    viewport.drawTag(lastData.position, trail);
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
      banner.textContent = 'No se pudo obtener /api/map. Comprueba que el tag esté en línea.';
      banner.classList.remove('hidden');
    }
  }

  function startPoll() {
    stopPoll();
    if (autoRefreshEl.checked) {
      pollTimer = setInterval(refresh, MAP_POLL_MS);
    }
  }

  function stopPoll() {
    if (pollTimer) {
      clearInterval(pollTimer);
      pollTimer = null;
    }
  }

  btnReset.addEventListener('click', () => {
    viewport.resetView();
    render();
  });

  autoRefreshEl.addEventListener('change', startPoll);
  window.addEventListener('resize', render);

  refresh();
  startPoll();
})();
