// Metro Route Finder Frontend JavaScript
// Communicates with C++ Winsock2 Backend at http://127.0.0.1:8080

const API_BASE = ''; // Same-origin when served by C++ server, or direct API routes

// App State
let allStations = [];
let stationMap = new Map();
let networkEdges = [];
let selectedStationName = null;

// Zoom and Pan State
let zoomLevel = 1.0;
let panX = 0;
let panY = 0;
let isPanning = false;
let startPanX = 0;
let startPanY = 0;

// Color mappings for Delhi Metro lines (Strict Requirements)
const LINE_COLORS = {
  'Blue Line': '#0072CE',
  'Yellow Line': '#FFC600',
  'Violet Line': '#8B2671',
  'Red Line': '#D9262E',
  'Magenta Line': '#db2777',
  'Standard Line': '#64748b'
};

// Base schematic positions for initial Delhi Metro network
const BASE_COORDINATES = {
  'Kashmere Gate':       { x: 480, y: 55,  labelPos: 'right',  labelOff: { x: 20, y: 4 } },
  'Chandni Chowk':       { x: 480, y: 125, labelPos: 'right',  labelOff: { x: 20, y: 4 } },
  'New Delhi':           { x: 480, y: 195, labelPos: 'right',  labelOff: { x: 20, y: 4 } },
  'Rajiv Chowk':         { x: 480, y: 265, labelPos: 'top-left', labelOff: { x: -20, y: -18 } },
  'Patel Chowk':         { x: 480, y: 355, labelPos: 'left',   labelOff: { x: -20, y: 4 } },
  'Central Secretariat': { x: 480, y: 455, labelPos: 'left',   labelOff: { x: -20, y: 4 } },
  'Karol Bagh':          { x: 190, y: 265, labelPos: 'bottom', labelOff: { x: 0, y: 24 } },
  'Mandi House':         { x: 770, y: 265, labelPos: 'top',    labelOff: { x: 0, y: -20 } }
};

// Document Init
document.addEventListener('DOMContentLoaded', () => {
  initTabs();
  initForms();
  initMapControls();
  loadStations();
  checkBackendHealth();
});

// Toast notification helper
function showToast(message, type = 'info') {
  const container = document.getElementById('toast-container');
  if (!container) return;
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  toast.textContent = message;
  container.appendChild(toast);
  setTimeout(() => {
    toast.style.opacity = '0';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

// Check Backend Health
async function checkBackendHealth() {
  const dot = document.getElementById('backend-status-dot');
  const text = document.getElementById('backend-status-text');
  try {
    const res = await fetch(`${API_BASE}/api/stations`);
    if (res.ok) {
      if (dot) dot.className = 'status-indicator live';
      if (text) text.textContent = 'C++ Backend Connected (127.0.0.1:8080)';
    } else {
      throw new Error();
    }
  } catch (err) {
    if (dot) {
      dot.className = 'status-indicator';
      dot.style.background = '#ef4444';
    }
    if (text) text.textContent = 'Backend Offline';
  }
}

// Tab navigation
function initTabs() {
  const tabBtns = document.querySelectorAll('.tab-btn');
  const tabPanes = document.querySelectorAll('.tab-pane');

  tabBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      tabBtns.forEach(b => b.classList.remove('active'));
      tabPanes.forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      const target = document.getElementById(btn.dataset.tab);
      if (target) {
        target.classList.add('active');
        // If switching to network map, re-render to ensure correct sizing
        if (btn.dataset.tab === 'tab-network') {
          renderSvgMap();
        }
      }
    });
  });
}

// Load stations from C++ Backend
async function loadStations() {
  try {
    const [stationsRes, networkRes] = await Promise.all([
      fetch(`${API_BASE}/api/stations`),
      fetch(`${API_BASE}/api/network`)
    ]);

    const stationsData = await stationsRes.json();
    const networkData = await networkRes.json();

    if (!stationsData.success || !stationsData.stations) {
      showToast('Failed to load stations from C++ backend.', 'error');
      return;
    }

    allStations = stationsData.stations;
    stationMap.clear();
    allStations.forEach(s => stationMap.set(s.name, s));

    if (networkData.success && networkData.edges) {
      networkEdges = networkData.edges;
    }

    // Update Quick Stats
    const totalEl = document.getElementById('stat-total-stations');
    if (totalEl) totalEl.textContent = allStations.length;
    const interchangeCount = allStations.filter(s => s.isInterchange).length;
    const icEl = document.getElementById('stat-interchanges');
    if (icEl) icEl.textContent = `${interchangeCount} Stations`;

    // Populate Dropdowns
    populateSelect('source-select', allStations, 'Rajiv Chowk');
    populateSelect('dest-select', allStations, 'Kashmere Gate');
    populateSelect('traversal-start-select', allStations, 'Rajiv Chowk');
    populateSelect('conn-s1', allStations);
    populateSelect('conn-s2', allStations);

    // Render Station Catalog and SVG Map
    renderStationCatalog(allStations);
    renderSvgMap();
  } catch (err) {
    console.error('Network error loading stations:', err);
    showToast('Cannot connect to C++ backend on port 8080.', 'error');
  }
}

// Populate HTML Select Element
function populateSelect(selectId, stations, defaultValue = '') {
  const select = document.getElementById(selectId);
  if (!select) return;

  const currentVal = select.value || defaultValue;
  select.innerHTML = '<option value="" disabled>Select Station</option>';

  stations.forEach(s => {
    const opt = document.createElement('option');
    opt.value = s.name;
    const linesStr = s.lines ? s.lines.join(', ') : '';
    opt.textContent = `${s.name} ${s.isInterchange ? '⚡ [Interchange]' : ''} (${linesStr})`;
    select.appendChild(opt);
  });

  if (currentVal && stations.some(s => s.name === currentVal)) {
    select.value = currentVal;
  } else if (stations.length > 0) {
    select.selectedIndex = 1;
  }
}

