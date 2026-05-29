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
const LINK_POLL_MS = 800;
const LINK_POLL_ATTEMPTS = 15;
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

const DEFAULT_UWB_OFFSET_M = -0.2;

function roundOffset(value) {
  return Math.round(Number(value) * 1000) / 1000;
}

function roundCoord(value, decimals = 2) {
  const factor = 10 ** decimals;
  return Math.round(Number(value) * factor) / factor;
}

function formatCoordInput(value, decimals = 2) {
  return roundCoord(value, decimals).toFixed(decimals);
}

function captureOffsetDrafts() {
  const drafts = new Map();
  linkedRowsEl?.querySelectorAll('.offset-input').forEach((input) => {
    const key = input.dataset.key;
    if (!key) {
      return;
    }
    if (document.activeElement === input || input.dataset.dirty === '1') {
      drafts.set(key, input.value);
    }
  });
  return drafts;
}

function applyOffsetDrafts(drafts) {
  if (!drafts?.size) {
    return;
  }
  linkedRowsEl.querySelectorAll('.offset-input').forEach((input) => {
    const key = input.dataset.key;
    if (drafts.has(key)) {
      input.value = drafts.get(key);
      input.dataset.dirty = '1';
    }
  });
}

function bindOffsetInput(input) {
  if (!input || input.dataset.bound === '1') {
    return;
  }
  input.dataset.bound = '1';
  const markDirty = () => {
    input.dataset.dirty = '1';
  };
  input.addEventListener('input', markDirty);
  input.addEventListener('focus', markDirty);
}

function canUpdateLinkedLiveOnly(configured) {
  if (!linkedRowsEl || configured.length === 0) {
    return false;
  }
  if (linkedRowsEl.children.length !== configured.length) {
    return false;
  }
  return configured.every((anchor) =>
    linkedRowsEl.querySelector(`tr[data-key="${anchorKey(anchor)}"]`),
  );
}

function updateLinkedLiveColumns(configured) {
  configured.forEach((anchor) => {
    const key = anchorKey(anchor);
    const row = linkedRowsEl.querySelector(`tr[data-key="${key}"]`);
    if (!row) {
      return;
    }
    const raw =
      anchor.rawRange != null && anchor.rawRange >= 0
        ? formatRange(anchor.rawRange)
        : '—';
    const live =
      anchor.liveRange != null && anchor.liveRange >= 0
        ? formatRange(anchor.liveRange)
        : '—';
    const rawCell = row.querySelector('[data-col="raw"]');
    const liveCell = row.querySelector('[data-col="live"]');
    if (rawCell) {
      rawCell.textContent = raw;
    }
    if (liveCell) {
      liveCell.textContent = live;
    }
  });
}

function anchorKey(anchor) {
  const short = Number(anchor.shortAddress);
  return short > 0 ? `s${short}` : `m${anchor.mac || ''}`;
}

function isUwbAddressStr(s) {
  return /^([0-9A-Fa-f]{2}:){7}[0-9A-Fa-f]{2}$/.test(String(s || '').trim());
}

