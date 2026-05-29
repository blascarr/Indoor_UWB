import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const statusEl = document.getElementById('status');
const root = document.getElementById('three-root');
const autoRefreshEl = document.getElementById('auto-refresh');
const btnReset = document.getElementById('btn-reset');
const banner = document.getElementById('banner');

let scene;
let camera;
let renderer;
let controls;
let anchorGroup;
let tagMesh;
let tagTrail;
let pollTimer = null;
let lastData = null;
let trailPoints = [];

function initThree() {
  const rect = root.getBoundingClientRect();
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x0b1220);

  camera = new THREE.PerspectiveCamera(55, rect.width / rect.height, 0.1, 200);
  camera.position.set(6, 8, 10);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(rect.width, rect.height);
  renderer.setPixelRatio(window.devicePixelRatio || 1);
  root.appendChild(renderer.domElement);

  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.target.set(0, 0, 0);

  const ambient = new THREE.AmbientLight(0xffffff, 0.65);
  scene.add(ambient);
  const dir = new THREE.DirectionalLight(0xffffff, 0.85);
  dir.position.set(5, 10, 7);
  scene.add(dir);

  const grid = new THREE.GridHelper(20, 20, 0x334155, 0x1e293b);
  scene.add(grid);

  const axes = new THREE.AxesHelper(1.5);
  scene.add(axes);

  anchorGroup = new THREE.Group();
  scene.add(anchorGroup);

  tagMesh = new THREE.Mesh(
    new THREE.SphereGeometry(0.15, 16, 16),
    new THREE.MeshStandardMaterial({ color: 0x22c55e, emissive: 0x14532d }),
  );
  tagMesh.visible = false;
  scene.add(tagMesh);

  tagTrail = new THREE.Line(
    new THREE.BufferGeometry(),
    new THREE.LineBasicMaterial({ color: 0x22c55e, transparent: true, opacity: 0.45 }),
  );
  scene.add(tagTrail);

  function animate() {
    requestAnimationFrame(animate);
    controls.update();
    renderer.render(scene, camera);
  }
  animate();

  window.addEventListener('resize', onResize);
}

function onResize() {
  const rect = root.getBoundingClientRect();
  camera.aspect = rect.width / rect.height;
  camera.updateProjectionMatrix();
  renderer.setSize(rect.width, rect.height);
}

function clearAnchors() {
  while (anchorGroup.children.length) {
    const obj = anchorGroup.children[0];
    anchorGroup.remove(obj);
    obj.geometry?.dispose();
    if (obj.material) {
      if (Array.isArray(obj.material)) {
        obj.material.forEach((m) => m.dispose());
      } else {
        obj.material.dispose();
      }
    }
  }
}

function addLabel(text, position) {
  const canvas = document.createElement('canvas');
  const ctx = canvas.getContext('2d');
  canvas.width = 256;
  canvas.height = 64;
  ctx.fillStyle = '#e2e8f0';
  ctx.font = '28px system-ui,sans-serif';
  ctx.fillText(text, 8, 40);
  const tex = new THREE.CanvasTexture(canvas);
  const mat = new THREE.SpriteMaterial({ map: tex, transparent: true });
  const sprite = new THREE.Sprite(mat);
  sprite.position.copy(position);
  sprite.position.y += 0.35;
  sprite.scale.set(1.2, 0.3, 1);
  anchorGroup.add(sprite);
}

function updateScene(data) {
  clearAnchors();

  (data.anchors || []).forEach((anchor) => {
    const color = anchor.active ? 0x3b82f6 : 0x475569;
    const mesh = new THREE.Mesh(
      new THREE.SphereGeometry(anchor.isOrigin ? 0.18 : 0.12, 12, 12),
      new THREE.MeshStandardMaterial({ color }),
    );
    mesh.position.set(anchor.x, anchor.z, -anchor.y);
    anchorGroup.add(mesh);

    if (anchor.liveRange > 0) {
      const wire = new THREE.Mesh(
        new THREE.SphereGeometry(anchor.liveRange, 16, 12),
        new THREE.MeshBasicMaterial({
          color: 0x3b82f6,
          wireframe: true,
          transparent: true,
          opacity: 0.12,
        }),
      );
      wire.position.copy(mesh.position);
      anchorGroup.add(wire);
    }

    addLabel(anchor.name || 'Anchor', mesh.position);
  });

  if (data.position?.valid) {
    tagMesh.visible = true;
    tagMesh.position.set(data.position.x, data.position.z, -data.position.y);
    trailPoints.push(tagMesh.position.clone());
    if (trailPoints.length > window.MAP_TRAIL_MAX) {
      trailPoints.shift();
    }
    const positions = new Float32Array(trailPoints.length * 3);
    trailPoints.forEach((p, i) => {
      positions[i * 3] = p.x;
      positions[i * 3 + 1] = p.y;
      positions[i * 3 + 2] = p.z;
    });
    tagTrail.geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    tagTrail.geometry.setDrawRange(0, trailPoints.length);
  } else {
    tagMesh.visible = false;
  }

  const bounds = window.computeViewBounds(data);
  const cx = (bounds.minX + bounds.maxX) / 2;
  const cy = (bounds.minY + bounds.maxY) / 2;
  const span = Math.max(bounds.maxX - bounds.minX, bounds.maxY - bounds.minY, 2);
  controls.target.set(cx, 0, -cy);
  camera.position.set(cx + span, span * 0.8, -cy + span);
}

async function refresh() {
  try {
    lastData = await window.fetchMapData();
    if (lastData.trilateration_mode !== '3d') {
      banner.textContent =
        'Este firmware usa trilateración 2D. La vista 3D es orientativa (planta + Z).';
      banner.classList.remove('hidden');
    } else {
      banner.classList.add('hidden');
    }
    statusEl.textContent = window.formatPositionStatus(lastData);
    updateScene(lastData);
  } catch (err) {
    statusEl.textContent = `Error: ${err.message}`;
    banner.textContent =
      'No se pudo cargar /api/map o Three.js (CDN). Prueba /map/ sin internet.';
    banner.classList.remove('hidden');
  }
}

function startPoll() {
  if (pollTimer) {
    clearInterval(pollTimer);
  }
  if (autoRefreshEl.checked) {
    pollTimer = setInterval(refresh, window.MAP_POLL_MS);
  }
}

btnReset.addEventListener('click', () => {
  trailPoints = [];
  if (lastData) {
    updateScene(lastData);
  }
  controls.reset();
});

autoRefreshEl.addEventListener('change', startPoll);

try {
  initThree();
  refresh();
  startPoll();
} catch (err) {
  statusEl.textContent = `Three.js: ${err.message}`;
  banner.textContent = 'Error cargando Three.js desde CDN. Usa /map/ local.';
  banner.classList.remove('hidden');
}