// Form Handlers
function initForms() {
  // Swap stations button
  const swapBtn = document.getElementById('swap-stations-btn');
  if (swapBtn) {
    swapBtn.addEventListener('click', () => {
      const src = document.getElementById('source-select');
      const dest = document.getElementById('dest-select');
      const temp = src.value;
      src.value = dest.value;
      dest.value = temp;
    });
  }

  // Sample route chips
  document.querySelectorAll('.sample-chip').forEach(chip => {
    chip.addEventListener('click', () => {
      document.getElementById('source-select').value = chip.dataset.from;
      document.getElementById('dest-select').value = chip.dataset.to;
      document.getElementById('find-route-btn').click();
    });
  });

  // Find Shortest Route Form
  const routeForm = document.getElementById('route-form');
  if (routeForm) {
    routeForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const from = document.getElementById('source-select').value;
      const to = document.getElementById('dest-select').value;

      if (!from || !to) {
        showToast('Please select both starting and destination stations.', 'error');
        return;
      }

      await fetchShortestRoute(from, to);
    });
  }

  // Traversal Form (BFS & DFS)
  const traversalForm = document.getElementById('traversal-form');
  if (traversalForm) {
    traversalForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const start = document.getElementById('traversal-start-select').value;
      const algo = document.querySelector('input[name="traversal-algo"]:checked').value;
      await runTraversal(start, algo);
    });
  }

  // Station search input in catalog
  const searchInput = document.getElementById('station-search-input');
  if (searchInput) {
    searchInput.addEventListener('input', (e) => {
      const query = e.target.value.toLowerCase().trim();
      const filtered = allStations.filter(s =>
        s.name.toLowerCase().includes(query) ||
        (s.lines && s.lines.some(l => l.toLowerCase().includes(query)))
      );
      renderStationCatalog(filtered);
    });
  }

  // Add Station Form
  const addStationForm = document.getElementById('add-station-form');
  if (addStationForm) {
    addStationForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const name = document.getElementById('new-station-name').value.trim();
      const line = document.getElementById('new-station-line').value;

      if (!name) return;

      try {
        const res = await fetch(`${API_BASE}/api/station`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ name, line })
        });
        const data = await res.json();
        if (data.success) {
          showToast(`Station '${name}' added to C++ Graph!`, 'success');
          document.getElementById('new-station-name').value = '';
          await loadStations();

          // Immediately select and inspect new station on the map
          selectStation(name);
          // Switch to map tab so user sees their new station right away
          document.querySelector('.tab-btn[data-tab="tab-network"]').click();
        } else {
          showToast(data.message || 'Failed to add station.', 'error');
        }
      } catch (err) {
        showToast('Server communication error.', 'error');
      }
    });
  }

  // Add Connection Form
  const addConnForm = document.getElementById('add-connection-form');
  if (addConnForm) {
    addConnForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const s1 = document.getElementById('conn-s1').value;
      const s2 = document.getElementById('conn-s2').value;
      const dist = parseInt(document.getElementById('conn-dist').value, 10);

      if (!s1 || !s2 || s1 === s2 || isNaN(dist) || dist <= 0) {
        showToast('Please select two distinct stations and positive distance.', 'error');
        return;
      }

      try {
        const res = await fetch(`${API_BASE}/api/connection`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ station1: s1, station2: s2, distance: dist })
        });
        const data = await res.json();
        if (data.success) {
          showToast(`Track added between ${s1} and ${s2} (${dist} km)!`, 'success');
          document.getElementById('conn-dist').value = '';
          await loadStations();

          // Select station to highlight the new connection
          selectStation(s1);
          document.querySelector('.tab-btn[data-tab="tab-network"]').click();
        } else {
          showToast(data.message || 'Failed to add connection.', 'error');
        }
      } catch (err) {
        showToast('Server communication error.', 'error');
      }
    });
  }

  // Inspector Action Buttons
  const btnOrigin = document.getElementById('inspect-btn-origin');
  if (btnOrigin) {
    btnOrigin.addEventListener('click', () => {
      if (!selectedStationName) return;
      document.getElementById('source-select').value = selectedStationName;
      document.querySelector('.tab-btn[data-tab="tab-planner"]').click();
      showToast(`Selected '${selectedStationName}' as Origin Station`, 'info');
    });
  }

  const btnDest = document.getElementById('inspect-btn-dest');
  if (btnDest) {
    btnDest.addEventListener('click', () => {
      if (!selectedStationName) return;
      document.getElementById('dest-select').value = selectedStationName;
      document.querySelector('.tab-btn[data-tab="tab-planner"]').click();
      showToast(`Selected '${selectedStationName}' as Destination Station`, 'info');
    });
  }

  const btnTrav = document.getElementById('inspect-btn-traversal');
  if (btnTrav) {
    btnTrav.addEventListener('click', () => {
      if (!selectedStationName) return;
      document.getElementById('traversal-start-select').value = selectedStationName;
      document.querySelector('.tab-btn[data-tab="tab-traversal"]').click();
      showToast(`Ready to run traversal from '${selectedStationName}'`, 'info');
    });
  }

  const btnCloseInspect = document.getElementById('inspect-close');
  if (btnCloseInspect) {
    btnCloseInspect.addEventListener('click', () => {
      clearStationSelection();
    });
  }
}