function buildAnchorPayload(formData) {
  const data = Object.fromEntries(formData);
  let wifiMac = data.wifi_mac?.trim() || '';
  let uwbAddress = data.uwb_address?.trim() || '';
  if (isUwbAddressStr(wifiMac) && !uwbAddress) {
    uwbAddress = wifiMac;
    wifiMac = '';
  }
  const payload = {
    name: data.name,
    x: roundCoord(data.x, 2),
    y: roundCoord(data.y, 2),
    z: roundCoord(data.z, 2),
    offset: roundOffset(data.offset),
  };
  if (wifiMac) {
    payload.wifi_mac = wifiMac;
  }
  if (uwbAddress) {
    payload.uwb_address = uwbAddress;
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
    const raw = Number(dev.rawRange ?? dev.range);
    const corrected = Number(dev.correctedRange ?? dev.range);
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td><code>${formatShort(short)}</code>${linked ? ' <span class="tag-linked">NVS</span>' : ''}</td>
      <td>${formatRange(raw)}</td>
      <td>${formatRange(corrected)}</td>
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

function renderLinked(configured, { liveOnly = false } = {}) {
  if (liveOnly && canUpdateLinkedLiveOnly(configured)) {
    updateLinkedLiveColumns(configured);
    return;
  }

  const drafts = captureOffsetDrafts();

  linkedRowsEl.innerHTML = '';
  linkedEmptyEl.hidden = configured.length > 0;

  configured.forEach((anchor) => {
    const tr = document.createElement('tr');
    const key = anchorKey(anchor);
    tr.dataset.key = key;
    const short = Number(anchor.shortAddress);
    const raw =
      anchor.rawRange != null && anchor.rawRange >= 0
        ? formatRange(anchor.rawRange)
        : '—';
    const live =
      anchor.liveRange != null && anchor.liveRange >= 0
        ? formatRange(anchor.liveRange)
        : '—';
    const offsetVal = roundOffset(anchor.offset ?? 0);
    tr.innerHTML = `
      <td>${anchor.name || '-'}</td>
      <td><code>${anchor.mac || '—'}</code></td>
      <td>${formatShort(short)}</td>
      <td data-col="raw">${raw}</td>
      <td data-col="live">${live}</td>
      <td>
        <input type="number" class="offset-input" step="0.001" value="${offsetVal.toFixed(3)}"
               data-key="${key}" aria-label="Offset ${anchor.name || short}">
      </td>
      <td>${formatCoordInput(anchor.x, 2)}</td>
      <td>${formatCoordInput(anchor.y, 2)}</td>
      <td>${formatCoordInput(anchor.z, 2)}</td>
      <td class="row-actions">
        <button type="button" class="secondary edit-anchor" data-key="${key}">Editar</button>
        <button type="button" class="secondary save-offset" data-key="${key}">Offset</button>
        <button type="button" class="secondary sync-one" data-short="${short}">Sync</button>
        <button type="button" class="danger delete" data-key="${key}">Eliminar</button>
      </td>
    `;
    linkedRowsEl.appendChild(tr);
    bindOffsetInput(tr.querySelector('.offset-input'));
  });

  applyOffsetDrafts(drafts);

  document.querySelectorAll('.edit-anchor').forEach((btn) => {
    btn.addEventListener('click', () => {
      const anchor = configured.find((a) => anchorKey(a) === btn.dataset.key);
      if (anchor) {
        openEditDialog(anchor);
      }
    });
  });

  document.querySelectorAll('.save-offset').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const key = btn.dataset.key;
      const anchor = configured.find((a) => anchorKey(a) === key);
      const input = linkedRowsEl.querySelector(`.offset-input[data-key="${key}"]`);
      if (!anchor || !input) {
        return;
      }
      await saveAnchorOffset(anchor, roundOffset(input.value));
    });
  });

  document.querySelectorAll('.delete').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const anchor = configured.find((a) => anchorKey(a) === btn.dataset.key);
      if (!anchor) {
        return;
      }
      const short = Number(anchor.shortAddress);
      const url =
        anchor.mac && String(anchor.mac).trim()
          ? `/api/anchors?mac=${encodeURIComponent(anchor.mac)}`
          : `/api/anchors?short=${short}`;
      appendDebug(`DELETE ${url}`);
      try {
        await api(url, { method: 'DELETE' });
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
      await runEspNowSync(short > 0 ? short : 0);
    });
  });
}

function openEditDialog(anchor) {
  const dialog = document.getElementById('edit-anchor-dialog');
  if (!dialog) {
    return;
  }
  const short = Number(anchor.shortAddress);
  document.getElementById('edit-mac').value = anchor.mac || '';
  document.getElementById('edit-lookup-short').value = short > 0 ? String(short) : '';
  document.getElementById('edit-name').value = anchor.name || '';
  document.getElementById('edit-short').value = short > 0 ? formatShort(short) : '';
  document.getElementById('edit-uwb').value = anchor.uwb_address || '';
  document.getElementById('edit-x').value = formatCoordInput(anchor.x ?? 0, 2);
  document.getElementById('edit-y').value = formatCoordInput(anchor.y ?? 0, 2);
  document.getElementById('edit-z').value = formatCoordInput(anchor.z ?? 0, 2);
  document.getElementById('edit-offset').value = roundOffset(anchor.offset ?? 0).toFixed(3);
  dialog.showModal();
}

async function saveAnchorEdit(formData) {
  const mac = String(formData.get('mac') || '').trim();
  const lookupShort = parseShortInput(formData.get('lookupShortAddress'));
  const payload = {
    name: formData.get('name'),
    x: roundCoord(formData.get('x'), 2),
    y: roundCoord(formData.get('y'), 2),
    z: roundCoord(formData.get('z'), 2),
    offset: roundOffset(formData.get('offset')),
  };
  if (mac) {
    payload.mac = mac;
  }
  if (lookupShort > 0) {
    payload.lookupShortAddress = lookupShort;
  }
  const uwb = formData.get('uwb_address')?.trim();
  if (uwb) {
    payload.uwb_address = uwb;
  }
  const short = parseShortInput(formData.get('shortAddress'));
  if (short > 0) {
    payload.shortAddress = short;
  }
  if (!payload.mac && !payload.lookupShortAddress) {
    throw new Error('No se puede identificar el anchor (falta MAC o short).');
  }
  appendDebug('PUT /api/anchors', payload);
  const res = await apiRaw('/api/anchors', {
    method: 'PUT',
    body: JSON.stringify(payload),
  });
  if (!res.ok) {
    throw new Error(formatApiError(res.body, `HTTP ${res.status}`));
  }
  showFeedback(`Anchor ${payload.name} actualizado.`);
  await refresh({ silent: true });
}

