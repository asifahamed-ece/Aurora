/* ===========================================================================
 * Aurora — Dashboard client
 *
 * Single-file, no dependencies. Talks to the ESP32-C3 over WebSocket and
 * fetches /api/state on first paint so the dashboard is never blank.
 *
 * WS reconnect: exponential backoff (1s → 16s cap).
 * Hash route:  #letter opens the secret letter modal.
 * =========================================================================== */

(function () {
  'use strict';

  // --- DOM refs ---
  const $time       = document.getElementById('time');
  const $date       = document.getElementById('date');
  const $message    = document.getElementById('message');
  const $wifiGlyph  = document.getElementById('wifi-glyph');
  const $ledDot     = document.getElementById('led-dot');
  const $batFill    = document.getElementById('battery-fill');
  const $batPct     = document.getElementById('battery-pct');
  const $batMv      = document.getElementById('battery-mv');
  const $cntUp      = document.getElementById('cnt-up');
  const $cntSel     = document.getElementById('cnt-sel');
  const $cntDown    = document.getElementById('cnt-down');
  const $cntWifi    = document.getElementById('cnt-wifi');
  const $reconnect  = document.getElementById('reconnect');
  const $modal      = document.getElementById('letter-modal');
  const $modalClose = document.getElementById('modal-close');
  const $letterBody = document.getElementById('letter-content');

  const MESSAGES = window.AURORA_MESSAGES || [];

  // --- State ---
  let prevCounters = { up: 0, sel: 0, down: 0, wifi: 0 };
  let ws = null;
  let reconnectDelay = 1000;
  let reconnectTimer = null;
  let overlayTimer = null;     // delays showing "reconnecting" so a slow first
                               //   WS handshake doesn't flash the overlay
  let lastSeen = 0;            // timestamp of last WS message
  let stuckTimer = null;
  let prevSecond = -1;
  let letterLoaded = false;
  let gotFirstState = false;   // true once we have any data from REST or WS

  // ====================================================================
  //  Message selection
  // ====================================================================
  function pickMessageByDoy(doy, serverIdx) {
    // Trust the server's index if provided
    if (typeof serverIdx === 'number' && serverIdx >= 0 && serverIdx < MESSAGES.length) {
      return MESSAGES[serverIdx];
    }
    // Fallback: dayOfYear % 30
    if (MESSAGES.length === 0) return '—';
    return MESSAGES[((doy || 1) - 1) % MESSAGES.length];
  }

  function setMessage(text) {
    if ($message.textContent === text) return;
    $message.classList.add('fading');
    setTimeout(() => {
      $message.textContent = text;
      $message.classList.remove('fading');
    }, 350);
  }

  // ====================================================================
  //  Battery
  // ====================================================================
  function setBattery(b) {
    if (!b) return;
    const pct = Math.max(0, Math.min(100, b.pct || 0));
    $batFill.style.width = pct + '%';
    $batPct.textContent = pct + '%';
    $batMv.textContent  = (b.mv || 0) + ' mV';
    $batFill.classList.remove('low', 'crit');
    if (b.state === 'LOW' || pct < 30)        $batFill.classList.add('low');
    if (b.state === 'CRIT' || pct < 15)       $batFill.classList.add('crit');
  }

  // ====================================================================
  //  WiFi + LED
  // ====================================================================
  function setWifi(on) {
    $wifiGlyph.classList.toggle('on', !!on);
  }
  function setLed(on) {
    $ledDot.classList.toggle('off', !on);
  }

  // ====================================================================
  //  Counters
  // ====================================================================
  function bumpCounter(el, prev, next) {
    if (next > prev) {
      el.classList.add('bump');
      el.textContent = next;
      setTimeout(() => el.classList.remove('bump'), 200);
    } else {
      el.textContent = next;
    }
  }
  function setCounters(b) {
    if (!b) return;
    bumpCounter($cntUp,   prevCounters.up,   b.up   || 0); prevCounters.up   = b.up   || 0;
    bumpCounter($cntSel,  prevCounters.sel,  b.sel  || 0); prevCounters.sel  = b.sel  || 0;
    bumpCounter($cntDown, prevCounters.down, b.down || 0); prevCounters.down = b.down || 0;
    bumpCounter($cntWifi, prevCounters.wifi, b.wifi || 0); prevCounters.wifi = b.wifi || 0;
  }

  // ====================================================================
  //  Time + date
  // ====================================================================
  function setTime(t) {
    if (!t) return;
    $time.textContent = t;
    // Subtle per-second pulse
    const sec = parseInt(t.split(':')[2], 10);
    if (!isNaN(sec) && sec !== prevSecond) {
      prevSecond = sec;
      $time.classList.add('tick');
      setTimeout(() => $time.classList.remove('tick'), 180);
    }
  }
  function setDate(d) {
    if (d) $date.textContent = d;
  }

  // ====================================================================
  //  State apply (single function used by REST + WS)
  // ====================================================================
  function applyState(s) {
    if (!s) return;
    if (s.time) setTime(s.time);
    if (s.date) setDate(s.date);
    if (s.bat !== undefined) setBattery(s.bat);
    if (s.wifi !== undefined) setWifi(s.wifi);
    if (s.led  !== undefined) setLed(s.led === 'on');
    if (s.btns) setCounters(s.btns);
    if (s.msg_idx !== undefined || s.doy !== undefined) {
      setMessage(pickMessageByDoy(s.doy, s.msg_idx));
    }
    lastSeen = Date.now();
    if (!gotFirstState) {
      gotFirstState = true;
      hideReconnectSoon();
    }
  }

  // ====================================================================
  //  REST: /api/state (initial paint)
  // ====================================================================
  async function fetchInitialState() {
    try {
      const r = await fetch('api/state', { cache: 'no-store' });
      if (r.ok) {
        const s = await r.json();
        applyState(s);
      }
    } catch (_) { /* will retry via WS */ }
  }

  // ====================================================================
  //  WebSocket
  // ====================================================================
  function connectWs() {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    const url   = proto + '://' + location.host + '/ws';
    try {
      ws = new WebSocket(url);
    } catch (_) {
      scheduleReconnect();
      return;
    }

    ws.addEventListener('open', () => {
      reconnectDelay = 1000;
      hideReconnectSoon();
    });

    ws.addEventListener('message', (ev) => {
      try {
        const msg = JSON.parse(ev.data);
        if (msg.type === 'state') applyState(msg);
        else if (msg.type === 'hello') {
          // version handshake — nothing to do yet
        }
        else if (msg.type === 'ping') {
          // optional pong
          try { ws.send(JSON.stringify({ type: 'pong' })); } catch (_) {}
        }
      } catch (_) { /* malformed frame, ignore */ }
    });

    ws.addEventListener('close', () => {
      ws = null;
      scheduleReconnect();
    });
    ws.addEventListener('error', () => {
      if (ws) try { ws.close(); } catch (_) {}
    });
  }

  function scheduleReconnect() {
    if (reconnectTimer) return;
    // Don't flash "reconnecting" during the first ~3s — most phones
    // finish the AP handshake inside that window and never need to see it.
    if (overlayTimer) clearTimeout(overlayTimer);
    overlayTimer = setTimeout(() => {
      overlayTimer = null;
      if (!gotFirstState) $reconnect.hidden = false;
    }, 3000);
    reconnectTimer = setTimeout(() => {
      reconnectTimer = null;
      connectWs();
    }, reconnectDelay);
    reconnectDelay = Math.min(reconnectDelay * 2, 16000);
  }

  function hideReconnectSoon() {
    if (overlayTimer) { clearTimeout(overlayTimer); overlayTimer = null; }
    $reconnect.hidden = true;
  }

  // Stuck-connection detector: if we haven't seen a frame in 15s, force reconnect
  setInterval(() => {
    if (!ws || ws.readyState !== 1) return;
    if (Date.now() - lastSeen > 15000) {
      try { ws.close(); } catch (_) {}
    }
  }, 5000);

  // ====================================================================
  //  Letter modal
  // ====================================================================
  async function openLetter() {
    $modal.hidden = false;
    document.body.classList.add('modal-open');
    if (!letterLoaded) {
      try {
        const r = await fetch('letter.html', { cache: 'no-cache' });
        if (r.ok) {
          $letterBody.innerHTML = await r.text();
          letterLoaded = true;
        } else {
          $letterBody.innerHTML = '<p>(the letter could not be loaded)</p>';
        }
      } catch (_) {
        $letterBody.innerHTML = '<p>(the letter could not be loaded)</p>';
      }
    }
    $modalClose.focus();
  }
  function closeLetter() {
    $modal.hidden = true;
    document.body.classList.remove('modal-open');
    if (location.hash === '#letter') {
      // Avoid retriggering on hashchange
      history.replaceState(null, '', location.pathname);
    }
  }
  function checkRoute() {
    if (location.hash === '#letter') openLetter();
    else closeLetter();
  }
  $modalClose.addEventListener('click', closeLetter);
  $modal.addEventListener('click', (e) => {
    if (e.target === $modal) closeLetter();
  });
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && !$modal.hidden) closeLetter();
  });
  window.addEventListener('hashchange', checkRoute);

  // ====================================================================
  //  Boot
  // ====================================================================
  function boot() {
    // Hide the loading message in case server hasn't responded yet
    if (MESSAGES.length > 0) {
      setMessage(MESSAGES[0]);
    } else {
      setMessage('—');
    }
    fetchInitialState();
    connectWs();
    checkRoute();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', boot);
  } else {
    boot();
  }
})();