// Map Zoom and Pan Controls
function initMapControls() {
  const btnIn = document.getElementById('map-zoom-in');
  const btnOut = document.getElementById('map-zoom-out');
  const btnReset = document.getElementById('map-zoom-reset');
  const svg = document.getElementById('metro-svg-canvas');
  const wrapper = document.getElementById('svg-map-wrapper');

  if (btnIn) {
    btnIn.addEventListener('click', () => {
      zoomLevel = Math.min(zoomLevel * 1.25, 3.0);
      updateSvgTransform();
    });
  }

  if (btnOut) {
    btnOut.addEventListener('click', () => {
      zoomLevel = Math.max(zoomLevel / 1.25, 0.5);
      updateSvgTransform();
    });
  }

  if (btnReset) {
    btnReset.addEventListener('click', () => {
      zoomLevel = 1.0;
      panX = 0;
      panY = 0;
      updateSvgTransform();
    });
  }

  if (svg) {
    // Mouse Drag Pan
    svg.addEventListener('mousedown', (e) => {
      // Ignore clicks on station nodes so clicking still selects station
      if (e.target.closest('.svg-node-group') || e.target.closest('.dist-badge-group')) return;
      isPanning = true;
      startPanX = e.clientX - panX;
      startPanY = e.clientY - panY;
    });

    window.addEventListener('mousemove', (e) => {
      if (!isPanning) return;
      panX = e.clientX - startPanX;
      panY = e.clientY - startPanY;
      updateSvgTransform();
    });

    window.addEventListener('mouseup', () => {
      isPanning = false;
    });

    // Mouse Wheel Zoom
    if (wrapper) {
      wrapper.addEventListener('wheel', (e) => {
        e.preventDefault();
        const factor = e.deltaY < 0 ? 1.15 : 0.87;
        zoomLevel = Math.min(Math.max(zoomLevel * factor, 0.5), 3.0);
        updateSvgTransform();
      }, { passive: false });
    }
  }
}

// Apply Zoom & Pan Transform to SVG Viewport Group
function updateSvgTransform() {
  const g = document.getElementById('svg-viewport-group');
  if (g) {
    g.setAttribute('transform', `translate(${panX}, ${panY}) scale(${zoomLevel})`);
  }
}

// Compute Dynamic Coordinates for All Stations (Baseline + Auto-layout for New/Isolated Stations)
function computeStationLayout() {
  const coords = {};
  const isolatedList = [];
  const connectedNewList = [];

  // 1. Assign fixed base coordinates to known initial stations
  allStations.forEach(s => {
    if (BASE_COORDINATES[s.name]) {
      coords[s.name] = { ...BASE_COORDINATES[s.name] };
    } else {
      const hasConnections = s.connections && s.connections.length > 0;
      if (!hasConnections) {
        isolatedList.push(s);
      } else {
        connectedNewList.push(s);
      }
    }
  });

  // 2. Position connected new stations logically relative to their neighbor
  connectedNewList.forEach(s => {
    const primaryConn = s.connections[0];
    const neighborName = primaryConn.station;
    const neighborPos = coords[neighborName] || { x: 480, y: 265 };

    // Determine direction based on neighbor's known line
    let offsetX = 0;
    let offsetY = 75; // Default extend downwards

    if (neighborName === 'Central Secretariat') {
      offsetY = 85; // Continue South along Yellow Line
      offsetX = 0;
    } else if (neighborName === 'Karol Bagh') {
      offsetX = -120; // Continue West along Blue Line
      offsetY = 0;
    } else if (neighborName === 'Mandi House') {
      offsetX = 120; // Continue East along Blue Line
      offsetY = 0;
    } else if (neighborName === 'Kashmere Gate') {
      offsetX = -130; // Along Red Line corridor
      offsetY = 0;
    } else {
      // Radial offset away from central Rajiv Chowk (480, 265)
      const dx = neighborPos.x - 480;
      const dy = neighborPos.y - 265;
      const len = Math.hypot(dx, dy) || 1;
      offsetX = Math.round((dx / len) * 85);
      offsetY = Math.round((dy / len) * 85);
    }

    coords[s.name] = {
      x: neighborPos.x + offsetX,
      y: neighborPos.y + offsetY,
      labelPos: offsetX < 0 ? 'left' : (offsetX > 0 ? 'right' : 'bottom'),
      labelOff: { x: offsetX < 0 ? -18 : 18, y: offsetY > 0 ? 18 : 4 }
    };
  });

  // 3. Position isolated stations in a dedicated shelf at the bottom
  isolatedList.forEach((s, idx) => {
    const shelfStartX = 180;
    const shelfSpacingX = 170;
    coords[s.name] = {
      x: shelfStartX + idx * shelfSpacingX,
      y: 520,
      isIsolated: true,
      labelPos: 'bottom',
      labelOff: { x: 0, y: 22 }
    };
  });

  // 4. Calculate bounding box to dynamically scale SVG viewBox if needed
  let minX = 100, maxX = 860, minY = 30, maxY = 530;
  Object.values(coords).forEach(p => {
    if (p.x < minX) minX = p.x;
    if (p.x > maxX) maxX = p.x;
    if (p.y < minY) minY = p.y;
    if (p.y > maxY) maxY = p.y;
  });

  return { coords, isolatedCount: isolatedList.length, viewBox: [minX - 70, minY - 40, (maxX - minX) + 140, (maxY - minY) + 90] };
}