async function runEspNowSync(shortAddress) {
  appendDebug(
    shortAddress > 0
      ? `POST /api/sync short=${formatShort(shortAddress)}`
      : 'POST /api/sync (todos)',
  );
  try {
    const body =
      shortAddress > 0 ? JSON.stringify({ shortAddress }) : '{}';
    const res = await apiRaw('/api/sync', { method: 'POST', body });
    appendDebug(`Sync → HTTP ${res.status}`, res.body);
    if (!res.ok) {
      throw new Error(
        formatApiError(
          res.body,
          res.status === 503
            ? 'WiFi o ESP-NOW no disponible en el tag'
            : `HTTP ${res.status}`,
        ),
      );
    }

    let linked = false;
    for (let i = 0; i < LINK_POLL_ATTEMPTS; i++) {
      await new Promise((r) => setTimeout(r, LINK_POLL_MS));
      const anchorsRaw = await api('/api/anchors');
      const { configured, uwb } = parseAnchorsResponse(anchorsRaw);
      const match = configured.find(
        (a) => Number(a.shortAddress) === shortAddress,
      );
      if (shortAddress === 0) {
        linked = configured.length > 0;
      } else if (match && match.mac && Number(match.shortAddress) === shortAddress) {
        linked = true;
        break;
      }
      renderDiscovered(uwb, configured);
      renderLinked(configured, { liveOnly: true });
    }

    if (shortAddress > 0) {
      showFeedback(
        linked
          ? `${formatShort(shortAddress)} vinculado correctamente.`
          : `${formatShort(shortAddress)}: sin respuesta ESP-NOW. Comprueba WiFi del tag y del anchor, mismo AP, y reflashea ambos.`,
        !linked,
      );
    } else {
      showFeedback(linked ? 'Sync global completado.' : 'Sync enviado; revisa NVS.');
    }
    await refresh({ silent: true });
  } catch (err) {
    showFeedback(err.message, true);
  }
}

async function saveAnchorOffset(anchor, offset) {
  const body = { offset: roundOffset(offset) };
  if (anchor.mac && String(anchor.mac).trim()) {
    body.mac = anchor.mac;
  } else if (Number(anchor.shortAddress) > 0) {
    body.shortAddress = Number(anchor.shortAddress);
  } else {
    showFeedback('No se puede guardar offset: falta MAC o short.', true);
    return;
  }
  appendDebug('POST /api/anchors/offset', body);
  try {
    const res = await apiRaw('/api/anchors/offset', {
      method: 'POST',
      body: JSON.stringify(body),
    });
    if (!res.ok) {
      throw new Error(formatApiError(res.body, `HTTP ${res.status}`));
    }
    showFeedback(`Offset guardado: ${roundOffset(offset).toFixed(3)} m`);
    linkedRowsEl
      .querySelector(`.offset-input[data-key="${anchorKey(anchor)}"]`)
      ?.removeAttribute('data-dirty');
    await refresh({ silent: true });
  } catch (err) {
    showFeedback(err.message, true);
  }
}

async function linkAnchor(shortAddress, buttonEl) {
  if (buttonEl) {
    buttonEl.disabled = true;
    buttonEl.textContent = '…';
  }
  try {
    await runEspNowSync(shortAddress);
  } finally {
    if (buttonEl) {
      buttonEl.disabled = false;
      buttonEl.textContent = 'Vincular';
    }
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
      ` · NVS: ${configured.length}` +
      (status.trilateration_mode ? ` · Tri: ${status.trilateration_mode}` : '');

    const map3dLink = document.getElementById('portal-map-3d');
    if (map3dLink && status.trilateration_mode !== '3d') {
      map3dLink.hidden = true;
    }

    renderDiscovered(uwb, configured);
    renderLinked(configured, { liveOnly: silent });

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
  if (!payload.shortAddress && !payload.wifi_mac) {
    showFeedback('Indica short UWB o MAC WiFi.', true);
    return;
  }
  appendDebug(
    andSync ? 'POST /api/anchors + sync' : 'POST /api/anchors',
    payload,
  );
  pushToast('Guardando en NVS…', 'info');
  try {
    const res = await apiRaw('/api/anchors', {
      method: 'POST',
      body: JSON.stringify(payload),
    });
    appendDebug(`Guardar → HTTP ${res.status}`, res.body);
    if (!res.ok) {
      throw new Error(formatApiError(res.body, `HTTP ${res.status}`));
    }
    const label =
      res.body.name ||
      res.body.mac ||
      (payload.shortAddress ? formatShort(payload.shortAddress) : payload.name);
    showFeedback(`Guardado en NVS: ${label}`);
    addForm.reset();
    await refresh({ silent: true });
    if (andSync) {
      await runEspNowSync(payload.shortAddress || 0);
    }
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
  try {
    await runEspNowSync(0);
  } finally {
    btn.disabled = false;
  }
});

document.getElementById('edit-anchor-form')?.addEventListener('submit', async (event) => {
  event.preventDefault();
  const form = event.target;
  try {
    await saveAnchorEdit(new FormData(form));
    document.getElementById('edit-anchor-dialog')?.close();
  } catch (err) {
    showFeedback(err.message, true);
  }
});

document.getElementById('edit-cancel')?.addEventListener('click', () => {
  document.getElementById('edit-anchor-dialog')?.close();
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
