const DEFAULT_UWB_OFFSET_M = -0.2;

const statusEl = document.getElementById('status');
const feedbackEl = document.getElementById('feedback');
const uwbFeedbackEl = document.getElementById('uwb-feedback');
const form = document.getElementById('position-form');
const uwbForm = document.getElementById('uwb-form');
const uwbBytesEl = document.getElementById('uwb-bytes');
const uwbPreviewEl = document.getElementById('uwb-address-preview');
const apiOutputEl = document.getElementById('api-output');

const HEX_RE = /^[0-9A-Fa-f]{0,2}$/;

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
    });
  });
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

function initApiExplorer() {
  document.querySelectorAll('.try-api').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const path = btn.dataset.path;
      const method = btn.dataset.method || 'GET';
      let body;

      if (btn.dataset.bodyId) {
        const raw = document.getElementById(btn.dataset.bodyId)?.value?.trim();
        try {
          body = raw;
          JSON.parse(raw);
        } catch {
          showApiOutput('Error', 'JSON inválido en el cuerpo', true);
          return;
        }
      } else if (btn.dataset.body) {
        body = btn.dataset.body;
      }

      const options = { method };
      if (body !== undefined && method !== 'GET') {
        options.body = body;
      }

      try {
        const result = await apiRaw(path, options);
        showApiOutput(
          `${method} ${path} → HTTP ${result.status}`,
          result.body,
          !result.ok,
        );
        if (result.ok) {
          await refresh();
        }
      } catch (err) {
        showApiOutput('Error de red', err.message, true);
      }
    });
  });
}

async function api(path, options = {}) {
  const res = await fetch(path, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(text || res.statusText);
  }
  const type = res.headers.get('content-type') || '';
  return type.includes('json') ? res.json() : res.text();
}

function buildUwbInputs() {
  uwbBytesEl.innerHTML = '';
  for (let i = 0; i < 8; i += 1) {
    if (i > 0) {
      const sep = document.createElement('span');
      sep.className = 'uwb-sep';
      sep.textContent = ':';
      uwbBytesEl.appendChild(sep);
    }
    const input = document.createElement('input');
    input.type = 'text';
    input.className = 'uwb-byte';
    input.maxLength = 2;
    input.inputMode = 'text';
    input.autocomplete = 'off';
    input.spellcheck = false;
    input.dataset.index = String(i);
    input.setAttribute('aria-label', `Byte ${i + 1}`);
    input.addEventListener('input', onUwbByteInput);
    input.addEventListener('keydown', onUwbByteKeydown);
    input.addEventListener('paste', onUwbPaste);
    uwbBytesEl.appendChild(input);
  }
}

function getUwbInputs() {
  return [...uwbBytesEl.querySelectorAll('.uwb-byte')];
}

function sanitizeHex(value) {
  return value.replace(/[^0-9A-Fa-f]/g, '').toUpperCase().slice(0, 2);
}

function onUwbByteInput(event) {
  const input = event.target;
  const clean = sanitizeHex(input.value);
  input.value = clean;
  updateUwbPreview();

  if (clean.length === 2) {
    const index = Number(input.dataset.index);
    const inputs = getUwbInputs();
    if (index < inputs.length - 1) {
      inputs[index + 1].focus();
      inputs[index + 1].select();
    }
  }
}

function onUwbByteKeydown(event) {
  const input = event.target;
  const index = Number(input.dataset.index);
  const inputs = getUwbInputs();

  if (event.key === 'Backspace' && input.value === '' && index > 0) {
    inputs[index - 1].focus();
    return;
  }
  if (event.key === 'ArrowLeft' && index > 0) {
    event.preventDefault();
    inputs[index - 1].focus();
    return;
  }
  if (event.key === 'ArrowRight' && index < inputs.length - 1) {
    event.preventDefault();
    inputs[index + 1].focus();
  }
}

function onUwbPaste(event) {
  event.preventDefault();
  const text = (event.clipboardData || window.clipboardData).getData('text');
  const parts = text.replace(/[^0-9A-Fa-f:]/g, '').split(':').filter(Boolean);
  const inputs = getUwbInputs();

  if (parts.length >= 8) {
    parts.slice(0, 8).forEach((part, i) => {
      inputs[i].value = sanitizeHex(part).padStart(Math.min(part.length, 2), '0').slice(-2);
    });
  } else {
    const flat = sanitizeHex(text);
    for (let i = 0; i < 8 && i * 2 < flat.length; i += 1) {
      inputs[i].value = flat.slice(i * 2, i * 2 + 2);
    }
  }
  updateUwbPreview();
}

function readUwbBytes() {
  const inputs = getUwbInputs();
  const bytes = inputs.map((input) => sanitizeHex(input.value));
  const valid = bytes.every((byte) => HEX_RE.test(byte) && byte.length === 2);
  return { bytes, valid };
}

function updateUwbPreview() {
  const { bytes, valid } = readUwbBytes();
  uwbPreviewEl.textContent = valid ? bytes.join(':') : 'Completa los 8 bytes (00–FF)';
}

function fillUwbForm(data) {
  const inputs = getUwbInputs();
  const source = Array.isArray(data.bytes) ? data.bytes : [];
  inputs.forEach((input, i) => {
    input.value = sanitizeHex(String(source[i] ?? ''));
  });
  if (data.uwb_address) {
    uwbPreviewEl.textContent = data.uwb_address;
  } else {
    updateUwbPreview();
  }
}

function fillForm(data) {
  form.x.value = data.x ?? 0;
  form.y.value = data.y ?? 0;
  form.z.value = data.z ?? 0;
  form.offset.value = data.offset ?? DEFAULT_UWB_OFFSET_M;
  statusEl.textContent = `${data.name} · MAC ${data.mac} · IP ${data.ip}`;
  fillUwbForm(data);
}

async function refresh() {
  const data = await api('/api/position');
  fillForm(data);
}

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const payload = Object.fromEntries(new FormData(form));
  Object.keys(payload).forEach((key) => {
    payload[key] = Number(payload[key]);
  });
  await api('/api/position', {
    method: 'POST',
    body: JSON.stringify(payload),
  });
  feedbackEl.textContent = 'Posición guardada.';
  await refresh();
});

uwbForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  const { bytes, valid } = readUwbBytes();
  if (!valid) {
    uwbFeedbackEl.textContent = 'Introduce 8 bytes hex válidos (00–FF).';
    return;
  }

  uwbFeedbackEl.textContent = 'Guardando… el anchor se reiniciará.';
  try {
    await api('/api/uwb-address', {
      method: 'POST',
      body: JSON.stringify({ bytes }),
    });
    uwbFeedbackEl.textContent = 'Guardado. Reiniciando anchor…';
  } catch (err) {
    uwbFeedbackEl.textContent = `Error: ${err.message}`;
  }
});

document.getElementById('sync').addEventListener('click', async () => {
  await api('/api/sync', { method: 'POST', body: '{}' });
  feedbackEl.textContent = 'Posición enviada al tag por ESP-NOW.';
});

buildUwbInputs();
initNav();
initApiExplorer();

refresh().catch((err) => {
  statusEl.textContent = `Error: ${err.message}`;
});