// Main SVG Map Rendering Engine
function renderSvgMap() {
  const svg = document.getElementById('metro-svg-canvas');
  if (!svg) return;
  svg.innerHTML = '';

  const { coords, isolatedCount, viewBox } = computeStationLayout();

  // Set viewBox with dynamic bounds
  svg.setAttribute('viewBox', `${viewBox[0]} ${viewBox[1]} ${viewBox[2]} ${viewBox[3]}`);

  // Defs: Filters & Radial Gradients
  svg.innerHTML += `
    <defs>
      <filter id="node-glow" x="-50%" y="-50%" width="200%" height="200%">
        <feGaussianBlur stdDeviation="3.5" result="blur" />
        <feMerge>
          <feMergeNode in="blur" />
          <feMergeNode in="SourceGraphic" />
        </feMerge>
      </filter>
      <filter id="track-shadow" x="-20%" y="-20%" width="140%" height="140%">
        <feDropShadow dx="0" dy="2" stdDeviation="3" flood-color="#000000" flood-opacity="0.6"/>
      </filter>
    </defs>
  `;

  // Master Viewport Group for Pan & Zoom
  const viewport = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  viewport.id = 'svg-viewport-group';
  viewport.setAttribute('transform', `translate(${panX}, ${panY}) scale(${zoomLevel})`);
  svg.appendChild(viewport);

  // Background Grid / Schematic Guidelines (Subtle)
  const bgGrid = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  bgGrid.setAttribute('opacity', '0.12');
  viewport.appendChild(bgGrid);

  // Red Line Active Corridor Line through Kashmere Gate (East-West)
  const kgPos = coords['Kashmere Gate'];
  if (kgPos) {
    const redTrack = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    redTrack.innerHTML = `
      <line x1="${kgPos.x - 170}" y1="${kgPos.y}" x2="${kgPos.x + 170}" y2="${kgPos.y}" stroke="${LINE_COLORS['Red Line']}" stroke-width="8" stroke-linecap="round" stroke-dasharray="8,6" opacity="0.65"/>
      <text x="${kgPos.x - 180}" y="${kgPos.y + 4}" fill="${LINE_COLORS['Red Line']}" font-size="10px" font-weight="700" text-anchor="end" font-family="'Plus Jakarta Sans', sans-serif">◀ Red Line (Rithala)</text>
      <text x="${kgPos.x + 180}" y="${kgPos.y + 4}" fill="${LINE_COLORS['Red Line']}" font-size="10px" font-weight="700" text-anchor="start" font-family="'Plus Jakarta Sans', sans-serif">Red Line (Shaheed Sthal) ▶</text>
    `;
    viewport.appendChild(redTrack);
  }

  // Shelf for Isolated Stations if any exist
  if (isolatedCount > 0) {
    const shelf = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    shelf.innerHTML = `
      <rect x="130" y="475" width="${Math.max(isolatedCount * 170 + 80, 360)}" height="75" rx="8" fill="rgba(11, 19, 43, 0.55)" stroke="#f59e0b" stroke-width="1.5" stroke-dasharray="6,4" opacity="0.8"/>
      <text x="145" y="492" fill="#f59e0b" font-size="11px" font-weight="700" font-family="'Plus Jakarta Sans', sans-serif">⚠️ Isolated / Under Construction Stations (No Track Connected)</text>
    `;
    viewport.appendChild(shelf);
  }

  // Tracks Group
  const tracksGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  tracksGroup.id = 'svg-tracks-group';
  viewport.appendChild(tracksGroup);

  // Distance Badges Group (drawn on top of tracks)
  const badgesGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  badgesGroup.id = 'svg-badges-group';
  viewport.appendChild(badgesGroup);

  // Nodes Group (drawn on top of badges)
  const nodesGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  nodesGroup.id = 'svg-nodes-group';
  viewport.appendChild(nodesGroup);

  // 1. Draw Edges from C++ Backend
  const drawnEdges = new Set();

  allStations.forEach(s => {
    const p1 = coords[s.name];
    if (!p1 || !s.connections) return;

    s.connections.forEach(conn => {
      const edgeKey = [s.name, conn.station].sort().join('---');
      if (drawnEdges.has(edgeKey)) return;
      drawnEdges.add(edgeKey);

      const p2 = coords[conn.station];
      if (!p2) return;

      // Determine track color
      const otherStation = stationMap.get(conn.station);
      let trackColor = LINE_COLORS['Standard Line'];

      // Check if backend provided line in networkEdges
      const matchingEdge = networkEdges.find(e =>
        (e.source === s.name && e.target === conn.station) ||
        (e.source === conn.station && e.target === s.name)
      );

      if (matchingEdge && matchingEdge.line && LINE_COLORS[matchingEdge.line]) {
        trackColor = LINE_COLORS[matchingEdge.line];
      } else if (otherStation && s.lines) {
        // Intersect lines between station 1 and station 2
        for (const l of s.lines) {
          if (otherStation.lines && otherStation.lines.includes(l)) {
            trackColor = LINE_COLORS[l] || trackColor;
            break;
          }
        }
      }

      // Check if this is the Mandi House <-> Central Secretariat Violet curve
      const isVioletCurve = (s.name === 'Mandi House' && conn.station === 'Central Secretariat') ||
                            (s.name === 'Central Secretariat' && conn.station === 'Mandi House');

      let pathD = '';
      let midX = (p1.x + p2.x) / 2;
      let midY = (p1.y + p2.y) / 2;

      if (isVioletCurve) {
        // Smooth transit curve avoiding Patel Chowk
        const mh = coords['Mandi House'];
        const cs = coords['Central Secretariat'];
        pathD = `M ${mh.x} ${mh.y} C ${mh.x} 390, 620 ${cs.y}, ${cs.x} ${cs.y}`;
        midX = 645;
        midY = 410;
      } else {
        pathD = `M ${p1.x} ${p1.y} L ${p2.x} ${p2.y}`;
      }

      // Track Underlay (Dark border outline for high contrast)
      const trackUnderlay = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      trackUnderlay.setAttribute('d', pathD);
      trackUnderlay.setAttribute('stroke', '#080d1a');
      trackUnderlay.setAttribute('stroke-width', '11');
      trackUnderlay.setAttribute('stroke-linecap', 'round');
      trackUnderlay.setAttribute('stroke-linejoin', 'round');
      trackUnderlay.setAttribute('fill', 'none');
      tracksGroup.appendChild(trackUnderlay);

      // Track Main Colored Path
      const trackPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      trackPath.setAttribute('d', pathD);
      trackPath.setAttribute('stroke', trackColor);
      trackPath.setAttribute('stroke-width', '7.5');
      trackPath.setAttribute('stroke-linecap', 'round');
      trackPath.setAttribute('stroke-linejoin', 'round');
      trackPath.setAttribute('fill', 'none');
      trackPath.setAttribute('filter', 'url(#track-shadow)');
      trackPath.classList.add('svg-edge-path');
      trackPath.dataset.edgeKey = edgeKey;
      trackPath.dataset.s1 = s.name;
      trackPath.dataset.s2 = conn.station;
      tracksGroup.appendChild(trackPath);

      // Distance Badge Calculation with Perpendicular Offset
      const dx = p2.x - p1.x;
      const dy = p2.y - p1.y;
      const angle = Math.atan2(dy, dx);
      let badgeX = midX;
      let badgeY = midY;

      if (isVioletCurve) {
        badgeX = 650;
        badgeY = 405;
      } else if (Math.abs(dx) < 10) {
        // Vertical track: offset to the left or right
        badgeX = midX + 18;
      } else if (Math.abs(dy) < 10) {
        // Horizontal track: offset above
        badgeY = midY - 14;
      } else {
        badgeX = midX - Math.sin(angle) * 16;
        badgeY = midY + Math.cos(angle) * 16;
      }

      // Distance Badge Group
      const badgeG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      badgeG.classList.add('dist-badge-group');
      badgeG.dataset.edgeKey = edgeKey;

      const badgeRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      badgeRect.setAttribute('x', badgeX - 16);
      badgeRect.setAttribute('y', badgeY - 10);
      badgeRect.setAttribute('width', '32');
      badgeRect.setAttribute('height', '20');
      badgeRect.setAttribute('rx', '4');
      badgeRect.setAttribute('fill', '#091024');
      badgeRect.setAttribute('stroke', '#1e293b');
      badgeRect.setAttribute('stroke-width', '1.2');

      const badgeText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      badgeText.setAttribute('x', badgeX);
      badgeText.setAttribute('y', badgeY + 4);
      badgeText.setAttribute('text-anchor', 'middle');
      badgeText.setAttribute('fill', '#cbd5e1');
      badgeText.setAttribute('font-size', '10.5px');
      badgeText.setAttribute('font-family', "'JetBrains Mono', monospace");
      badgeText.setAttribute('font-weight', '700');
      badgeText.textContent = `${conn.distance}k`;

      badgeG.appendChild(badgeRect);
      badgeG.appendChild(badgeText);
      badgesGroup.appendChild(badgeG);
    });
  });

  // 2. Draw Station Nodes
  allStations.forEach(s => {
    const pos = coords[s.name];
    if (!pos) return;

    const isInterchange = s.isInterchange;
    const isIsolated = pos.isIsolated || (!s.connections || s.connections.length === 0);

    const nodeG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    nodeG.classList.add('svg-node-group');
    nodeG.dataset.station = s.name;

    // Selection Pulsing Halo (shown when active)
    const halo = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    halo.classList.add('selection-halo');
    halo.setAttribute('cx', pos.x);
    halo.setAttribute('cy', pos.y);
    halo.setAttribute('r', isInterchange ? '22' : '17');
    halo.setAttribute('stroke', '#38bdf8');
    halo.setAttribute('stroke-width', '2.5');
    halo.setAttribute('fill', 'none');
    nodeG.appendChild(halo);

    if (isIsolated) {
      // Isolated Station Node: Dashed Amber Ring + Alert Dot
      const outerRing = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      outerRing.setAttribute('cx', pos.x);
      outerRing.setAttribute('cy', pos.y);
      outerRing.setAttribute('r', '13');
      outerRing.setAttribute('fill', '#0a0f1d');
      outerRing.setAttribute('stroke', '#f59e0b');
      outerRing.setAttribute('stroke-width', '2.5');
      outerRing.setAttribute('stroke-dasharray', '4,3');
      nodeG.appendChild(outerRing);

      const primaryLineColor = (s.lines && s.lines[0]) ? (LINE_COLORS[s.lines[0]] || '#f59e0b') : '#f59e0b';
      const innerDot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      innerDot.setAttribute('cx', pos.x);
      innerDot.setAttribute('cy', pos.y);
      innerDot.setAttribute('r', '6');
      innerDot.setAttribute('fill', primaryLineColor);
      nodeG.appendChild(innerDot);
    }
    else if (isInterchange) {
      // Interchange Station: Distinctive Double-Ring with Dual Line Colors
      const outerRing = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      outerRing.setAttribute('cx', pos.x);
      outerRing.setAttribute('cy', pos.y);
      outerRing.setAttribute('r', '15');
      outerRing.setAttribute('fill', '#0a0f1d');
      outerRing.setAttribute('stroke', '#ffffff');
      outerRing.setAttribute('stroke-width', '3');
      outerRing.setAttribute('filter', 'url(#node-glow)');
      nodeG.appendChild(outerRing);

      // Render split circle for interchange lines
      const line1 = (s.lines && s.lines[0]) ? s.lines[0] : 'Yellow Line';
      const line2 = (s.lines && s.lines[1]) ? s.lines[1] : (s.lines[0] || 'Blue Line');
      const c1 = LINE_COLORS[line1] || '#0072CE';
      const c2 = LINE_COLORS[line2] || '#FFC600';

      // Left semi-circle
      const leftSemi = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      leftSemi.setAttribute('d', `M ${pos.x} ${pos.y - 11} A 11 11 0 0 0 ${pos.x} ${pos.y + 11} Z`);
      leftSemi.setAttribute('fill', c1);
      nodeG.appendChild(leftSemi);

      // Right semi-circle
      const rightSemi = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      rightSemi.setAttribute('d', `M ${pos.x} ${pos.y - 11} A 11 11 0 0 1 ${pos.x} ${pos.y + 11} Z`);
      rightSemi.setAttribute('fill', c2);
      nodeG.appendChild(rightSemi);

      // Inner white hub core
      const hubDot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      hubDot.setAttribute('cx', pos.x);
      hubDot.setAttribute('cy', pos.y);
      hubDot.setAttribute('r', '5');
      hubDot.setAttribute('fill', '#ffffff');
      hubDot.setAttribute('stroke', '#0a0f1d');
      hubDot.setAttribute('stroke-width', '1.5');
      nodeG.appendChild(hubDot);
    }
    else {
      // Ordinary Station: Solid Node with Station's Line Color
      const primaryLine = (s.lines && s.lines[0]) ? s.lines[0] : 'Yellow Line';
      const nodeColor = LINE_COLORS[primaryLine] || '#FFC600';

      const outerRing = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      outerRing.classList.add('station-core');
      outerRing.setAttribute('cx', pos.x);
      outerRing.setAttribute('cy', pos.y);
      outerRing.setAttribute('r', '9');
      outerRing.setAttribute('fill', nodeColor);
      outerRing.setAttribute('stroke', '#0a0f1d');
      outerRing.setAttribute('stroke-width', '2.5');
      outerRing.setAttribute('filter', 'url(#node-glow)');
      nodeG.appendChild(outerRing);

      const innerCore = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      innerCore.setAttribute('cx', pos.x);
      innerCore.setAttribute('cy', pos.y);
      innerCore.setAttribute('r', '4.5');
      innerCore.setAttribute('fill', '#ffffff');
      nodeG.appendChild(innerCore);
    }

    // Station Name Label
    const labelText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    labelText.classList.add('station-label-text');

    let lx = pos.x + (pos.labelOff ? pos.labelOff.x : 18);
    let ly = pos.y + (pos.labelOff ? pos.labelOff.y : 4);
    let anchor = 'start';

    if (pos.labelPos === 'left' || pos.labelPos === 'top-left') {
      anchor = 'end';
    } else if (pos.labelPos === 'bottom' || pos.labelPos === 'top') {
      anchor = 'middle';
    }

    labelText.setAttribute('x', lx);
    labelText.setAttribute('y', ly);
    labelText.setAttribute('text-anchor', anchor);
    labelText.textContent = isIsolated ? `${s.name} (Isolated)` : s.name;
    nodeG.appendChild(labelText);

    // Click Handler: Select & Inspect Station
    nodeG.addEventListener('click', (e) => {
      e.stopPropagation();
      selectStation(s.name);
    });

    nodesGroup.appendChild(nodeG);
  });

  // Click on empty canvas deselects
  svg.addEventListener('click', (e) => {
    if (!e.target.closest('.svg-node-group')) {
      clearStationSelection();
    }
  });

  // Re-apply selection if a station was previously selected
  if (selectedStationName) {
    applyStationSelectionEffects(selectedStationName);
  }
}

