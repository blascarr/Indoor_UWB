const statusEl = document.getElementById('status');
const discoveredRowsEl = document.getElementById('discovered-rows');
const linkedRowsEl = document.getElementById('linked-rows');
const discoveredEmptyEl = document.getElementById('discovered-empty');
const linkedEmptyEl = document.getElementById('linked-empty');
const toastContainerEl = document.getElementById('toast-container');
const debugLogEl = document.getElementById('debug-log');
const addForm = document.getElementById('add-form');
const apiOutputEl = document.getElementById('api-output');
const autoRefreshEl = document.getElementById('auto-refresh');

const REFRESH_MS = 3000;
const LINK_POLL_MS = 600;
const LINK_POLL_ATTEMPTS = 8;
const MAX_DEBUG_LINES = 80;

let refreshTimer = null;

function formatShort(shortAddress) {
  const n = Number(shortAddress);
  if (!n) {
    return '—';
  }
  return `0x${n.toString(16).toUpperCase().padStart(4, '0')}`;
}

function formatRange(range) {
  const r = Number(range);
  if (Number.isNaN(r) || r < 0) {
    return '—';
  }
  return `${r.toFixed(2)} m`;
}

function nowTime() {
  return new Date().toLocaleTimeString('es-ES', { hour12: false });
}

let setDebugDrawerOpen = () => {};

function appendDebug(label, data) {
  if (!debugLogEl) {
    return;
  }
  const body =
    data === undefined
      ? ''
      : typeof data === 'string'
        ? data
        : JSON.stringify(data, null, 2);
  const block = `[${nowTime()}] ${label}${body ? `\n${body}` : ''}\n\n`;
  const lines = (debugLogEl.textContent + block).split('\n');
  debugLogEl.textContent = lines.slice(-MAX_DEBUG_LINES * 2).join('\n');
  debugLogEl.scrollTop = debugLogEl.scrollHeight;
}

function initDebugDrawer() {
  const drawer = document.getElementById('debug-drawer');
  const toggle = document.getElementById('debug-drawer-toggle');
  const navBtn = document.getElementById('debug-drawer-nav');
  if (!drawer) {
    return;
  }

  setDebugDrawerOpen = (open) => {
    drawer.classList.toggle('is-open', open);
    document.body.classList.toggle('drawer-open', open);
    toggle?.setAttribute('aria-expanded', String(open));
    try {
      localStorage.setItem('debugDrawerOpen', open ? '1' : '0');
    } catch {
      /* ignore */
    }
  };

  const stored = localStorage.getItem('debugDrawerOpen');
  setDebugDrawerOpen(stored === '1');

  const onToggle = () =>
    setDebugDrawerOpen(!drawer.classList.contains('is-open'));
  toggle?.addEventListener('click', onToggle);
  navBtn?.addEventListener('click', onToggle);
}

const MAX_TOASTS = 6;

function pushToast(message, type = 'info') {
  if (!toastContainerEl || !message) {
    return;
  }
  while (toastContainerEl.children.length >= MAX_TOASTS) {
    toastContainerEl.firstChild?.remove();
  }
  const el = document.createElement('div');
  el.className = `toast ${type}`;
  el.textContent = message;
  el.title = 'Clic para cerrar';
  el.addEventListener('click', () => el.remove());
  toastContainerEl.appendChild(el);
}

function formatApiError(body, fallback) {
  if (typeof body === 'object' && body !== null) {
    const parts = [];
    if (body.error) {
      parts.push(body.error);
    }
    if (body.hint) {
      parts.push(body.hint);
    }
    if (parts.length) {
      return parts.join(' — ');
    }
  }
  if (typeof body === 'string' && body.length < 200) {
    return body;
  }
  return fallback;
}

function showFeedback(message, isError = false) {
  const type = isError ? 'error' : 'success';
  pushToast(message, type);
  appendDebug(isError ? `ERROR: ${message}` : message);
  if (isError) {
    setDebugDrawerOpen(true);
  }
}

function initNav() {
  document.querySelectorAll('.nav-btn').forEach((btn) => {
    btn.addEventListener('click', () => {
      const panelId = btn.dataset.panel;
      document.querySelectorAll('.nav-btn').forEach((b) => b.classList.remove('active'));
      btn.classList.add('active');
      document.querySelectorAll('.panel').forEach((panel) => {
        const show = panel.id === `panel-${panelId}`;
        panel.hidden = !show;
        panel.classList.toggle('active', show);
      });
      setupAutoRefresh();
    });
  });
}