// Select Station and Update Inspector Panel + Map Focus
function selectStation(stationName) {
  selectedStationName = stationName;
  applyStationSelectionEffects(stationName);

  const station = stationMap.get(stationName);
  if (!station) return;

  const promptBox = document.getElementById('inspector-prompt');
  const detailsBox = document.getElementById('inspector-details');
  if (promptBox) promptBox.classList.add('hidden');
  if (detailsBox) detailsBox.classList.remove('hidden');

  // Fill Header
  document.getElementById('inspect-name').textContent = station.name;
  const badge = document.getElementById('inspect-badge');
  const statusText = document.getElementById('inspect-status-text');

  const isIsolated = !station.connections || station.connections.length === 0;

  if (isIsolated) {
    badge.className = 'badge';
    badge.style.background = 'rgba(245, 158, 11, 0.2)';
    badge.style.color = '#fde047';
    badge.style.borderColor = 'rgba(245, 158, 11, 0.4)';
    badge.textContent = 'Isolated Station (No Track)';
    statusText.textContent = 'Unconnected';
  } else if (station.isInterchange) {
    badge.className = 'badge';
    badge.style.background = 'rgba(234, 179, 8, 0.2)';
    badge.style.color = '#ffc600';
    badge.style.borderColor = 'rgba(234, 179, 8, 0.4)';
    badge.textContent = `Interchange Hub (${station.lines.length} Lines)`;
    statusText.textContent = 'Connected';
  } else {
    badge.className = 'badge';
    badge.style.background = 'rgba(0, 114, 206, 0.2)';
    badge.style.color = '#38bdf8';
    badge.style.borderColor = 'rgba(0, 114, 206, 0.4)';
    badge.textContent = 'Regular Station';
    statusText.textContent = 'Connected';
  }

  // Fill Lines
  const linesContainer = document.getElementById('inspect-lines-chips');
  linesContainer.innerHTML = '';
  if (station.lines && station.lines.length > 0) {
    station.lines.forEach(line => {
      const chip = document.createElement('span');
      chip.className = 'chip';
      chip.style.background = LINE_COLORS[line] || '#64748b';
      if (line === 'Yellow Line') chip.style.color = '#000';
      chip.textContent = line;
      linesContainer.appendChild(chip);
    });
  } else {
    linesContainer.innerHTML = '<span class="text-muted">No line assigned</span>';
  }

  // Fill Connections List
  const connList = document.getElementById('inspect-neighbors-list');
  connList.innerHTML = '';
  if (station.connections && station.connections.length > 0) {
    station.connections.forEach(conn => {
      const neighborStation = stationMap.get(conn.station);
      const neighborLines = neighborStation ? neighborStation.lines : [];

      const row = document.createElement('div');
      row.className = 'neighbor-row';
      row.innerHTML = `
        <span class="neighbor-name">➜ ${conn.station}</span>
        <div style="display:flex;gap:0.4rem;align-items:center;">
          <span style="font-size:0.75rem;color:var(--text-muted);">${neighborLines.join(', ')}</span>
          <span class="neighbor-dist">${conn.distance} km</span>
        </div>
      `;
      connList.appendChild(row);
    });
  } else {
    connList.innerHTML = `
      <div class="transfer-pill" style="margin:0;font-size:0.8rem;">
        <span>⚠️ This station is isolated. Use the 'Add Metro Track Connection' tab to link it to adjacent stations.</span>
      </div>
    `;
  }
}

// Highlight connected edges and dim non-related nodes/edges
function applyStationSelectionEffects(stationName) {
  const station = stationMap.get(stationName);
  if (!station) return;

  const neighborNames = new Set(station.connections ? station.connections.map(c => c.station) : []);

  // Update Node Styles
  document.querySelectorAll('.svg-node-group').forEach(nodeG => {
    const sName = nodeG.dataset.station;
    const isTarget = sName === stationName;
    const isNeighbor = neighborNames.has(sName);

    nodeG.classList.remove('active', 'dimmed');
    if (isTarget) {
      nodeG.classList.add('active');
    } else if (!isNeighbor) {
      nodeG.classList.add('dimmed');
    }
  });

  // Update Edge Paths
  document.querySelectorAll('.svg-edge-path').forEach(edgePath => {
    const s1 = edgePath.dataset.s1;
    const s2 = edgePath.dataset.s2;
    const isConnected = (s1 === stationName || s2 === stationName);

    edgePath.classList.remove('highlighted', 'dimmed');
    if (isConnected) {
      edgePath.classList.add('highlighted');
    } else {
      edgePath.classList.add('dimmed');
    }
  });

  // Update Distance Badges
  document.querySelectorAll('.dist-badge-group').forEach(badgeG => {
    const edgeKey = badgeG.dataset.edgeKey;
    const parts = edgeKey ? edgeKey.split('---') : [];
    const isConnected = parts.includes(stationName);

    badgeG.classList.remove('dimmed');
    if (!isConnected) {
      badgeG.classList.add('dimmed');
    }
  });
}