function setupAutoRefresh() {
  if (refreshTimer) {
    clearInterval(refreshTimer);
    refreshTimer = null;
  }
  const onDiscovery =
    !document.getElementById('panel-discovery')?.hidden &&
    autoRefreshEl?.checked;
  if (onDiscovery) {
    refreshTimer = setInterval(() => {
      refresh({ silent: true }).catch(() => {});
    }, REFRESH_MS);
  }
}

function showApiOutput(label, data, isError = false) {
  if (!apiOutputEl) {
    return;
  }
  const body =
    typeof data === 'string' ? data : JSON.stringify(data, null, 2);
  apiOutputEl.textContent = `${label}\n${body}`;
  apiOutputEl.style.color = isError ? '#fca5a5' : '#e2e8f0';
}

async function apiRaw(path, options = {}) {
  const res = await fetch(path, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  const type = res.headers.get('content-type') || '';
  const body = type.includes('json') ? await res.json() : await res.text();
  return { ok: res.ok, status: res.status, body };
}

async function api(path, options = {}) {
  const result = await apiRaw(path, options);
  if (!result.ok) {
    throw new Error(
      formatApiError(result.body, `HTTP ${result.status}`),
    );
  }
  return result.body;
}

function parseAnchorsResponse(data) {
  if (Array.isArray(data)) {
    return { configured: data, uwb: [], uwb_count: data.length };
  }
  return {
    configured: Array.isArray(data.configured) ? data.configured : [],
    uwb: Array.isArray(data.uwb) ? data.uwb : [],
    uwb_count: Number(data.uwb_count) || 0,
  };
}

function parseShortInput(raw) {
  if (!raw || !String(raw).trim()) {
    return 0;
  }
  const s = String(raw).trim();
  return /^0x/i.test(s) ? parseInt(s, 16) : parseInt(s, 10);
}

function buildAnchorPayload(formData) {
  const data = Object.fromEntries(formData);
  const payload = {
    name: data.name,
    wifi_mac: data.wifi_mac?.trim(),
    x: Number(data.x),
    y: Number(data.y),
    z: Number(data.z),
    offset: Number(data.offset),
  };
  if (data.uwb_address?.trim()) {
    payload.uwb_address = data.uwb_address.trim();
  }
  const short = parseShortInput(data.shortAddress);
  if (short > 0 && !Number.isNaN(short)) {
    payload.shortAddress = short;
  }
  return payload;
}

function renderDiscovered(uwbDevices, configured) {
  discoveredRowsEl.innerHTML = '';
  const hasUwb = uwbDevices.length > 0;
  discoveredEmptyEl.hidden = hasUwb;

  uwbDevices.forEach((dev) => {
    const short = Number(dev.shortAddress);
    const linked = configured.some(
      (a) => Number(a.shortAddress) === short && short > 0,
    );
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td><code>${formatShort(short)}</code>${linked ? ' <span class="tag-linked">NVS</span>' : ''}</td>
      <td>${formatRange(dev.range)}</td>
      <td>${dev.rxPower != null ? Number(dev.rxPower).toFixed(1) : '—'}</td>
      <td><span class="badge ${dev.active ? 'on' : 'off'}">${dev.active ? 'activo' : 'inactivo'}</span></td>
      <td><button type="button" class="link-btn" data-short="${short}">Vincular</button></td>
    `;
    discoveredRowsEl.appendChild(tr);
  });

  document.querySelectorAll('.link-btn').forEach((btn) => {
    btn.addEventListener('click', () => linkAnchor(Number(btn.dataset.short), btn));
  });
}

function renderLinked(configured) {
  linkedRowsEl.innerHTML = '';
  linkedEmptyEl.hidden = configured.length > 0;

  configured.forEach((anchor) => {
    const tr = document.createElement('tr');
    const short = Number(anchor.shortAddress);
    const live =
      anchor.liveRange != null && anchor.liveRange >= 0
        ? formatRange(anchor.liveRange)
        : '—';
    tr.innerHTML = `
      <td>${anchor.name || '-'}</td>
      <td><code>${anchor.mac || '—'}</code></td>
      <td>${formatShort(short)}</td>
      <td>${live}</td>
      <td>${Number(anchor.x).toFixed(2)}</td>
      <td>${Number(anchor.y).toFixed(2)}</td>
      <td>${Number(anchor.z).toFixed(2)}</td>
      <td class="row-actions">
        <button type="button" class="secondary sync-one" data-short="${short}">Sync</button>
        <button data-mac="${anchor.mac}" class="danger delete">Eliminar</button>
      </td>
    `;
    linkedRowsEl.appendChild(tr);
  });

  document.querySelectorAll('.delete').forEach((btn) => {
    btn.addEventListener('click', async () => {
      appendDebug(`DELETE /api/anchors?mac=${btn.dataset.mac}`);
      try {
        await api(`/api/anchors?mac=${encodeURIComponent(btn.dataset.mac)}`, {
          method: 'DELETE',
        });
        showFeedback('Anchor eliminado.');
        await refresh();
      } catch (err) {
        showFeedback(err.message, true);
      }
    });
  });

  document.querySelectorAll('.sync-one').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const short = Number(btn.dataset.short);
      appendDebug(
        short > 0
          ? `POST /api/sync short=${formatShort(short)}`
          : 'POST /api/sync (todos)',
      );
      try {
        const body =
          short > 0 ? JSON.stringify({ shortAddress: short }) : '{}';
        const res = await apiRaw('/api/sync', { method: 'POST', body });
        appendDebug(`Sync → HTTP ${res.status}`, res.body);
        showFeedback('Sync ESP-NOW enviado.');
        setTimeout(() => refresh(), 800);
      } catch (err) {
        showFeedback(err.message, true);
      }
    });
  });
}

async function linkAnchor(shortAddress, buttonEl) {
  if (buttonEl) {
    buttonEl.disabled = true;
    buttonEl.textContent = '…';
  }
  appendDebug(`Vincular ${formatShort(shortAddress)}`, {
    action: 'POST /api/sync',
    shortAddress,
  });
  try {
    await api('/api/sync', {
      method: 'POST',
      body: JSON.stringify({ shortAddress }),
    });

    let linked = false;
    for (let i = 0; i < LINK_POLL_ATTEMPTS; i++) {
      await new Promise((r) => setTimeout(r, LINK_POLL_MS));
      const anchorsRaw = await api('/api/anchors');
      const { configured, uwb } = parseAnchorsResponse(anchorsRaw);
      if (
        configured.some((a) => Number(a.shortAddress) === shortAddress) &&
        configured.find((a) => Number(a.shortAddress) === shortAddress)?.mac
      ) {
        linked = true;
        break;
      }
      renderDiscovered(uwb, configured);
      renderLinked(configured);
    }

    showFeedback(
      linked
        ? `${formatShort(shortAddress)} vinculado.`
        : `${formatShort(shortAddress)}: sin respuesta ESP-NOW (revisa WiFi del anchor).`,
      !linked,
    );
  } catch (err) {
    showFeedback(err.message, true);
  } finally {
    if (buttonEl) {
      buttonEl.disabled = false;
      buttonEl.textContent = 'Vincular';
    }
    await refresh();
  }
}

async function refresh(options = {}) {
  const { silent = false } = options;
  const refreshBtn = document.getElementById('refresh');
  if (refreshBtn && !silent) {
    refreshBtn.disabled = true;
    refreshBtn.textContent = 'Actualizando…';
  }

  if (!silent) {
    setDebugDrawerOpen(true);
    appendDebug('--- Actualizar: GET /api/debug, /api/status, /api/anchors ---');
  }

  try {
    const [debugRes, status, anchorsRaw] = await Promise.all([
      apiRaw('/api/debug'),
      api('/api/status'),
      api('/api/anchors'),
    ]);

    if (!silent) {
      appendDebug(
        `GET /api/debug → HTTP ${debugRes.status}`,
        debugRes.body,
      );
    } else if (debugRes.ok) {
      appendDebug(
        `[auto] uwb=${debugRes.body?.uwb_network_devices ?? '?'} nvs=${debugRes.body?.anchors_nvs ?? '?'}`,
      );
    }

    const { configured, uwb, uwb_count } = parseAnchorsResponse(anchorsRaw);

    if (!silent) {
      appendDebug('GET /api/status', status);
      appendDebug(`GET /api/anchors (uwb_count=${uwb_count})`, anchorsRaw);
    }

    statusEl.textContent =
      `MAC ${status.mac} · IP ${status.ip}` +
      ` · WiFi ${status.wifi_connected ? 'OK' : 'NO'}` +
      ` · UWB en red: ${status.uwb_count ?? uwb_count}` +
      ` · NVS: ${configured.length}`;

    renderDiscovered(uwb, configured);
    renderLinked(configured);

    if (!silent && uwb.length === 0) {
      showFeedback(
        debugRes.body?.hint ||
          '0 dispositivos UWB en ranging. Comprueba anchor encendido y protocolo.',
        true,
      );
    }
  } catch (err) {
    appendDebug('Actualizar FALLÓ', err.message);
    if (!silent) {
      showFeedback(`Error al actualizar: ${err.message}`, true);
    }
    statusEl.textContent = `Error: ${err.message}`;
  } finally {
    if (refreshBtn && !silent) {
      refreshBtn.disabled = false;
      refreshBtn.textContent = 'Actualizar';
    }
  }
}

async function submitManualAnchor(andSync) {
  const payload = buildAnchorPayload(new FormData(addForm));
  const bodyJson = JSON.stringify(payload);
  appendDebug(
    andSync ? 'POST /api/anchors + sync' : 'POST /api/anchors',
    payload,
  );
  pushToast('Enviando registro al tag…', 'info');
  setDebugDrawerOpen(true);
  try {
    const res = await apiRaw('/api/anchors', {
      method: 'POST',
      body: bodyJson,
    });
    appendDebug(`Guardar → HTTP ${res.status}`, res.body);
    if (!res.ok) {
      throw new Error(formatApiError(res.body, `HTTP ${res.status}`));
    }
    showFeedback(`Guardado en NVS: ${res.body.mac || payload.wifi_mac}`);
    if (andSync) {
      const short = payload.shortAddress || 0;
      const syncBody =
        short > 0 ? JSON.stringify({ shortAddress: short }) : '{}';
      const syncRes = await apiRaw('/api/sync', {
        method: 'POST',
        body: syncBody,
      });
      appendDebug(`Sync → HTTP ${syncRes.status}`, syncRes.body);
      showFeedback('Guardado y sync ESP-NOW enviados.');
    }
    addForm.reset();
    await refresh();
  } catch (err) {
    showFeedback(err.message, true);
  }
}

if (addForm) {
  addForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    await submitManualAnchor(false);
  });
}

document.getElementById('save-and-sync')?.addEventListener('click', async () => {
  if (!addForm.reportValidity()) {
    return;
  }
  await submitManualAnchor(true);
});

document.getElementById('sync-all')?.addEventListener('click', async () => {
  const btn = document.getElementById('sync-all');
  btn.disabled = true;
  appendDebug('POST /api/sync (todos)');
  try {
    const res = await apiRaw('/api/sync', { method: 'POST', body: '{}' });
    appendDebug(`Vincular todos → HTTP ${res.status}`, res.body);
    showFeedback('Sync global enviado.');
    await new Promise((r) => setTimeout(r, 1200));
    await refresh();
  } catch (err) {
    showFeedback(err.message, true);
  } finally {
    btn.disabled = false;
  }
});

document.getElementById('refresh')?.addEventListener('click', () => refresh());
document.getElementById('clear-debug')?.addEventListener('click', () => {
  debugLogEl.textContent = '';
  appendDebug('Log limpiado');
});
autoRefreshEl?.addEventListener('change', setupAutoRefresh);

function initApiExplorer() {
  document.querySelectorAll('.try-api').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const path = btn.dataset.path;
      const method = btn.dataset.method || 'GET';
      let body = btn.dataset.body;
      if (btn.dataset.bodyId) {
        body = document.getElementById(btn.dataset.bodyId)?.value?.trim();
      }
      const options = { method };
      if (body && method !== 'GET') {
        options.body = body;
      }
      try {
        const result = await apiRaw(path, options);
        showApiOutput(`${method} ${path} → ${result.status}`, result.body, !result.ok);
        appendDebug(`API ${method} ${path}`, result.body);
        if (result.ok) {
          await refresh({ silent: true });
        }
      } catch (err) {
        showApiOutput('Error', err.message, true);
      }
    });
  });
}

initDebugDrawer();
initNav();
initApiExplorer();
setupAutoRefresh();

refresh().catch((err) => {
  statusEl.textContent = `Error: ${err.message}`;
  appendDebug('Inicio fallido', err.message);
});