// Clear Station Selection
function clearStationSelection() {
  selectedStationName = null;
  const promptBox = document.getElementById('inspector-prompt');
  const detailsBox = document.getElementById('inspector-details');
  if (promptBox) promptBox.classList.remove('hidden');
  if (detailsBox) detailsBox.classList.add('hidden');

  document.querySelectorAll('.svg-node-group').forEach(n => n.classList.remove('active', 'dimmed'));
  document.querySelectorAll('.svg-edge-path').forEach(e => e.classList.remove('highlighted', 'dimmed'));
  document.querySelectorAll('.dist-badge-group').forEach(b => b.classList.remove('dimmed'));
}

// Fetch Shortest Route from C++ Backend
async function fetchShortestRoute(from, to) {
  const btn = document.getElementById('find-route-btn');
  const originalHtml = btn.innerHTML;
  btn.disabled = true;
  btn.innerHTML = '<span>Calculating in C++...</span>';

  try {
    const url = `${API_BASE}/api/route?from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}`;
    const res = await fetch(url);
    const data = await res.json();

    if (!data.success) {
      showToast(data.message || 'Route not found.', 'error');
      return;
    }

    displayRouteResult(data);
  } catch (err) {
    console.error('Error fetching route:', err);
    showToast('Failed to connect to C++ backend.', 'error');
  } finally {
    btn.disabled = false;
    btn.innerHTML = originalHtml;
  }
}

// Render Route Results to DOM
function displayRouteResult(data) {
  document.getElementById('result-placeholder').classList.add('hidden');
  const details = document.getElementById('result-details');
  details.classList.remove('hidden');

  // Metrics
  document.getElementById('res-distance').textContent = data.totalDistance;
  document.getElementById('res-fare').textContent = data.fare;
  document.getElementById('res-time').textContent = data.estimatedMinutes;
  document.getElementById('res-interchanges').textContent = data.interchangesCount;

  // Interchanges Alert Box
  const alertBox = document.getElementById('interchange-alert-box');
  alertBox.innerHTML = '';

  if (data.interchangesCount === 0) {
    const directBox = document.createElement('div');
    directBox.className = 'transfer-pill';
    directBox.style.background = 'rgba(16, 185, 129, 0.12)';
    directBox.style.borderColor = 'rgba(16, 185, 129, 0.35)';
    directBox.style.color = '#6ee7b7';
    directBox.innerHTML = `
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
      <span><strong>Direct Journey:</strong> No line changes required! Seamless travel on ${data.lineSegments[0] || 'Metro'}.</span>
    `;
    alertBox.appendChild(directBox);
  } else {
    data.interchangeStations.forEach(ic => {
      const pill = document.createElement('div');
      pill.className = 'transfer-pill';
      pill.innerHTML = `
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="17 1 21 5 17 9"/><path d="M3 11V9a4 4 0 0 1 4-4h14"/><polyline points="7 23 3 19 7 15"/><path d="M21 13v2a4 4 0 0 1-4 4H3"/></svg>
        <span><strong>Transfer at ${ic.station}:</strong> Switch from <span style="color:${LINE_COLORS[ic.fromLine] || '#fff'}">${ic.fromLine}</span> &rarr; <span style="color:${LINE_COLORS[ic.toLine] || '#fff'}">${ic.toLine}</span></span>
      `;
      alertBox.appendChild(pill);
    });
  }

  // Render Visual Timeline
  const timeline = document.getElementById('route-timeline-steps');
  timeline.innerHTML = '';

  const path = data.shortestPath;
  const segments = data.lineSegments;

  path.forEach((stationName, idx) => {
    const isStart = idx === 0;
    const isEnd = idx === path.length - 1;
    const isInterchangeStation = data.interchangeStations && data.interchangeStations.some(ic => ic.station === stationName);

    const step = document.createElement('div');
    step.className = 'timeline-step';

    // Station track column
    const trackCol = document.createElement('div');
    trackCol.className = 'timeline-track-col';

    const node = document.createElement('div');
    node.className = 'station-node';
    if (isStart) node.classList.add('start');
    else if (isEnd) node.classList.add('end');
    else if (isInterchangeStation) node.classList.add('interchange');

    trackCol.appendChild(node);

    if (!isEnd) {
      const lineName = segments[idx] || 'Standard Line';
      const cleanLineClass = lineName.replace(' Line', '');
      const trackLine = document.createElement('div');
      trackLine.className = `track-line line-${cleanLineClass}`;
      trackCol.appendChild(trackLine);
    }

    // Station info column
    const info = document.createElement('div');
    info.className = 'timeline-info';

    const nameRow = document.createElement('div');
    nameRow.className = 'station-name-row';

    const nameSpan = document.createElement('span');
    nameSpan.className = 'timeline-station-name';
    nameSpan.textContent = stationName;
    nameRow.appendChild(nameSpan);

    if (isStart) {
      nameRow.innerHTML += '<span class="timeline-badge" style="background:rgba(16,185,129,0.2);color:#34d399">Origin</span>';
    } else if (isEnd) {
      nameRow.innerHTML += '<span class="timeline-badge" style="background:rgba(239,68,68,0.2);color:#f87171">Destination</span>';
    } else if (isInterchangeStation) {
      nameRow.innerHTML += '<span class="timeline-badge" style="background:rgba(234,179,8,0.2);color:#fde047">Interchange</span>';
    }

    info.appendChild(nameRow);

    if (!isEnd && data.routeDetails && data.routeDetails[idx]) {
      const detail = data.routeDetails[idx];
      const meta = document.createElement('div');
      meta.className = 'segment-meta';
      meta.innerHTML = `
        <span style="color: ${LINE_COLORS[detail.line] || '#fff'}">&#9679; ${detail.line}</span>
        <span>&bull; ${detail.distance} km</span>
      `;
      info.appendChild(meta);
    }

    step.appendChild(trackCol);
    step.appendChild(info);
    timeline.appendChild(step);
  });
}

// Run BFS / DFS Graph Traversal
async function runTraversal(startStation, algo) {
  const btn = document.getElementById('run-traversal-btn');
  btn.disabled = true;

  try {
    const url = `${API_BASE}/api/${algo}?start=${encodeURIComponent(startStation)}`;
    const res = await fetch(url);
    const data = await res.json();

    if (!data.success) {
      showToast(data.message || 'Traversal failed.', 'error');
      return;
    }

    document.getElementById('traversal-placeholder').classList.add('hidden');
    const resBox = document.getElementById('traversal-results');
    resBox.classList.remove('hidden');

    document.getElementById('traversal-algo-badge').textContent = data.algorithm;
    document.getElementById('traversal-start-node').textContent = data.start;
    document.getElementById('traversal-complexity').textContent = data.timeComplexity;
    document.getElementById('traversal-count').textContent = data.traversal.length;
    document.getElementById('traversal-explanation').textContent = data.explanation;

    const seqContainer = document.getElementById('traversal-sequence-container');
    seqContainer.innerHTML = '';

    // Animate traversal sequence steps
    data.traversal.forEach((stName, idx) => {
      const stationObj = stationMap.get(stName);
      const lines = stationObj ? stationObj.lines : [];

      const pill = document.createElement('div');
      pill.className = 'step-pill';
      pill.style.animationDelay = `${idx * 0.08}s`;

      pill.innerHTML = `
        <div class="step-number">${idx + 1}</div>
        <div class="step-station">${stName}</div>
        <div class="line-chips">
          ${lines.map(l => `<span class="chip" style="background:${LINE_COLORS[l] || '#555'}">${l}</span>`).join('')}
        </div>
      `;
      seqContainer.appendChild(pill);
    });
  } catch (err) {
    showToast('Failed to run traversal on C++ backend.', 'error');
  } finally {
    btn.disabled = false;
  }
}

// Render Station Catalog Grid
function renderStationCatalog(stations) {
  const grid = document.getElementById('stations-catalog-grid');
  if (!grid) return;
  grid.innerHTML = '';

  stations.forEach(s => {
    const card = document.createElement('div');
    card.className = 'station-item-card';

    const linesHtml = s.lines ? s.lines.map(l =>
      `<span class="chip" style="background:${LINE_COLORS[l] || '#555'};${l === 'Yellow Line' ? 'color:#000;' : ''}">${l}</span>`
    ).join(' ') : '';

    const connStr = s.connections && s.connections.length > 0 ?
      s.connections.map(c => `${c.station} (${c.distance}km)`).join(', ') : '⚠️ No track connections (Isolated)';

    card.innerHTML = `
      <div class="station-item-header">
        <span class="station-item-name">${s.name}</span>
        ${s.isInterchange ? '<span class="badge" style="background:rgba(234,179,8,0.2);color:#ffc600">Interchange</span>' : ''}
        ${(!s.connections || s.connections.length === 0) ? '<span class="badge" style="background:rgba(245,158,11,0.2);color:#fde047">Isolated</span>' : ''}
      </div>
      <div class="station-item-lines">${linesHtml}</div>
      <div class="station-connections"><strong>Links:</strong> ${connStr}</div>
    `;

    // Click selects station on map and opens inspector
    card.addEventListener('click', () => {
      document.querySelector('.tab-btn[data-tab="tab-network"]').click();
      selectStation(s.name);
      showToast(`Selected '${s.name}' on Metro Network Map`, 'info');
    });

    grid.appendChild(card);
  });
}
