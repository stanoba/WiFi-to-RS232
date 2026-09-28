#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Update.h>
#include "Config.h"
#include "Timezones.h"
#include "BatteryData.h"
#include "PrometheusExporter.h"
#include "ConsoleLog.h"
#include "PylonSerial.h"
#include "MqttClientManager.h"
#include "BatteryHistory.h"
#include "SystemStats.h"
#include "PeerDiscovery.h"

extern time_t lastNtpSyncTimestamp;
extern void triggerNtpSync();

// Pylon Smart Monitor SVG Vector Logo (Battery + Telemetry Pulse)
static const char PYLON_LOGO_SVG[] PROGMEM = 
R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 330 44" style="height:42px;width:auto;display:block;">
  <defs>
    <linearGradient id="pylonGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#77b243"/>
      <stop offset="100%" stop-color="#00b3ba"/>
    </linearGradient>
  </defs>
  <g transform="translate(2, 2)">
    <!-- WiFi Radiance -->
    <path d="M24 10 A15 15 0 0 1 37 23" fill="none" stroke="url(#pylonGrad)" stroke-width="2.8" stroke-linecap="round"/>
    <path d="M29 5 A22 22 0 0 1 44 20" fill="none" stroke="url(#pylonGrad)" stroke-width="2.8" stroke-linecap="round"/>
    <!-- Battery Nub -->
    <rect x="8" y="2" width="7.5" height="3.5" rx="1.2" fill="url(#pylonGrad)"/>
    <!-- Battery Cell -->
    <rect x="2.5" y="5.5" width="18.5" height="34" rx="4.5" fill="none" stroke="url(#pylonGrad)" stroke-width="2.8"/>
    <!-- Lightning Bolt -->
    <path d="M12.5 12 L7.5 22.5 L12.5 22.5 L10.5 31 L17 20.5 L12 20.5 Z" fill="url(#pylonGrad)"/>
  </g>
  <!-- Wordmark: PYLON SMART MONITOR -->
  <text class="brand-pylon" x="52" y="29.5" font-family="system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif" font-weight="800" font-size="21.5" fill="#171c61" letter-spacing="1.2">PYLON</text>
  <text class="brand-sub" x="135" y="29.5" font-family="system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif" font-weight="600" font-size="15" fill="#008b91" letter-spacing="1.6">SMART</text>
  <text class="brand-sub" x="196" y="29.5" font-family="system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif" font-weight="600" font-size="15" fill="#008b91" letter-spacing="1.6">MONITOR</text>
</svg>)rawliteral";

// Pylon Smart Monitor SVG Favicon (Battery + Telemetry Pulse)
static const char PYLON_FAVICON_SVG[] PROGMEM = 
R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 44 44">
  <defs>
    <linearGradient id="favPylonGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#77b243"/>
      <stop offset="100%" stop-color="#00b3ba"/>
    </linearGradient>
  </defs>
  <g transform="translate(0.5, 3)">
    <path d="M23 10 A14 14 0 0 1 35 22" fill="none" stroke="url(#favPylonGrad)" stroke-width="2.8" stroke-linecap="round"/>
    <path d="M28 5 A21 21 0 0 1 42 19" fill="none" stroke="url(#favPylonGrad)" stroke-width="2.8" stroke-linecap="round"/>
    <rect x="7.5" y="2" width="7" height="3" rx="1.2" fill="url(#favPylonGrad)"/>
    <rect x="2.5" y="5" width="17" height="32" rx="4" fill="none" stroke="url(#favPylonGrad)" stroke-width="2.8"/>
    <path d="M12 11 L7.5 21 L12 21 L10 29 L16 19 L11.5 19 Z" fill="url(#favPylonGrad)"/>
  </g>
</svg>)rawliteral";

// Shared Cascading Style Sheets (Zero dynamic RAM allocated, served directly from Flash ROM)
static const char COMMON_CSS[] PROGMEM = 
R"rawliteral(html{box-sizing:border-box;overflow-y:scroll;}
*, *::before, *::after{box-sizing:inherit;}
:root{--navy:#171c61;--green:#77b243;--teal:#00b3ba;--bg:#f4f6fa;--card:#ffffff;--text:#2d3748;--border:#e2e8f0;}
html.dark{--navy:#38bdf8;--green:#4ade80;--teal:#2dd4bf;--bg:#0b1120;--card:#151e32;--text:#cbd5e1;--border:#22324d;}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;background:var(--bg);color:var(--text);margin:0;padding:0;}
.header-wrap{position:sticky;top:0;z-index:1000;width:100%;}
.top-accent{height:4px;background:#e2e8f0;position:relative;overflow:hidden;width:100%;}
html.dark .top-accent{background:#1e293b;}
.top-accent-bar{height:100%;width:0%;background:linear-gradient(90deg,var(--green),var(--teal));transition:width 0.25s ease-out;box-shadow:0 0 8px rgba(0,179,186,0.6);}
.navbar{position:relative;background:var(--card);border-bottom:1px solid var(--border);box-shadow:0 2px 8px rgba(23,28,97,0.04);padding:10px 0;width:100%;}
html.dark .navbar{box-shadow:0 2px 10px rgba(0,0,0,0.4);}
.nav-inner{max-width:1150px;margin:0 auto;padding:0 20px;width:100%;display:flex;align-items:center;justify-content:space-between;gap:16px;}
.brand{display:flex;align-items:center;gap:12px;text-decoration:none;flex-shrink:0;}
html.dark .brand-pylon{fill:#f8fafc!important;}
html.dark .brand-sub{fill:#2dd4bf!important;}
.nav-links{display:flex;gap:8px;align-items:center;justify-content:center;flex-wrap:wrap;}
.nav-link{padding:6px 14px;border-radius:6px;font-size:0.88rem;font-weight:600;text-decoration:none;color:#1e293b;border:1.5px solid #94a3b8;background:#f1f5f9;box-shadow:0 1px 2px rgba(0,0,0,0.04);transition:all 0.18s;display:inline-flex;align-items:center;gap:6px;}
.nav-link:hover{color:#ffffff;border-color:#007378;background:#008b91;box-shadow:0 2px 6px rgba(0,139,145,0.25);transform:translateY(-1px);}
.nav-link.active{background:var(--navy);color:#ffffff;border:1.5px solid #0f1240;font-weight:700;box-shadow:0 2px 5px rgba(23,28,97,0.25);}
.nav-link.active:hover{background:#23297a;color:#ffffff;}
html.dark .nav-link{color:#e2e8f0;background:#1e293b;border-color:#334155;}
html.dark .nav-link:hover{background:#0d9488;border-color:#14b8a6;color:#ffffff;}
html.dark .nav-link.active{background:#0284c7;border-color:#38bdf8;color:#ffffff;}
html.dark .nav-link.active:hover{background:#0369a1;}
.nav-right{display:flex;align-items:center;flex-shrink:0;}
.nav-actions{display:flex;align-items:center;}
.btn{padding:7px 14px;border-radius:6px;font-size:0.86rem;font-weight:600;text-decoration:none;display:inline-flex;align-items:center;gap:6px;transition:all 0.18s;cursor:pointer;}
.btn-primary{background:linear-gradient(135deg,var(--green),var(--teal));color:#ffffff;border:1.5px solid #009aa0;box-shadow:0 2px 6px rgba(0,179,186,0.25);}
.btn-primary:hover{opacity:0.92;transform:translateY(-1px);}
html.dark .btn-primary{background:linear-gradient(135deg,#166534,#115e59);color:#ffffff;border:1.5px solid #14b8a6;box-shadow:0 2px 6px rgba(0,0,0,0.3);}
html.dark .btn-primary:hover{background:linear-gradient(135deg,#15803d,#0f766e);border-color:#2dd4bf;}
.btn-navy{background:var(--navy);color:#ffffff;border:1.5px solid #0f1240;box-shadow:0 2px 5px rgba(23,28,97,0.2);}
.btn-navy:hover{background:#23297a;transform:translateY(-1px);}
html.dark .btn-navy{background:#0369a1;border-color:#38bdf8;color:#ffffff;}
html.dark .btn-navy:hover{background:#0284c7;}
.btn-outline{background:#f1f5f9;color:#1e293b;border:1.5px solid #94a3b8;box-shadow:0 1px 2px rgba(0,0,0,0.04);}
.btn-outline:hover{background:#008b91;border-color:#007378;color:#ffffff;box-shadow:0 2px 6px rgba(0,139,145,0.25);transform:translateY(-1px);}
html.dark .btn-outline{background:#1e293b;color:#e2e8f0;border-color:#334155;}
html.dark .btn-outline:hover{background:#0d9488;border-color:#14b8a6;color:#ffffff;}
.btn-resume{background:#fef2f2;color:#b91c1c;border:1.5px solid #f87171;font-weight:700;box-shadow:0 1px 3px rgba(239,68,68,0.12);}
.btn-resume:hover{background:#dc2626;border-color:#b91c1c;color:#ffffff;transform:translateY(-1px);}
html.dark .btn-resume{background:#450a0a;color:#fca5a5;border-color:#991b1b;}
html.dark .btn-resume:hover{background:#dc2626;color:#ffffff;}
.switch-wrap{display:inline-flex;align-items:center;gap:9px;cursor:pointer;user-select:none;padding:3px 10px;border-radius:20px;background:#f8fafc;border:1px solid #cbd5e1;transition:all 0.15s;}
.switch-wrap:hover{background:#f1f5f9;border-color:#94a3b8;}
html.dark .switch-wrap{background:#1e293b;border-color:#334155;}
html.dark .switch-wrap:hover{background:#283548;border-color:#475569;}
.switch{position:relative;display:inline-block;width:38px;height:22px;}
.switch input{opacity:0;width:0;height:0;position:absolute;}
.switch-slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#e2e8f0;transition:.2s;border-radius:22px;border:1.5px solid #cbd5e1;}
.switch-slider:before{position:absolute;content:'';height:14px;width:14px;left:2px;bottom:2px;background-color:#94a3b8;transition:.2s;border-radius:50%;box-shadow:0 1px 2px rgba(0,0,0,0.15);}
.switch:hover .switch-slider{border-color:#008b91;}
.switch input:checked + .switch-slider{background:linear-gradient(135deg,var(--green),var(--teal));border-color:#009aa0;}
.switch input:checked + .switch-slider:before{background-color:#ffffff;transform:translateX(16px);box-shadow:0 1px 3px rgba(0,0,0,0.25);}
html.dark .switch-slider{background-color:#334155;border-color:#475569;}
html.dark .switch-slider:before{background-color:#94a3b8;}
.theme-switch{display:inline-flex;align-items:center;background:#f1f5f9;border:1.5px solid #cbd5e1;border-radius:20px;padding:2px;gap:2px;}
.theme-btn{background:transparent;border:none;border-radius:16px;padding:4px 7px;display:inline-flex;align-items:center;justify-content:center;color:#64748b;cursor:pointer;transition:all 0.15s;}
.theme-btn:hover{color:#0f172a;}
.theme-btn.active{background:#ffffff;color:#0f172a;box-shadow:0 1px 2px rgba(0,0,0,0.15);}
html.dark .theme-switch{background:#1e293b;border-color:#334155;}
html.dark .theme-btn{color:#94a3b8;}
html.dark .theme-btn:hover{color:#ffffff;}
html.dark .theme-btn.active{background:#334155;color:#f8fafc;box-shadow:0 1px 2px rgba(0,0,0,0.3);}
.container{max-width:1150px;margin:20px auto;padding:0 20px;width:100%;}
.ap-banner{background:#fffbeb;border:1px solid #fef3c7;border-radius:8px;padding:10px 16px;margin-bottom:18px;display:flex;align-items:center;justify-content:space-between;gap:12px;font-size:0.86rem;color:#92400e;}
.ap-banner a{color:#b45309;font-weight:700;text-decoration:underline;}
html.dark .ap-banner{background:#451a03;border-color:#78350f;color:#fde68a;}
html.dark .ap-banner a{color:#fbbf24;}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:16px;margin-bottom:22px;}
.grid-dash{display:grid;grid-template-columns:repeat(6,1fr);gap:12px;margin-bottom:22px;}
@media(max-width:1100px){.grid-dash{grid-template-columns:repeat(3,1fr);}}
@media(max-width:640px){.grid-dash{grid-template-columns:repeat(2,1fr);gap:10px;}}
@media(max-width:400px){.grid-dash{grid-template-columns:1fr;}}
.grid-dash .card{padding:14px 14px 12px 14px;display:flex;flex-direction:column;justify-content:flex-start;min-height:96px;}
.grid-dash .card h3{margin:0 0 6px 0;font-size:0.78rem;color:#4a5568;text-transform:uppercase;letter-spacing:0.04em;font-weight:700;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}
.grid-dash .card .val{font-size:1.65rem;font-weight:700;color:var(--navy);display:flex;align-items:baseline;line-height:1.15;}
.grid-dash .card .sub{font-size:0.75rem;color:#718096;margin-top:auto;padding-top:4px;font-weight:500;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}
.grid-dash .card .unit{font-size:0.88rem;color:#718096;margin-left:4px;font-weight:500;}
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:16px 18px;box-shadow:0 4px 12px rgba(23,28,97,0.04);position:relative;overflow:hidden;}
html.dark .card{box-shadow:0 4px 14px rgba(0,0,0,0.3);}
.card::before{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:var(--teal);}
.card.card-teal::before{background:var(--teal);}
.card.card-green::before{background:var(--green);}
.card.card-navy::before{background:var(--navy);}
.card.card-amber::before{background:#d97706;}
.card.card-amber .val{color:#c2410c;}
html.dark .card.card-amber .val{color:#fb923c;}
.card.card-blue::before{background:#2563eb;}
.card.card-blue .val{color:#1d4ed8;}
html.dark .card.card-blue .val{color:#60a5fa;}
.card.card-power::before{background:#00b3ba;}
.card.card-red::before{background:#dc2626;}
.card h3{margin:0 0 8px 0;font-size:0.92rem;color:#4a5568;text-transform:uppercase;letter-spacing:0.05em;font-weight:700;}
.card .val{font-size:1.80rem;font-weight:700;color:var(--navy);display:flex;align-items:baseline;}
.card .sub{font-size:0.80rem;color:#718096;margin-top:4px;font-weight:500;}
.card .unit{font-size:0.95rem;color:#718096;margin-left:5px;font-weight:500;}
html.dark .card h3, html.dark .grid-dash .card h3{color:#94a3b8;}
html.dark .card .sub, html.dark .grid-dash .card .sub, html.dark .grid-dash .card .unit{color:#94a3b8;}
.sys-card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:16px 20px;box-shadow:0 4px 12px rgba(23,28,97,0.04);margin-bottom:20px;}
html.dark .sys-card{box-shadow:0 4px 14px rgba(0,0,0,0.3);}
.sys-hdr{color:var(--navy);font-weight:700;font-size:0.92rem;letter-spacing:0.06em;margin-bottom:12px;border-bottom:2px solid var(--teal);padding-bottom:6px;display:flex;align-items:center;gap:8px;}
.sys-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px 32px;font-size:0.86rem;}
.sys-row{display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px dashed #f1f5f9;gap:10px;white-space:nowrap;}
html.dark .sys-row{border-bottom-color:#1e293b;}
.sys-label{color:#64748b;font-weight:600;white-space:nowrap;flex-shrink:0;}
html.dark .sys-label{color:#94a3b8;}
.sys-val{color:var(--navy);font-weight:700;font-family:monospace;font-size:0.88rem;white-space:nowrap;text-align:right;}
.table-card{background:var(--card);border:1px solid var(--border);border-radius:10px;box-shadow:0 4px 12px rgba(23,28,97,0.04);overflow-x:auto;width:100%;box-sizing:border-box;}
html.dark .table-card{box-shadow:0 4px 14px rgba(0,0,0,0.3);}
.table-header{padding:14px 18px;border-bottom:1px solid var(--border);background:#fafcff;display:flex;justify-content:space-between;align-items:center;}
html.dark .table-header{background:#182238;}
.table-header h3{margin:0;color:var(--navy);font-size:1.08rem;font-weight:700;}
table{width:100%;min-width:100%;border-collapse:collapse;table-layout:auto;}
th{background:var(--navy);color:#ffffff;font-size:0.82rem;text-transform:uppercase;letter-spacing:0.05em;padding:11px 12px;text-align:left;font-weight:700;}
html.dark th{background:#1e293b;color:#f8fafc;}
td{padding:11px 12px;border-bottom:1px solid #edf2f7;font-size:0.90rem;color:#2d3748;}
html.dark td{border-bottom:1px solid #1e293b;color:#cbd5e1;}
tr:nth-child(even){background:#fafcff;}
tr:hover{background:#f1f7f9;}
html.dark tr:nth-child(even){background:#131b2e;}
html.dark tr:hover{background:#1b2640;}
.mod-link{font-weight:700;color:#1e293b;text-decoration:none;padding:2px 8px;border-radius:4px;background:#f1f5f9;border:1px solid #cbd5e1;transition:all 0.15s;}
.mod-link:hover{background:#008b91;color:#ffffff;border-color:#007378;}
html.dark .mod-link{color:#e2e8f0;background:#1e293b;border-color:#334155;}
html.dark .mod-link:hover{background:#0d9488;color:#ffffff;border-color:#14b8a6;}
.mod-dev-link{color:var(--navy);text-decoration:none;font-weight:700;transition:color 0.15s;}
.mod-dev-link:hover{color:var(--teal);text-decoration:underline;}
.badge{padding:3px 8px;border-radius:12px;font-size:0.74rem;font-weight:600;display:inline-block;}
.badge-ok{background:#eaf6ea;color:#2d7a2d;border:1px solid #c3e6c3;}
.badge-charge{background:#e6f9fa;color:#00838f;border:1px solid #b2ebf2;}
.badge-warn{background:#fff8e6;color:#b7791f;border:1px solid #fbd38d;}
.badge-danger{background:#fef2f2;color:#dc2626;border:1px solid #fca5a5;}
html.dark .badge-ok{background:#14532d;color:#86efac;border:1px solid #166534;}
html.dark .badge-charge{background:#134e4a;color:#5eead4;border:1px solid #115e59;}
html.dark .badge-warn{background:#78350f;color:#fde68a;border:1px solid #92400e;}
html.dark .badge-danger{background:#7f1d1d;color:#fca5a5;border:1px solid #991b1b;}
.term-dark{background:#0f172a;border:1px solid #1e293b;color:#e2e8f0;}
.term-dark .line-tx{color:#38bdf8;font-weight:700;}
.term-dark .line-err{color:#f87171;font-weight:700;}
.term-dark .line-sys{color:#facc15;}
.term-dark .line-info{color:#a3e635;}
.term-dark .ts{color:#64748b;}
.term-light{background:#ffffff;border:1px solid #cbd5e1;color:#1e293b;}
.term-light .line-tx{color:#0284c7;font-weight:700;}
.term-light .line-err{color:#dc2626;font-weight:700;}
.term-light .line-sys{color:#d97706;}
.term-light .line-info{color:#15803d;}
.term-light .ts{color:#94a3b8;}
.chart-btn-group{display:flex;background:#edf2f7;border-radius:6px;padding:2px;gap:2px;}
html.dark .chart-btn-group{background:#1e293b;}
.chart-btn{padding:4px 10px;font-size:0.80rem;font-weight:600;border:none;background:transparent;color:#475569;border-radius:4px;cursor:pointer;transition:all 0.15s;}
.chart-btn:hover{background:#e2e8f0;color:#1e293b;}
.chart-btn.active{background:var(--navy);color:#ffffff;box-shadow:0 1px 3px rgba(0,0,0,0.15);}
html.dark .chart-btn{color:#94a3b8;}
html.dark .chart-btn:hover{background:#334155;color:#f8fafc;}
html.dark .chart-btn.active{background:#0284c7;color:#ffffff;}
.chart-leg-soc{display:inline-flex;align-items:center;gap:5px;cursor:pointer;color:#008b91;}
.chart-leg-volt{display:inline-flex;align-items:center;gap:5px;cursor:pointer;color:#5a8e2e;}
.chart-leg-curr{display:inline-flex;align-items:center;gap:5px;cursor:pointer;color:#dc2626;}
html.dark .chart-leg-soc{color:#2dd4bf;}
html.dark .chart-leg-volt{color:#4ade80;}
html.dark .chart-leg-curr{color:#f87171;}
.cell-legend{display:flex;align-items:center;gap:10px;font-size:0.75rem;background:#f8fafc;padding:5px 12px;border-radius:6px;border:1px solid #e2e8f0;}
html.dark .cell-legend{background:#1e293b;border-color:#334155;}
.bat-shell{position:relative;width:24px;height:12px;border:1.2px solid #64748b;border-radius:2.5px;padding:1px;background:#f8fafc;display:inline-flex;align-items:center;flex-shrink:0;}
.bat-shell::after{content:'';position:absolute;right:-3px;top:2.5px;width:1.8px;height:4.5px;background:#64748b;border-radius:0 1px 1px 0;}
html.dark .bat-shell{background:#1e293b;border-color:#475569;}
html.dark .bat-shell::after{background:#475569;}
.bat-fill{height:100%;border-radius:1px;}
html.dark input[type='text'],html.dark input[type='password'],html.dark input[type='number'],html.dark select{background-color:#1e293b!important;color:#f1f5f9!important;border-color:#334155!important;}
.footer{margin:35px 0 20px 0;text-align:center;font-size:0.80rem;color:#64748b;}
html.dark .footer{color:#94a3b8;}
@media(min-width:1250px){.nav-actions{position:absolute;right:24px;top:50%;transform:translateY(-50%);}}
@media(max-width:1249px){.nav-inner{flex-wrap:wrap;justify-content:center;gap:12px;}.nav-actions{display:flex;align-items:center;justify-content:center;margin-top:4px;}}
@media(max-width:960px){.header-wrap{position:static;}.navbar{padding:12px 0;}.nav-inner{flex-direction:column;align-items:center;gap:12px;}.sys-grid{grid-template-columns:1fr;gap:14px;}})rawliteral";

// Pure HTML5 Canvas 24-Hour Telemetry History Chart (Zero external libraries)
static const char DASHBOARD_CHART_JS[] PROGMEM = 
R"rawliteral(<script>
(function(){
  var currentRange = 86400;
  var is24h = (typeof CONFIG_TIME_24H !== 'undefined') ? CONFIG_TIME_24H : true;
  var chartData = [];
  var hoverIdx = -1;
  var canvas = document.getElementById('telemetryCanvas');
  var tooltip = document.getElementById('chartTooltip');
  if(!canvas) return;
  var ctx = canvas.getContext('2d');

  window.setChartRange = function(sec, btn) {
    currentRange = sec;
    var btns = document.querySelectorAll('.chart-btn');
    btns.forEach(function(b){ b.classList.remove('active'); });
    if(btn) btn.classList.add('active');
    fetchData();
  };

  function formatTime(t, full) {
    if (t > 1577836800) {
      var d = new Date(t * 1000);
      var opt = { hour12: !is24h, hour: '2-digit', minute: '2-digit' };
      if (full) opt.second = '2-digit';
      return d.toLocaleTimeString([], opt);
    }
    var h = Math.floor(t / 3600), m = Math.floor((t % 3600) / 60), s = t % 60;
    return full ? ('+' + h + 'h ' + m + 'm ' + s + 's') : ('+' + h + 'h ' + m + 'm');
  }

  function formatNow() {
    var opt = { hour12: !is24h, hour: '2-digit', minute: '2-digit', second: '2-digit' };
    return new Date().toLocaleTimeString([], opt);
  }

  function fetchData() {
    var sub = document.getElementById('chartSub');
    if(sub) sub.innerText = 'Refreshing data...';
    fetch('/api/history?range=' + currentRange)
      .then(function(r){ return r.json(); })
      .then(function(d){
        chartData = (d && d.samples) ? d.samples : [];
        if(sub) {
          sub.innerText = chartData.length + ' samples (' + (currentRange/3600) + 'h window) • Updated: ' + formatNow();
        }
        drawChart();
      })
      .catch(function(err){
        if(sub) sub.innerText = 'Failed to load telemetry history';
      });
  }

  window.drawChart = function() {
    var dpr = window.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    if(rect.width === 0 || rect.height === 0) return;
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    if(ctx.resetTransform) ctx.resetTransform();
    else ctx.setTransform(1,0,0,1,0,0);
    ctx.scale(dpr, dpr);
    var w = rect.width, h = rect.height;

    ctx.clearRect(0, 0, w, h);

    if(!chartData || chartData.length === 0) {
      ctx.fillStyle = '#64748b';
      ctx.font = '600 13px system-ui, sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText('Collecting 24-hour telemetry data... (Updates every minute)', w / 2, h / 2);
      return;
    }

    var showSoc = document.getElementById('chkSoc') ? document.getElementById('chkSoc').checked : true;
    var showCurr = document.getElementById('chkCurr') ? document.getElementById('chkCurr').checked : true;

    var padL = 48, padR = 48, padT = 20, padB = 26;
    var plotW = w - padL - padR;
    var plotH = h - padT - padB;

    var tMin = chartData[0].t;
    var tMax = chartData[chartData.length - 1].t;
    if(tMax - tMin < 60) tMax = tMin + 60;

    var cMin = 0, cMax = 0;
    var sMin = 0, sMax = 100;

    chartData.forEach(function(s){
      if(s.c < cMin) cMin = s.c;
      if(s.c > cMax) cMax = s.c;
    });

    cMin = Math.min(-1, Math.floor(cMin - 1));
    cMax = Math.max(1, Math.ceil(cMax + 1));

    function xPos(t) { return padL + ((t - tMin) / (tMax - tMin)) * plotW; }
    function yCurr(c) { return padT + plotH - ((c - cMin) / (cMax - cMin)) * plotH; }
    function ySoc(s)  { return padT + plotH - ((s - sMin) / (sMax - sMin)) * plotH; }

    // Grid lines & Y Ticks (5 intervals)
    ctx.lineWidth = 1;
    ctx.font = '10px monospace';

    var isDark = document.documentElement.classList.contains('dark');
    for(var i = 0; i <= 4; i++) {
      var y = padT + (plotH * i) / 4;
      ctx.strokeStyle = isDark ? '#1e293b' : '#f1f5f9';
      ctx.beginPath();
      ctx.moveTo(padL, y);
      ctx.lineTo(padL + plotW, y);
      ctx.stroke();

      if(showSoc) {
        var sVal = Math.round(sMax - (i * (sMax - sMin)) / 4);
        ctx.fillStyle = isDark ? '#2dd4bf' : '#008b91';
        ctx.textAlign = 'right';
        ctx.fillText(sVal + '%', padL - 6, y + 3);
      }

      if(showCurr) {
        var cVal = (cMax - (i * (cMax - cMin)) / 4).toFixed(0);
        ctx.fillStyle = isDark ? '#f87171' : '#dc2626';
        ctx.textAlign = 'left';
        ctx.fillText((cVal > 0 ? '+' : '') + cVal + 'A', padL + plotW + 6, y + 3);
      }
    }

    // Zero current line (if Current shown and crosses 0)
    if(showCurr && cMin < 0 && cMax > 0) {
      var y0 = yCurr(0);
      ctx.setLineDash([4, 4]);
      ctx.strokeStyle = isDark ? '#334155' : '#cbd5e1';
      ctx.beginPath();
      ctx.moveTo(padL, y0);
      ctx.lineTo(padL + plotW, y0);
      ctx.stroke();
      ctx.setLineDash([]);
      ctx.fillStyle = isDark ? '#94a3b8' : '#64748b';
      ctx.textAlign = 'left';
      ctx.fillText('0A', padL + plotW + 6, y0 + 3);
    }

    // X-Axis Ticks (Time)
    ctx.textAlign = 'center';
    ctx.fillStyle = isDark ? '#94a3b8' : '#64748b';
    var xCount = w > 600 ? 6 : 3;
    for(var k = 0; k <= xCount; k++) {
      var tVal = tMin + (k * (tMax - tMin)) / xCount;
      var x = xPos(tVal);
      ctx.fillText(formatTime(tVal, false), x, h - 8);
    }

    // Draw Series Lines (supports area fill and dashed lines)
    function drawSeries(getY, strokeStyle, fillGrad, isDashed) {
      if(chartData.length === 0) return;

      // 1. Fill area underneath first
      if(fillGrad) {
        ctx.save();
        ctx.beginPath();
        chartData.forEach(function(p, idx){
          var x = xPos(p.t);
          var y = getY(p);
          if(idx === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        });
        ctx.lineTo(xPos(chartData[chartData.length - 1].t), padT + plotH);
        ctx.lineTo(xPos(chartData[0].t), padT + plotH);
        ctx.closePath();
        ctx.fillStyle = fillGrad;
        ctx.fill();
        ctx.restore();
      }

      // 2. Stroke line on top
      ctx.save();
      if(isDashed) {
        ctx.setLineDash([5, 4]);
      } else {
        ctx.setLineDash([]);
      }
      ctx.beginPath();
      chartData.forEach(function(p, idx){
        var x = xPos(p.t);
        var y = getY(p);
        if(idx === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.strokeStyle = strokeStyle;
      ctx.lineWidth = isDashed ? 2 : 2.5;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.stroke();
      ctx.restore();
    }

    if(showSoc) {
      var grad = ctx.createLinearGradient(0, padT, 0, padT + plotH);
      grad.addColorStop(0, isDark ? 'rgba(45, 212, 191, 0.35)' : 'rgba(0, 179, 186, 0.35)');
      grad.addColorStop(0.7, isDark ? 'rgba(45, 212, 191, 0.12)' : 'rgba(0, 179, 186, 0.12)');
      grad.addColorStop(1, 'rgba(0, 179, 186, 0.01)');
      drawSeries(function(p){ return ySoc(p.s); }, isDark ? '#2dd4bf' : '#00b3ba', grad, false);
    }

    if(showCurr) {
      drawSeries(function(p){ return yCurr(p.c); }, isDark ? '#f87171' : '#dc2626', null, true);
    }

    // Hover Crosshair & Indicators
    if(hoverIdx >= 0 && hoverIdx < chartData.length) {
      var hp = chartData[hoverIdx];
      var hx = xPos(hp.t);

      ctx.setLineDash([3, 3]);
      ctx.strokeStyle = isDark ? '#94a3b8' : '#64748b';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(hx, padT);
      ctx.lineTo(hx, padT + plotH);
      ctx.stroke();
      ctx.setLineDash([]);

      function drawDot(y, color) {
        ctx.fillStyle = isDark ? '#151e32' : '#ffffff';
        ctx.strokeStyle = color;
        ctx.lineWidth = 2.5;
        ctx.beginPath();
        ctx.arc(hx, y, 4, 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();
      }

      if(showSoc) drawDot(ySoc(hp.s), isDark ? '#2dd4bf' : '#00b3ba');
      if(showCurr) drawDot(yCurr(hp.c), isDark ? '#f87171' : '#dc2626');
    }
  };

  function updateHover(clientX) {
    if(!chartData || chartData.length === 0) return;
    var rect = canvas.getBoundingClientRect();
    var mouseX = clientX - rect.left;
    var padL = 48, padR = 48;
    var plotW = rect.width - padL - padR;
    if(mouseX < padL || mouseX > padL + plotW) {
      hoverIdx = -1;
      tooltip.style.display = 'none';
      drawChart();
      return;
    }

    var tMin = chartData[0].t;
    var tMax = chartData[chartData.length - 1].t;
    if(tMax - tMin < 60) tMax = tMin + 60;
    var targetT = tMin + ((mouseX - padL) / plotW) * (tMax - tMin);

    var bestIdx = 0, bestDiff = Math.abs(chartData[0].t - targetT);
    for(var i = 1; i < chartData.length; i++) {
      var diff = Math.abs(chartData[i].t - targetT);
      if(diff < bestDiff) {
        bestDiff = diff;
        bestIdx = i;
      }
    }
    hoverIdx = bestIdx;
    drawChart();

    var p = chartData[hoverIdx];
    var timeStr = formatTime(p.t, true);
    var currStr = (p.c >= 0 ? '+' : '') + p.c.toFixed(2) + ' A';

    tooltip.innerHTML = 
      '<div style="font-weight:700;color:#94a3b8;margin-bottom:4px;border-bottom:1px solid rgba(255,255,255,0.15);padding-bottom:2px;">' + timeStr + '</div>' +
      (showSoc ? '<div style="color:#2dd4bf;">● SOC: <b>' + p.s + '%</b></div>' : '') +
      (showCurr ? '<div style="color:#f87171;">- - Curr: <b>' + currStr + '</b></div>' : '');

    var tipX = mouseX + 12;
    if(tipX + 140 > rect.width) tipX = mouseX - 150;
    tooltip.style.left = tipX + 'px';
    tooltip.style.top = '25px';
    tooltip.style.display = 'block';
  }

  canvas.addEventListener('mousemove', function(e){ updateHover(e.clientX); });
  canvas.addEventListener('mouseleave', function(){
    hoverIdx = -1;
    tooltip.style.display = 'none';
    drawChart();
  });
  canvas.addEventListener('touchmove', function(e){
    if(e.touches.length > 0) updateHover(e.touches[0].clientX);
  });
  canvas.addEventListener('touchend', function(){
    hoverIdx = -1;
    tooltip.style.display = 'none';
    drawChart();
  });

  window.addEventListener('resize', drawChart);
  fetchData();
  setInterval(fetchData, 60000);
})();
</script>)rawliteral";

class ChunkedResponseSender {
private:
    WebServer &server;
    String buf;
    size_t flushThreshold;
    bool finished;

public:
    ChunkedResponseSender(WebServer &srv, size_t reserveSize = 1500) 
        : server(srv), flushThreshold(reserveSize > 300 ? reserveSize - 200 : 800), finished(false) {
        buf.reserve(reserveSize);
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");
    }

    ChunkedResponseSender(WebServer &srv, const char *contentType, size_t reserveSize = 1024) 
        : server(srv), flushThreshold(reserveSize > 300 ? reserveSize - 200 : 800), finished(false) {
        buf.reserve(reserveSize);
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, contentType, "");
    }

    ~ChunkedResponseSender() {
        if (!finished) {
            finish();
        }
    }

    template <typename T>
    ChunkedResponseSender& operator+=(const T &val) {
        buf += val;
        if (buf.length() >= flushThreshold) {
            flush();
        }
        return *this;
    }

    void flush() {
        if (buf.length() > 0) {
            server.sendContent(buf);
            buf = "";
            yield();
        }
    }

    void finish() {
        if (!finished) {
            flush();
            server.sendContent("");
            finished = true;
        }
    }
};

using ChunkedHtmlSender = ChunkedResponseSender;

class WebPortal {
private:
    WebServer &server;
    BatteryStack &stack;
    Preferences &prefs;
    bool &isApMode;
    bool &triggerManualPoll;
    bool &isPollingPaused;
    PylonSerialManager &pylonSerial;
    MqttClientManager &mqttClient;
    BatteryHistoryManager &history;
    PeerDiscoveryManager &peerDiscovery;

public:
    WebPortal(WebServer &srv, BatteryStack &stk, Preferences &p, bool &apMode, bool &manPoll, bool &isPaused, PylonSerialManager &serial, MqttClientManager &mqtt, BatteryHistoryManager &hist, PeerDiscoveryManager &peer)
        : server(srv), stack(stk), prefs(p), isApMode(apMode), triggerManualPoll(manPoll), isPollingPaused(isPaused), pylonSerial(serial), mqttClient(mqtt), history(hist), peerDiscovery(peer) {}

    void setupRoutes() {
        // Dashboard is accessible in both STA and Standalone AP mode!
        server.on("/", [this]() {
            if (!checkWebAuth()) return;
            handleDashboard();
        });

        // Module Detail Page (Rack visualization + telemetry)
        server.on("/module", [this]() {
            if (!checkWebAuth()) return;
            handleModuleDetail();
        });

        // On-demand refresh for specific module
        server.on("/refresh_module", [this]() {
            if (!checkWebAuth()) return;
            handleRefreshModule();
        });

        // Settings Page
        server.on("/settings", [this]() {
            if (!checkWebAuth()) return;
            handleSettingsPage();
        });

        server.on("/save_settings", HTTP_POST, [this]() {
            if (!checkWebAuth()) return;
            handleSaveSettings();
        });

        // Prometheus Metrics endpoint
        server.on("/metrics", [this]() {
            PrometheusExporter::generateMetrics(stack, server);
        });

        // REST JSON API
        server.on("/api/data", [this]() {
            if (!checkApiAuth()) return;
            handleApiData();
        });

        // Live Module Telemetry API (AJAX)
        server.on("/api/module", [this]() {
            if (!checkWebAuth()) return;
            if (!checkApiAuth()) return;
            handleApiModuleData();
        });

        // 24-Hour Telemetry History API
        server.on("/api/history", [this]() {
            if (!checkApiAuth()) return;
            uint32_t range = 86400;
            if (server.hasArg("range")) {
                range = server.arg("range").toInt();
                if (range < 300) range = 300;
                if (range > 86400) range = 86400;
            }
            history.streamJson(server, range);
        });

        // Network Peer Discovery API
        server.on("/api/peers", [this]() {
            if (!checkApiAuth()) return;
            handleApiPeers();
        });

        // Web Console Log
        server.on("/log", [this]() {
            if (!checkWebAuth()) return;
            handleLogPage();
        });

        server.on("/log/clear", [this]() {
            if (!checkWebAuth()) return;
            consoleLog.clear();
            server.sendHeader("Location", "/log");
            server.send(303);
        });

        server.on("/log/raw", [this]() {
            if (!checkWebAuth()) return;
            consoleLog.streamLog(server);
        });

        server.on("/save", HTTP_POST, [this]() {
            handleSaveWifi();
        });

        server.on("/wifi", [this]() {
            if (!checkWebAuth()) return;
            handleApPortal();
        });

        server.on("/reset_wifi", [this]() {
            if (!checkWebAuth()) return;
            prefs.remove("ssid");
            prefs.remove("pass");
            server.send(200, "text/html", "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;padding:50px;'><h3 style='color:#171c61;'>WiFi credentials erased.</h3><p>Restarting in AP mode...</p></body></html>");
            delay(1000);
            ESP.restart();
        });

        server.on("/restart", [this]() {
            if (!checkWebAuth()) return;
            server.send(200, "text/html", "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;padding:50px;'><h3 style='color:#171c61;'>Restarting device...</h3><p>Please wait 10 seconds and <a href='/'>click here</a>.</p><script>setTimeout(function(){window.location.href='/';},10000);</script></body></html>");
            delay(1000);
            ESP.restart();
        });

        server.on("/poll_now", [this]() {
            if (!checkWebAuth()) return;
            triggerManualPoll = true;
            String redirectUrl = "/";
            if (server.hasArg("redirect")) {
                redirectUrl = server.arg("redirect");
            }
            server.sendHeader("Location", redirectUrl);
            server.send(303);
        });

        server.on("/toggle_pause", [this]() {
            if (!checkWebAuth()) return;
            isPollingPaused = !isPollingPaused;
            if (isPollingPaused) {
                consoleLog.logInfo("[SYSTEM] Polling paused by user");
            } else {
                consoleLog.logInfo("[SYSTEM] Polling resumed by user");
            }
            String redirectUrl = "/";
            if (server.hasArg("redirect")) {
                redirectUrl = server.arg("redirect");
            }
            server.sendHeader("Location", redirectUrl);
            server.send(303);
        });

        server.on("/sync_ntp", [this]() {
            if (!checkWebAuth()) return;
            triggerNtpSync();
            consoleLog.logInfo("Manual NTP sync triggered by user.");
            String redirectUrl = "/settings";
            if (server.hasArg("redirect")) {
                redirectUrl = server.arg("redirect");
            }
            server.sendHeader("Location", redirectUrl);
            server.send(303);
        });

        server.on("/cmd", [this]() {
            if (!checkWebAuth()) return;
            if (server.hasArg("c")) {
                String cmd = server.arg("c");
                cmd.trim();
                if (cmd.length() > 0 && cmd.length() <= 32) {
                    pylonSerial.enqueueUserCommand(cmd);
                }
            }
            if (server.hasArg("ajax")) {
                server.send(200, "text/plain", "QUEUED");
            } else {
                String redirectUrl = "/log";
                if (server.hasArg("redirect")) {
                    redirectUrl = server.arg("redirect");
                }
                server.sendHeader("Location", redirectUrl);
                server.send(303);
            }
        });

        // Favicons
        server.on("/favicon.ico", [this]() {
            server.send_P(200, "image/svg+xml", PYLON_FAVICON_SVG);
        });

        // Shared Stylesheet (served directly from Flash ROM with 7-day browser caching)
        server.on("/style.css", [this]() {
            server.sendHeader("Cache-Control", "public, max-age=604800");
            server.send_P(200, "text/css; charset=utf-8", COMMON_CSS);
        });
        server.on("/favicon.svg", [this]() {
            server.send_P(200, "image/svg+xml", PYLON_FAVICON_SVG);
        });

        // Web Browser OTA
        server.on("/update", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleOtaPage();
        });

        server.on("/update", HTTP_POST, [this]() {
            if (!checkWebAuth()) return;
            server.sendHeader("Connection", "close");
            if (Update.hasError()) {
                server.send(500, "text/plain", "OTA Update Failed!");
            } else {
                server.send(200, "text/html", "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;padding:50px;'><h2 style='color:#77b243;'>Update Success!</h2><p>Device is rebooting, please wait 10 seconds...</p><script>setTimeout(function(){window.location.href='/';},10000);</script></body></html>");
                delay(1000);
                ESP.restart();
            }
        }, [this]() {
            handleOtaUpload();
        });

        // Captive portal redirects
        server.on("/generate_204", [this]() { handleCaptiveRedirect(); });
        server.on("/fwlink", [this]() { handleCaptiveRedirect(); });
        server.on("/hotspot-detect.html", [this]() { handleCaptiveRedirect(); });
        server.onNotFound([this]() {
            if (isApMode) {
                handleCaptiveRedirect();
            } else {
                server.send(404, "text/plain", "Not Found");
            }
        });
    }

private:
    // =========================================================================
    // Authentication Helpers
    // =========================================================================
    bool checkWebAuth() {
        bool authEnabled = prefs.getBool(NVS_KEY_AUTH_ENABLED, false);
        if (!authEnabled) return true;

        String user = prefs.getString(NVS_KEY_AUTH_USER, "admin");
        String pass = prefs.getString(NVS_KEY_AUTH_PASS, "admin");

        if (!server.authenticate(user.c_str(), pass.c_str())) {
            server.requestAuthentication(BASIC_AUTH, "Pylon Smart Monitor");
            return false;
        }
        return true;
    }

    bool checkApiAuth() {
        bool apiAuth = prefs.getBool(NVS_KEY_API_AUTH_ENABLED, false);
        if (!apiAuth) return true;

        String expectedToken = prefs.getString(NVS_KEY_API_TOKEN, "");
        if (expectedToken.length() == 0) return true;

        // Check Authorization header: Bearer <token>
        if (server.hasHeader("Authorization")) {
            String authHeader = server.header("Authorization");
            if (authHeader.startsWith("Bearer ") && authHeader.substring(7) == expectedToken) {
                return true;
            }
        }

        // Check query parameter ?token=<token>
        if (server.hasArg("token") && server.arg("token") == expectedToken) {
            return true;
        }

        server.send(401, "application/json", "{\"error\":\"Unauthorized\",\"message\":\"Invalid or missing Bearer token\"}");
        return false;
    }

    void handleCaptiveRedirect() {
        server.sendHeader("Location", String("http://") + server.client().localIP().toString() + "/", true);
        server.send(302, "text/plain", "");
    }

    // =========================================================================
    // Shared Layout Components (Header, Footer, CSS)
    // =========================================================================
    String renderHeader(const String &pageTitle, const String &activeNav) {
        String h;
        h.reserve(4096);
        String devHost = getDeviceHostname(prefs);
        h += "<!DOCTYPE html>\n<html lang='en'>\n<head>\n";
        h += "  <meta charset='utf-8'>\n";
        h += "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n";
        h += "  <title>" + pageTitle + " - " + devHost + "</title>\n";
        h += "  <link rel='icon' type='image/svg+xml' href='/favicon.svg'>\n";
        h += "  <link rel='icon' type='image/x-icon' href='/favicon.ico'>\n";
        h += "  <link rel='stylesheet' href='/style.css?v=" + String(FIRMWARE_VERSION) + "'>\n";
        h += "  <script>\n";
        h += "  (function(){\n";
        h += "    try {\n";
        h += "      var t = localStorage.getItem('pylon_theme') || 'system';\n";
        h += "      var d = false;\n";
        h += "      if (t === 'dark') d = true;\n";
        h += "      else if (t === 'light') d = false;\n";
        h += "      else {\n";
        h += "        var osDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;\n";
        h += "        var hr = new Date().getHours() + new Date().getMinutes() / 60;\n";
        h += "        d = osDark || (hr >= 19 || hr < 7);\n";
        h += "      }\n";
        h += "      if (d) document.documentElement.classList.add('dark');\n";
        h += "      else document.documentElement.classList.remove('dark');\n";
        h += "    } catch(e) {}\n";
        h += "  })();\n";
        h += "  </script>\n";
        h += "</head>\n<body>\n";
        h += "<div class='header-wrap'>\n";
        h += "  <div class='top-accent'><div id='topBar' class='top-accent-bar'></div></div>\n";
        h += "  <script>\n";
        h += "  (function(){\n";
        h += "    var b = document.getElementById('topBar');\n";
        h += "    if (b) {\n";
        h += "      b.style.width = '25%';\n";
        h += "      var p = 25, t = setInterval(function() { if (p < 85) { p += (85 - p) * 0.15; b.style.width = Math.round(p) + '%'; } else { clearInterval(t); } }, 120);\n";
        h += "      document.addEventListener('DOMContentLoaded', function() { clearInterval(t); if (b) b.style.width = '90%'; });\n";
        h += "      window.addEventListener('load', function() { clearInterval(t); if (b) b.style.width = '100%'; });\n";
        h += "      window.addEventListener('pageshow', function() { if (b) b.style.width = '100%'; });\n";
        h += "      document.addEventListener('click', function(e) {\n";
        h += "        var a = e.target.closest('a');\n";
        h += "        if (a && a.href && a.target !== '_blank' && !a.href.startsWith('javascript:') && !a.href.includes('#') && !a.hasAttribute('download')) {\n";
        h += "          if (a.origin === window.location.origin && b) {\n";
        h += "            b.style.transition = 'none'; b.style.width = '0%'; b.offsetWidth;\n";
        h += "            b.style.transition = 'width 0.4s cubic-bezier(0.1, 0.9, 0.2, 1)'; b.style.width = '75%';\n";
        h += "          }\n";
        h += "        }\n";
        h += "      });\n";
        h += "      document.addEventListener('submit', function() {\n";
        h += "        if (b) { b.style.transition = 'none'; b.style.width = '0%'; b.offsetWidth; b.style.transition = 'width 0.5s ease-out'; b.style.width = '75%'; }\n";
        h += "      });\n";
        h += "    }\n";
        h += "  })();\n";
        h += "  function isDarkTheme(m) {\n";
        h += "    if (m === 'dark') return true;\n";
        h += "    if (m === 'light') return false;\n";
        h += "    var osDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;\n";
        h += "    var hr = new Date().getHours() + new Date().getMinutes() / 60;\n";
        h += "    return osDark || (hr >= 19 || hr < 7);\n";
        h += "  }\n";
        h += "  function setTheme(m) { try { if (m === 'system') localStorage.removeItem('pylon_theme'); else localStorage.setItem('pylon_theme', m); } catch(e) {} applyThemeUI(); }\n";
        h += "  function applyThemeUI() {\n";
        h += "    var m = 'system'; try { m = localStorage.getItem('pylon_theme') || 'system'; } catch(e) {}\n";
        h += "    var d = isDarkTheme(m);\n";
        h += "    var wasDark = document.documentElement.classList.contains('dark');\n";
        h += "    if (d) document.documentElement.classList.add('dark'); else document.documentElement.classList.remove('dark');\n";
        h += "    var bl = document.getElementById('themeBtnLight'), bd = document.getElementById('themeBtnDark'), bs = document.getElementById('themeBtnSystem');\n";
        h += "    if (bl) bl.className = 'theme-btn' + (m === 'light' ? ' active' : '');\n";
        h += "    if (bd) bd.className = 'theme-btn' + (m === 'dark' ? ' active' : '');\n";
        h += "    if (bs) bs.className = 'theme-btn' + (m === 'system' ? ' active' : '');\n";
        h += "    if (wasDark !== d && typeof window.drawChart === 'function') window.drawChart();\n";
        h += "  }\n";
        h += "  if (window.matchMedia) { try { window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function() { if (!localStorage.getItem('pylon_theme')) applyThemeUI(); }); } catch(e) {} }\n";
        h += "  document.addEventListener('DOMContentLoaded', applyThemeUI);\n";
        h += "  document.addEventListener('visibilitychange', function() { if (!document.hidden) applyThemeUI(); });\n";
        h += "  setInterval(applyThemeUI, 30000);\n";
        h += "  </script>\n";
        h += "  <div class='navbar'>\n";
        h += "    <div class='nav-inner'>\n";
        h += "      <a href='/' class='brand'>" + String(FPSTR(PYLON_LOGO_SVG)) + "</a>\n";
        h += "      <div class='nav-links'>\n";
        h += "        <a class='nav-link" + String(activeNav == "dash" ? " active" : "") + "' href='/'>📊 Dashboard</a>\n";
        h += "        <a class='nav-link" + String(activeNav == "log" ? " active" : "") + "' href='/log'>📟 Console Log</a>\n";
        h += "        <a class='nav-link" + String(activeNav == "settings" ? " active" : "") + "' href='/settings'>⚙️ Settings</a>\n";
        h += "        <a class='nav-link' href='/metrics' target='_blank'>📈 Metrics</a>\n";
        h += "        <a class='nav-link' href='/api/data' target='_blank'>🔌 API</a>\n";
        h += "      </div>\n";
        h += "      <div class='nav-right'>\n";
        h += "        <label class='switch-wrap' title='Toggle BMS Polling (Click to " + String(isPollingPaused ? "resume" : "pause") + ")'>\n";
        if (isPollingPaused) {
            h += "          <span style='font-size:0.84rem;font-weight:700;color:#dc2626;'>⏸ Paused</span>\n";
        } else {
            h += "          <span style='font-size:0.84rem;font-weight:700;color:#16a34a;'>● Polling</span>\n";
        }
        h += "          <span class='switch'>\n";
        h += "            <input type='checkbox'" + String(isPollingPaused ? "" : " checked") + " onchange=\"window.location.href='/toggle_pause?redirect=' + encodeURIComponent(window.location.pathname + window.location.search);\">\n";
        h += "            <span class='switch-slider'></span>\n";
        h += "          </span>\n";
        h += "        </label>\n";
        h += "      </div>\n";
        h += "      <div class='nav-actions'>\n";
        h += "        <div class='theme-switch' role='group' aria-label='Theme switcher'>\n";
        h += "          <button type='button' id='themeBtnLight' class='theme-btn' title='Light Theme' onclick=\"setTheme('light')\">\n";
        h += "            <svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='4'/><path d='M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41'/></svg>\n";
        h += "          </button>\n";
        h += "          <button type='button' id='themeBtnDark' class='theme-btn' title='Dark Theme' onclick=\"setTheme('dark')\">\n";
        h += "            <svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z'/></svg>\n";
        h += "          </button>\n";
        h += "          <button type='button' id='themeBtnSystem' class='theme-btn' title='System Theme' onclick=\"setTheme('system')\">\n";
        h += "            <svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='2' y='3' width='20' height='14' rx='2'/><line x1='8' y1='21' x2='16' y2='21'/><line x1='12' y1='17' x2='12' y2='21'/></svg>\n";
        h += "          </button>\n";
        h += "        </div>\n";
        h += "      </div>\n";
        h += "    </div>\n";
        h += "  </div>\n";
        h += "</div>\n";
        h += "<div class='container'>\n";

        if (isApMode) {
            h += "  <div class='ap-banner'>\n";
            h += "    <span>📡 <b>Standalone AP Mode (192.168.4.1)</b> &bull; Offline Field Diagnostic Mode</span>\n";
            h += "    <a href='/wifi'>Configure WiFi &rarr;</a>\n";
            h += "  </div>\n";
        }

        return h;
    }

    String renderFooter() {
        String f;
        f.reserve(512);
        f += "</div>\n"; // close .container
        f += "<footer class='footer'>\n";
        f += "  <div>Pylon Smart Monitor <strong>v" + String(FIRMWARE_VERSION) + "</strong> &bull; Build: " + String(FIRMWARE_BUILD_DATE) + " " + String(FIRMWARE_BUILD_TIME) + "</div>\n";
        f += "  <div id='footerScrapeInfo' style='font-size:0.78rem;color:#718096;margin-top:4px;'>Last BMS Scrape: <b>" + String(stack.scrapeDurationMs / 1000.0f, 2) + "s</b> (" + (stack.scrapeSuccess ? "<span style='color:#16a34a;font-weight:700;'>OK</span>" : "<span style='color:#dc2626;font-weight:700;'>FAIL</span>") + ")</div>\n";
        f += "</footer>\n";
        f += "</body>\n</html>\n";
        return f;
    }

    struct StackAnalytics {
        float stackVolt = 0.0f;
        float stackCurr = 0.0f;
        float stackPower = 0.0f;
        float avgSoc = 0.0f;
        int activeSoh = -1;
        uint16_t lowestCellV = 0;
        uint8_t lowestMod = 0, lowestCellIdx = 0;
        uint16_t highestCellV = 0;
        uint8_t highestMod = 0, highestCellIdx = 0;
        uint16_t spreadMv = 0;
        double avgCellV = 0.0;
        double stdDev = 0.0;
        float minCellT = 0.0f;
        uint8_t minCellTMod = 0, minCellTIdx = 0;
        float maxCellT = 0.0f;
        uint8_t maxCellTMod = 0, maxCellTIdx = 0;
        bool foundCellT = false;
    };

    StackAnalytics calculateStackAnalytics() {
        StackAnalytics a;
        uint8_t validPwr = 0;

        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                a.stackVolt = stack.modules[m].power.voltMv / 1000.0f;
                a.stackCurr += (stack.modules[m].power.currMa / 1000.0f);
                a.avgSoc += stack.modules[m].power.socPercent;
                validPwr++;
            }
            int modSoh = (stack.modules[m].stats.sohPercent > 0) ? stack.modules[m].stats.sohPercent : getEffectiveSoh(stack.modules[m].stats);
            if (stack.modules[m].present && modSoh > 0) {
                if (a.activeSoh < 0 || m == stack.activeModuleIndex) {
                    a.activeSoh = modSoh;
                }
            }
        }
        if (validPwr > 0) a.avgSoc /= validPwr;
        a.stackPower = a.stackVolt * a.stackCurr;

        uint16_t lowV = 65535, highV = 0;
        uint32_t totalCellCount = 0;
        double sumCellV = 0.0;
        float minT = 100.0f, maxT = -40.0f;

        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (!stack.modules[m].present) continue;
            uint8_t nCells = stack.modules[m].cellCountParsed > 0 ? stack.modules[m].cellCountParsed : 15;
            for (uint8_t c = 0; c < nCells; ++c) {
                uint16_t v = stack.modules[m].cells[c].voltMv;
                if (v > 0) {
                    if (v < lowV) {
                        lowV = v;
                        a.lowestMod = m;
                        a.lowestCellIdx = c;
                    }
                    if (v > highV) {
                        highV = v;
                        a.highestMod = m;
                        a.highestCellIdx = c;
                    }
                    sumCellV += v;
                    totalCellCount++;
                }
                int32_t rawT = stack.modules[m].cells[c].tempMdeg;
                if (rawT != 0 || v > 0) {
                    float t = rawT / 1000.0f;
                    if (t > -30.0f && t < 100.0f && rawT != 0) {
                        if (t < minT) {
                            minT = t;
                            a.minCellTMod = m;
                            a.minCellTIdx = c;
                            a.foundCellT = true;
                        }
                        if (t > maxT) {
                            maxT = t;
                            a.maxCellTMod = m;
                            a.maxCellTIdx = c;
                            a.foundCellT = true;
                        }
                    }
                }
            }
        }

        if (!a.foundCellT) {
            for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
                if (!stack.modules[m].present || !stack.modules[m].power.valid) continue;
                float t = stack.modules[m].power.tempMdeg / 1000.0f;
                if (t > -30.0f && t < 100.0f && stack.modules[m].power.tempMdeg != 0) {
                    if (t < minT) { minT = t; a.minCellTMod = m; a.minCellTIdx = 0; a.foundCellT = true; }
                    if (t > maxT) { maxT = t; a.maxCellTMod = m; a.maxCellTIdx = 0; a.foundCellT = true; }
                }
            }
        }
        if (a.foundCellT) {
            a.minCellT = minT;
            a.maxCellT = maxT;
        }

        if (totalCellCount > 0) {
            a.lowestCellV = lowV;
            a.highestCellV = highV;
            a.spreadMv = highV - lowV;
            a.avgCellV = sumCellV / totalCellCount;
            double sumSqDiff = 0.0;
            for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
                if (!stack.modules[m].present) continue;
                uint8_t nCells = stack.modules[m].cellCountParsed > 0 ? stack.modules[m].cellCountParsed : 15;
                for (uint8_t c = 0; c < nCells; ++c) {
                    uint16_t v = stack.modules[m].cells[c].voltMv;
                    if (v > 0) {
                        double diff = v - a.avgCellV;
                        sumSqDiff += diff * diff;
                    }
                }
            }
            a.stdDev = sqrt(sumSqDiff / totalCellCount);
        }

        return a;
    }

    // =========================================================================
    // 1. Dashboard Page (`/`)
    // =========================================================================
    void handleDashboard() {
        StackAnalytics a = calculateStackAnalytics();

        if (isApMode && stack.moduleCount == 0) {
            triggerManualPoll = true;
        }

        ChunkedHtmlSender html(server);
        html += renderHeader("Dashboard", "dash");

        if (stack.moduleCount == 0) {
            html += "<div style='background:#f0fdfa;border:1.5px solid var(--teal);border-left:5px solid var(--teal);color:var(--navy);padding:12px 18px;border-radius:8px;margin-bottom:18px;font-size:0.9rem;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;'>\n";
            html += "  <div><span style='font-size:1.1rem;margin-right:6px;'>⏳</span> <b>Initial Battery Telemetry Scrape in Progress...</b> Connecting to Pylontech BMS. Please wait a few seconds and <a href='/' style='color:#008b91;font-weight:700;text-decoration:underline;'>refresh this page</a> or click <b>Poll Now</b>.</div>\n";
            html += "  <a href='/poll_now' class='btn btn-outline' style='padding:5px 12px;font-size:0.82rem;'>Poll Now</a>\n";
            html += "</div>\n";
        }

        // Dynamic colors: Positive (charging) = green, Negative (discharging) = red, Zero = blue/navy
        String currColor = "color:var(--navy);";
        String currCardClass = "card card-navy";
        if (a.stackCurr > 0.05f) {
            currColor = "color:#16a34a;";
            currCardClass = "card card-green";
        } else if (a.stackCurr < -0.05f) {
            currColor = "color:#dc2626;";
            currCardClass = "card card-red";
        }

        String pwrColor = "color:var(--navy);";
        String pwrCardClass = "card card-navy";
        if (a.stackPower > 1.0f) {
            pwrColor = "color:#16a34a;";
            pwrCardClass = "card card-green";
        } else if (a.stackPower < -1.0f) {
            pwrColor = "color:#dc2626;";
            pwrCardClass = "card card-red";
        }

        char bufLow[16], bufHigh[16], bufAvg[16];
        snprintf(bufLow, sizeof(bufLow), "%.3f", a.lowestCellV / 1000.0f);
        snprintf(bufHigh, sizeof(bufHigh), "%.3f", a.highestCellV / 1000.0f);
        snprintf(bufAvg, sizeof(bufAvg), "%.3f", (a.avgCellV > 0) ? (a.avgCellV / 1000.0) : 0.0);

        String strLow = String(bufLow);
        String strHigh = String(bufHigh);
        String strAvg = String(bufAvg);

        char bufMinT[16], bufMaxT[16];
        if (fabsf(a.minCellT - roundf(a.minCellT)) < 0.05f) {
            snprintf(bufMinT, sizeof(bufMinT), "%d", (int)roundf(a.minCellT));
        } else {
            snprintf(bufMinT, sizeof(bufMinT), "%.1f", a.minCellT);
        }
        if (fabsf(a.maxCellT - roundf(a.maxCellT)) < 0.05f) {
            snprintf(bufMaxT, sizeof(bufMaxT), "%d", (int)roundf(a.maxCellT));
        } else {
            snprintf(bufMaxT, sizeof(bufMaxT), "%.1f", a.maxCellT);
        }
        String strMinT = String(bufMinT);
        String strMaxT = String(bufMaxT);

        String subLow = (stack.moduleCount > 1) ? ("Battery " + String(a.lowestMod) + ", cell " + String(a.lowestCellIdx)) : ("Cell " + String(a.lowestCellIdx));
        String subHigh = (stack.moduleCount > 1) ? ("Battery " + String(a.highestMod) + ", cell " + String(a.highestCellIdx)) : ("Cell " + String(a.highestCellIdx));
        String subLowT = (stack.moduleCount > 1) ? ("Battery " + String(a.minCellTMod) + ", cell " + String(a.minCellTIdx)) : ("Cell " + String(a.minCellTIdx));
        String subHighT = (stack.moduleCount > 1) ? ("Battery " + String(a.maxCellTMod) + ", cell " + String(a.maxCellTIdx)) : ("Cell " + String(a.maxCellTIdx));

        // 2. Summary & Analytics Telemetry Cards (6 columns x 2 rows grid)
        html += "<div class='grid-dash'>\n";

        // --- Row 1 ---
        // 1. Average SOC
        html += "  <div class='card card-navy'><h3>Average SOC</h3><div class='val'><span id='dashSoc'>" + String((int)round(a.avgSoc)) + "</span><span class='unit'>%</span></div></div>\n";
        // 2. Total Current
        html += "  <div id='dashCurrCard' class='" + currCardClass + "'><h3>Total Current</h3><div class='val' style='" + currColor + "'><span id='dashCurr'>" + String(a.stackCurr, 2) + "</span><span class='unit'>A</span></div></div>\n";
        // 3. Highest Cell (Voltage)
        html += "  <div class='card card-blue'><h3>Highest Cell</h3><div class='val'><span id='dashHighV'>" + strHigh + "</span><span class='unit'>V</span></div><div class='sub' id='dashHighVSub'>" + subHigh + "</div></div>\n";
        // 4. Highest Cell (Temperature)
        html += "  <div class='card card-blue'><h3>Highest Cell</h3><div class='val'><span id='dashHighT'>" + (a.foundCellT ? strMaxT : "N/A") + "</span><span id='dashHighTUnit' class='unit'" + (a.foundCellT ? "" : " style='display:none;'") + ">&deg;C</span></div><div class='sub' id='dashHighTSub'>" + (a.foundCellT ? subHighT : "") + "</div></div>\n";
        // 5. Spread
        html += "  <div class='card'><h3>Spread</h3><div class='val'><span id='dashSpread'>" + String(a.spreadMv) + "</span><span class='unit'>mV</span></div><div class='sub'>Worst &Delta;V Spread</div></div>\n";
        // 6. Stack SOH
        html += "  <div class='card card-teal'><h3>Stack SOH</h3><div class='val'><span id='dashSoh'>" + (a.activeSoh > 0 ? String(a.activeSoh) : "N/A") + "</span><span id='dashSohUnit' class='unit'" + (a.activeSoh > 0 ? "" : " style='display:none;'") + ">%</span></div></div>\n";

        // --- Row 2 ---
        // 7. Stack Voltage
        html += "  <div class='card card-green'><h3>Stack Voltage</h3><div class='val'><span id='dashVolt'>" + String(a.stackVolt, 2) + "</span><span class='unit'>V</span></div></div>\n";
        // 8. Total Power
        html += "  <div id='dashPwrCard' class='" + pwrCardClass + "'><h3>Total Power</h3><div class='val' style='" + pwrColor + "'><span id='dashPwr'>" + String(a.stackPower, 1) + "</span><span class='unit'>W</span></div></div>\n";
        // 9. Lowest Cell (Voltage)
        html += "  <div class='card card-blue'><h3>Lowest Cell</h3><div class='val'><span id='dashLowV'>" + strLow + "</span><span class='unit'>V</span></div><div class='sub' id='dashLowVSub'>" + subLow + "</div></div>\n";
        // 10. Lowest Cell (Temperature)
        html += "  <div class='card card-blue'><h3>Lowest Cell</h3><div class='val'><span id='dashLowT'>" + (a.foundCellT ? strMinT : "N/A") + "</span><span id='dashLowTUnit' class='unit'" + (a.foundCellT ? "" : " style='display:none;'") + ">&deg;C</span></div><div class='sub' id='dashLowTSub'>" + (a.foundCellT ? subLowT : "") + "</div></div>\n";
        // 11. Average
        html += "  <div class='card'><h3>Average</h3><div class='val'><span id='dashAvg'>" + strAvg + "</span><span class='unit'>V</span></div><div class='sub'>Across all cells</div></div>\n";
        // 12. Deviation
        html += "  <div class='card'><h3>Deviation</h3><div class='val'><span id='dashDev'>" + String(a.stdDev, 1) + "</span><span class='unit'>mV</span></div><div class='sub'>Stack cell balance</div></div>\n";

        html += "</div>\n";

        // 3b. 24-Hour Telemetry History Chart
        html += "<div class='table-card' style='margin-bottom:22px;'>\n";
        html += "  <div class='table-header' style='flex-wrap:wrap;gap:10px;'>\n";
        html += "    <div>\n";
        html += "      <h3 style='display:flex;align-items:center;gap:8px;'><span>📈</span> 24-Hour Telemetry History</h3>\n";
        html += "      <small id='chartSub' style='color:#718096;font-weight:600;'>Loading telemetry data...</small>\n";
        html += "    </div>\n";
        html += "    <div style='display:flex;align-items:center;gap:10px;flex-wrap:wrap;'>\n";
        html += "      <div class='chart-btn-group'>\n";
        html += "        <button type='button' class='chart-btn' onclick='setChartRange(3600,this)'>1h</button>\n";
        html += "        <button type='button' class='chart-btn' onclick='setChartRange(21600,this)'>6h</button>\n";
        html += "        <button type='button' class='chart-btn' onclick='setChartRange(43200,this)'>12h</button>\n";
        html += "        <button type='button' class='chart-btn active' onclick='setChartRange(86400,this)'>24h</button>\n";
        html += "      </div>\n";
        html += "      <div style='display:flex;gap:12px;align-items:center;font-size:0.84rem;font-weight:600;'>\n";
        html += "        <label class='chart-leg-soc'><input type='checkbox' id='chkSoc' checked onchange='drawChart()'> <span style='font-size:0.95rem;line-height:1;'>●</span> SOC (%)</label>\n";
        html += "        <label class='chart-leg-curr'><input type='checkbox' id='chkCurr' checked onchange='drawChart()'> - - Current (A)</label>\n";
        html += "      </div>\n";
        html += "    </div>\n";
        html += "  </div>\n";
        html += "  <div style='position:relative;padding:12px 14px 14px 14px;background:var(--card);'>\n";
        html += "    <div style='position:relative;width:100%;height:270px;'>\n";
        html += "      <canvas id='telemetryCanvas' style='width:100%;height:100%;display:block;'></canvas>\n";
        html += "      <div id='chartTooltip' style='display:none;position:absolute;pointer-events:none;background:rgba(15,23,42,0.92);color:#fff;border-radius:6px;padding:7px 11px;font-size:0.78rem;box-shadow:0 4px 12px rgba(0,0,0,0.25);z-index:10;white-space:nowrap;line-height:1.4;'></div>\n";
        html += "    </div>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // 4. Modules Table (with Spread renamed and Device links)
        html += "<div class='table-card'>\n";
        html += "  <div class='table-header'>\n";
        html += "    <h3>🔋 Battery Modules Detail</h3>\n";
        html += "    <small style='color:#718096;font-weight:600;'>Active Modules: <span id='dashActiveMods'>" + String(stack.moduleCount) + "</span> &bull; Click # or Device for Rack View</small>\n";
        html += "  </div>\n";
        html += "  <table>\n";
        html += "    <thead>\n";
        html += "      <tr><th>#</th><th>Device</th><th>Voltage</th><th>Current</th><th style='white-space:nowrap;'>SOC</th><th>Spread</th><th>Temp</th><th>MOSFET</th><th>Base State</th></tr>\n";
        html += "    </thead>\n";
        html += "    <tbody>\n";

        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present) continue;

            const ModulePower &p = mod.power;
            const ModuleInfo &info = mod.info;

            String devName;
            if (strlen(info.deviceName) > 0) {
                devName = info.deviceName;
            } else if (stack.model != MODEL_UNKNOWN && m == stack.activeModuleIndex) {
                devName = stack.modelName;
            } else {
                devName = "Device " + String(m);
            }

            int vSpread = (p.voltHighMv > p.voltLowMv) ? (p.voltHighMv - p.voltLowMv) : 0;
            if (vSpread == 0 && mod.cellCountParsed > 0) {
                uint16_t vMin = 65535, vMax = 0;
                for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                    if (mod.cells[c].voltMv > 0) {
                        if (mod.cells[c].voltMv < vMin) vMin = mod.cells[c].voltMv;
                        if (mod.cells[c].voltMv > vMax) vMax = mod.cells[c].voltMv;
                    }
                }
                if (vMax >= vMin && vMin > 0) vSpread = vMax - vMin;
            }

            String spreadBadge = (vSpread <= 20) ? "badge-ok" : ((vSpread <= 50) ? "badge-warn" : "badge-danger");

            String badgeClass = "badge-ok";
            if (String(p.baseState).equalsIgnoreCase("Charge")) badgeClass = "badge-charge";
            else if (String(p.baseState).equalsIgnoreCase("Dischg")) badgeClass = "badge-warn";

            float modCurrVal = p.currMa / 1000.0f;
            String modCurrColor = "color:var(--navy);";
            if (modCurrVal > 0.05f) modCurrColor = "color:#16a34a;font-weight:700;";
            else if (modCurrVal < -0.05f) modCurrColor = "color:#dc2626;font-weight:700;";

            int socVal = p.socPercent;
            if (socVal < 0) socVal = 0;
            if (socVal > 100) socVal = 100;
            String batFillCol = (socVal >= 50) ? "#16a34a" : ((socVal >= 20) ? "#f59e0b" : "#dc2626");

            html += "      <tr>\n";
            html += "        <td><a class='mod-link' href='/module?m=" + String(m) + "'>#" + String(m) + "</a></td>\n";
            html += "        <td><a class='mod-dev-link' href='/module?m=" + String(m) + "'><b>" + devName + "</b></a></td>\n";
            html += "        <td id='mVolt_" + String(m) + "'><b>" + String(p.voltMv / 1000.0f, 3) + " V</b></td>\n";
            html += "        <td id='mCurr_" + String(m) + "' style='" + modCurrColor + "'>" + String(modCurrVal, 2) + " A</td>\n";
            html += "        <td style='white-space:nowrap;'><div style='display:inline-flex;align-items:center;gap:6px;'><div class='bat-shell'><div id='mBatFill_" + String(m) + "' class='bat-fill' style='width:" + String(socVal) + "%;background:" + batFillCol + ";'></div></div><b id='mSoc_" + String(m) + "'>" + String(socVal) + "%</b></div></td>\n";
            html += "        <td><span id='mSpread_" + String(m) + "' class='badge " + spreadBadge + "'>" + String(vSpread) + " mV</span></td>\n";
            html += "        <td id='mTemp_" + String(m) + "'>" + String(p.tempMdeg / 1000.0f, 1) + " °C</td>\n";
            html += "        <td id='mMos_" + String(m) + "'>" + (p.mosTempMdeg > 0 ? (String(p.mosTempMdeg / 1000.0f, 1) + " °C") : "N/A") + "</td>\n";
            html += "        <td><span id='mState_" + String(m) + "' class='badge " + badgeClass + "'>" + String(p.baseState) + "</span></td>\n";
            html += "      </tr>\n";
        }

        html += "    </tbody>\n";
        html += "  </table>\n";
        html += "</div>\n";

        // 5. System Information Card (Placed below Battery Modules Detail table)
        unsigned long totalSec = millis() / 1000;
        unsigned long days = totalSec / 86400; totalSec %= 86400;
        unsigned long hrs = totalSec / 3600; totalSec %= 3600;
        unsigned long mins = totalSec / 60;
        String uptimeStr = (days > 0 ? (String(days) + "d ") : "") + String(hrs) + "h " + String(mins) + "m";

        String ipStr = isApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        String wifiSig;
        if (isApMode) {
            wifiSig = "AP Mode (192.168.4.1)";
        } else {
            int rssi = WiFi.RSSI();
            String q = "(weak)";
            if (rssi >= -60) q = "(excellent)";
            else if (rssi >= -70) q = "(good)";
            else if (rssi >= -80) q = "(fair)";
            wifiSig = String(rssi) + " dBm " + q;
        }

        bool mqEn = prefs.getBool(NVS_KEY_MQTT_ENABLED, false);
        String mqStatus;
        if (!mqEn) {
            mqStatus = "<span style='color:#94a3b8;'>Not configured</span>";
        } else if (mqttClient.isConnected()) {
            mqStatus = "<span style='color:#16a34a;font-weight:700;'>Connected</span>";
        } else {
            mqStatus = "<span style='color:#dc2626;font-weight:700;'>Disconnected</span>";
        }

        String batLink = stack.scrapeSuccess ? "<span style='color:#16a34a;font-weight:700;'>Connected</span>" : "<span style='color:#dc2626;font-weight:700;'>Disconnected</span>";

        SystemStats sys = SystemMonitor::getSnapshot();

        String ssidStr = isApMode ? "AP Mode" : (sys.wifiSsid.length() > 0 ? sys.wifiSsid : (WiFi.SSID().length() > 0 ? WiFi.SSID() : "Disconnected"));
        String hostName = getDeviceHostname(prefs) + ".local";

        html += "<div class='sys-card' style='margin-top:22px;'>\n";
        html += "  <div class='sys-hdr'>⚙️ System Status & Diagnostics</div>\n";
        html += "  <div class='sys-grid'>\n";
        html += "    <div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>IP address</span><span class='sys-val'>" + ipStr + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Hostname</span><span class='sys-val'>" + hostName + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>WiFi signal</span><span id='sysWifi' class='sys-val'>" + wifiSig + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>WiFi SSID</span><span class='sys-val'>" + ssidStr + "</span></div>\n";
        html += "    </div>\n";
        html += "    <div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Battery link</span><span id='sysBatLink' class='sys-val'>" + batLink + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Battery units</span><span id='sysBatUnits' class='sys-val'>" + String(stack.moduleCount) + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Uptime</span><span id='sysUptime' class='sys-val'>" + uptimeStr + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Home Assistant (MQTT)</span><span class='sys-val'>" + mqStatus + "</span></div>\n";
        html += "    </div>\n";
        html += "    <div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Free RAM</span><span id='sysRam' class='sys-val'>" + String(sys.heapFree / 1024) + " KB (" + String(sys.heapFragPct) + "% frag)</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>CPU Load / Temp</span><span id='sysCpu' class='sys-val'>" + String(sys.cpuLoadPct, 1) + "% / " + String(sys.cpuTempC, 1) + " °C</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Reset reason</span><span class='sys-val'>" + sys.resetReason + "</span></div>\n";
        html += "      <div class='sys-row'><span class='sys-label'>Chip</span><span class='sys-val'>" + sys.chipModel + " (" + String(sys.cpuFreqMHz) + " MHz)</span></div>\n";
        html += "    </div>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // Discovered Peer Monitors Card (Only rendered if another Pylon Monitor is found on the network)
        auto peers = peerDiscovery.getPeers();
        if (!peers.empty()) {
            html += "<div class='table-card' style='margin-top:22px;'>\n";
            html += "  <div class='table-header'>\n";
            html += "    <div style='font-weight:700;color:var(--navy);font-size:0.95rem;display:flex;align-items:center;gap:8px;'>\n";
            html += "      🌐 Pylon Smart Monitors on Network\n";
            html += "    </div>\n";
            html += "    <span class='badge badge-ok'>" + String(peers.size()) + " Online</span>\n";
            html += "  </div>\n";
            html += "  <table class='data-table'>\n";
            html += "    <thead>\n";
            html += "      <tr>\n";
            html += "        <th>Device Name</th>\n";
            html += "        <th>IP Address</th>\n";
            html += "        <th>Battery Model</th>\n";
            html += "        <th>Firmware</th>\n";
            html += "        <th style='text-align:right;'>Action</th>\n";
            html += "      </tr>\n";
            html += "    </thead>\n";
            html += "    <tbody>\n";
            for (const auto &peer : peers) {
                html += "      <tr>\n";
                html += "        <td><a href='http://" + peer.ip.toString() + "/' target='_blank' class='mod-dev-link'><b>" + peer.hostname + ".local</b></a></td>\n";
                html += "        <td><a href='http://" + peer.ip.toString() + "/' target='_blank' class='mod-dev-link'><code>" + peer.ip.toString() + "</code></a></td>\n";
                html += "        <td>" + (peer.model.length() > 0 ? ("<b>" + peer.model + "</b>") : "<span style='color:#718096;'>-</span>") + "</td>\n";
                html += "        <td>" + (peer.version.length() > 0 ? ("<span class='badge badge-ok'>v" + peer.version + "</span>") : "<span style='color:#718096;'>-</span>") + "</td>\n";
                html += "        <td style='text-align:right;'><a href='http://" + peer.ip.toString() + "/' target='_blank' class='btn btn-outline' style='padding:4px 12px;font-size:0.80rem;text-decoration:none;'>Open Dashboard ↗</a></td>\n";
                html += "      </tr>\n";
            }
            html += "    </tbody>\n";
            html += "  </table>\n";
            html += "</div>\n";
        }

        bool is24h = prefs.getBool(NVS_KEY_TIME_FORMAT_24H, true);
        html += "<script>var CONFIG_TIME_24H=" + String(is24h ? "true" : "false") + ";</script>\n";
        html += FPSTR(DASHBOARD_CHART_JS);
        html += "\n";

        // Live Dashboard AJAX Polling Script
        bool apiAuthEn = prefs.getBool(NVS_KEY_API_AUTH_ENABLED, false);
        String apiTok = apiAuthEn ? prefs.getString(NVS_KEY_API_TOKEN, "") : "";

        html += "<script>\n";
        html += "(function(){\n";
        html += "  var apiTok = '" + apiTok + "';\n";
        html += "  var initMods = " + String(stack.moduleCount) + ";\n";
        html += "  function fmtT(t) { return (Math.abs(t - Math.round(t)) < 0.05) ? Math.round(t) : t.toFixed(1); }\n";
        html += "  function updateDashboardLive(){\n";
        html += "    var url = '/api/data?brief=1' + (apiTok ? '&token=' + encodeURIComponent(apiTok) : '');\n";
        html += "    fetch(url)\n";
        html += "      .then(function(r){ return r.json(); })\n";
        html += "      .then(function(d){\n";
        html += "        if (!d || !d.stack) return;\n";
        html += "        var s = d.stack;\n";
        html += "        if (initMods === 0 && s.modules_detected > 0) { window.location.reload(); return; }\n";
        html += "        var el, card, u, sub;\n";
        html += "        el = document.getElementById('dashSoc'); if (el) el.textContent = s.soc;\n";
        html += "        el = document.getElementById('dashCurr'); card = document.getElementById('dashCurrCard');\n";
        html += "        if (el) {\n";
        html += "          el.textContent = s.current.toFixed(2);\n";
        html += "          var col = 'var(--navy)', cls = 'card card-navy';\n";
        html += "          if (s.current > 0.05) { col = '#16a34a'; cls = 'card card-green'; }\n";
        html += "          else if (s.current < -0.05) { col = '#dc2626'; cls = 'card card-red'; }\n";
        html += "          el.style.color = col; if (card) card.className = cls;\n";
        html += "        }\n";
        html += "        el = document.getElementById('dashHighV'); if (el) el.textContent = s.highest_cell_v.toFixed(3);\n";
        html += "        el = document.getElementById('dashHighVSub'); if (el) el.textContent = (s.modules_detected > 1) ? ('Battery ' + s.highest_cell_mod + ', cell ' + s.highest_cell_idx) : ('Cell ' + s.highest_cell_idx);\n";
        html += "        el = document.getElementById('dashHighT'); u = document.getElementById('dashHighTUnit'); sub = document.getElementById('dashHighTSub');\n";
        html += "        if (el) {\n";
        html += "          if (s.found_cell_t) {\n";
        html += "            el.textContent = fmtT(s.max_cell_t);\n";
        html += "            if (u) u.style.display = '';\n";
        html += "            if (sub) sub.textContent = (s.modules_detected > 1) ? ('Battery ' + s.max_cell_t_mod + ', cell ' + s.max_cell_t_idx) : ('Cell ' + s.max_cell_t_idx);\n";
        html += "          } else {\n";
        html += "            el.textContent = 'N/A'; if (u) u.style.display = 'none'; if (sub) sub.textContent = '';\n";
        html += "          }\n";
        html += "        }\n";
        html += "        el = document.getElementById('dashSpread'); if (el) el.textContent = s.spread_mv;\n";
        html += "        el = document.getElementById('dashSoh'); u = document.getElementById('dashSohUnit');\n";
        html += "        if (el) {\n";
        html += "          if (s.soh !== null && s.soh > 0) {\n";
        html += "            el.textContent = s.soh; if (u) u.style.display = '';\n";
        html += "          } else {\n";
        html += "            el.textContent = 'N/A'; if (u) u.style.display = 'none';\n";
        html += "          }\n";
        html += "        }\n";
        html += "        el = document.getElementById('dashVolt'); if (el) el.textContent = s.voltage.toFixed(2);\n";
        html += "        el = document.getElementById('dashPwr'); card = document.getElementById('dashPwrCard');\n";
        html += "        if (el) {\n";
        html += "          el.textContent = s.power.toFixed(1);\n";
        html += "          var pCol = 'var(--navy)', pCls = 'card card-navy';\n";
        html += "          if (s.power > 1.0) { pCol = '#16a34a'; pCls = 'card card-green'; }\n";
        html += "          else if (s.power < -1.0) { pCol = '#dc2626'; pCls = 'card card-red'; }\n";
        html += "          el.style.color = pCol; if (card) card.className = pCls;\n";
        html += "        }\n";
        html += "        el = document.getElementById('dashLowV'); if (el) el.textContent = s.lowest_cell_v.toFixed(3);\n";
        html += "        el = document.getElementById('dashLowVSub'); if (el) el.textContent = (s.modules_detected > 1) ? ('Battery ' + s.lowest_cell_mod + ', cell ' + s.lowest_cell_idx) : ('Cell ' + s.lowest_cell_idx);\n";
        html += "        el = document.getElementById('dashLowT'); u = document.getElementById('dashLowTUnit'); sub = document.getElementById('dashLowTSub');\n";
        html += "        if (el) {\n";
        html += "          if (s.found_cell_t) {\n";
        html += "            el.textContent = fmtT(s.min_cell_t);\n";
        html += "            if (u) u.style.display = '';\n";
        html += "            if (sub) sub.textContent = (s.modules_detected > 1) ? ('Battery ' + s.min_cell_t_mod + ', cell ' + s.min_cell_t_idx) : ('Cell ' + s.min_cell_t_idx);\n";
        html += "          } else {\n";
        html += "            el.textContent = 'N/A'; if (u) u.style.display = 'none'; if (sub) sub.textContent = '';\n";
        html += "          }\n";
        html += "        }\n";
        html += "        el = document.getElementById('dashAvg'); if (el) el.textContent = s.avg_cell_v.toFixed(3);\n";
        html += "        el = document.getElementById('dashDev'); if (el) el.textContent = s.std_dev_mv.toFixed(1);\n";
        html += "        el = document.getElementById('dashActiveMods'); if (el) el.textContent = s.modules_detected;\n";
        html += "        el = document.getElementById('sysBatUnits'); if (el) el.textContent = s.modules_detected;\n";
        html += "        el = document.getElementById('sysBatLink'); if (el) el.innerHTML = s.scrape_success ? \"<span style='color:#16a34a;font-weight:700;'>Connected</span>\" : \"<span style='color:#dc2626;font-weight:700;'>Disconnected</span>\";\n";
        html += "        el = document.getElementById('sysScrape');\n";
        html += "        if (el) {\n";
        html += "          var dur = (s.scrape_duration_ms / 1000.0).toFixed(2);\n";
        html += "          var st = s.scrape_success ? \"<span style='color:#16a34a;font-weight:700;'>OK</span>\" : \"<span style='color:#dc2626;font-weight:700;'>FAIL</span>\";\n";
        html += "          el.innerHTML = dur + 's (' + st + ')';\n";
        html += "        }\n";
        html += "        el = document.getElementById('footerScrapeInfo');\n";
        html += "        if (el) {\n";
        html += "          var dur = (s.scrape_duration_ms / 1000.0).toFixed(2);\n";
        html += "          var st = s.scrape_success ? \"<span style='color:#16a34a;font-weight:700;'>OK</span>\" : \"<span style='color:#dc2626;font-weight:700;'>FAIL</span>\";\n";
        html += "          el.innerHTML = 'Last BMS Scrape: <b>' + dur + 's</b> (' + st + ')';\n";
        html += "        }\n";
        html += "        if (d.system) {\n";
        html += "          var sys = d.system;\n";
        html += "          el = document.getElementById('sysWifi'); if (el) el.textContent = sys.wifi_rssi_dbm + ' dBm (' + sys.wifi_signal_pct + '%)';\n";
        html += "          el = document.getElementById('sysRam'); if (el) el.textContent = Math.round(sys.heap_free_bytes / 1024) + ' KB (' + sys.heap_fragmentation_pct + '% frag)';\n";
        html += "          el = document.getElementById('sysCpu'); if (el) el.textContent = sys.cpu_usage_pct.toFixed(1) + '% / ' + sys.cpu_temp_c.toFixed(1) + ' °C';\n";
        html += "          el = document.getElementById('sysUptime');\n";
        html += "          if (el && sys.uptime_sec !== undefined) {\n";
        html += "            var sec = sys.uptime_sec;\n";
        html += "            var days = Math.floor(sec / 86400); sec %= 86400;\n";
        html += "            var hrs = Math.floor(sec / 3600); sec %= 3600;\n";
        html += "            var mins = Math.floor(sec / 60);\n";
        html += "            el.textContent = (days > 0 ? (days + 'd ') : '') + hrs + 'h ' + mins + 'm';\n";
        html += "          }\n";
        html += "        }\n";
        html += "        if (d.modules) {\n";
        html += "          d.modules.forEach(function(m){\n";
        html += "            var id = m.id;\n";
        html += "            var mv = document.getElementById('mVolt_' + id); if (mv) mv.innerHTML = '<b>' + m.voltage.toFixed(3) + ' V</b>';\n";
        html += "            var mc = document.getElementById('mCurr_' + id);\n";
        html += "            if (mc) {\n";
        html += "              mc.textContent = m.current.toFixed(2) + ' A';\n";
        html += "              mc.style.color = (m.current > 0.05) ? '#16a34a' : ((m.current < -0.05) ? '#dc2626' : 'var(--navy)');\n";
        html += "              mc.style.fontWeight = (m.current > 0.05 || m.current < -0.05) ? '700' : 'normal';\n";
        html += "            }\n";
        html += "            var ms = document.getElementById('mSoc_' + id); if (ms) ms.textContent = m.soc + '%';\n";
        html += "            var mf = document.getElementById('mBatFill_' + id);\n";
        html += "            if (mf) {\n";
        html += "              var sc = Math.max(0, Math.min(100, m.soc));\n";
        html += "              mf.style.width = sc + '%';\n";
        html += "              mf.style.background = (sc >= 50) ? '#16a34a' : ((sc >= 20) ? '#f59e0b' : '#dc2626');\n";
        html += "            }\n";
        html += "            var msp = document.getElementById('mSpread_' + id);\n";
        html += "            if (msp) {\n";
        html += "              msp.textContent = m.volt_spread_mv + ' mV';\n";
        html += "              msp.className = (m.volt_spread_mv <= 20) ? 'badge badge-ok' : ((m.volt_spread_mv <= 50) ? 'badge badge-warn' : 'badge badge-danger');\n";
        html += "            }\n";
        html += "            var mt = document.getElementById('mTemp_' + id); if (mt) mt.textContent = m.temp_c.toFixed(1) + ' °C';\n";
        html += "            var mm = document.getElementById('mMos_' + id); if (mm) mm.textContent = (m.mos_temp_c !== null) ? (m.mos_temp_c.toFixed(1) + ' °C') : 'N/A';\n";
        html += "            var mst = document.getElementById('mState_' + id);\n";
        html += "            if (mst) {\n";
        html += "              mst.textContent = m.base_state;\n";
        html += "              var stCls = 'badge badge-ok', bs = (m.base_state || '').toLowerCase();\n";
        html += "              if (bs === 'charge') stCls = 'badge badge-charge';\n";
        html += "              else if (bs === 'dischg') stCls = 'badge badge-warn';\n";
        html += "              mst.className = stCls;\n";
        html += "            }\n";
        html += "          });\n";
        html += "        }\n";
        html += "      })\n";
        html += "      .catch(function(e){});\n";
        html += "  }\n";
        html += "  setInterval(updateDashboardLive, 5000);\n";
        html += "})();\n";
        html += "</script>\n";

        html += renderFooter();
        html.finish();
    }

    // =========================================================================
    // 2. Module Detail & 19" Rack Battery Visualization (`/module?m=N`)
    // =========================================================================
    void handleModuleDetail() {
        uint8_t m = 1;
        if (server.hasArg("m")) {
            m = server.arg("m").toInt();
        }
        if (m < 1 || m > MAX_MODULES) m = stack.activeModuleIndex;
        if (m < 1 || m > MAX_MODULES) m = 1;

        const BatteryModule &mod = stack.modules[m];
        const ModulePower &p = mod.power;
        const ModuleInfo &info = mod.info;
        const ModuleStats &st = mod.stats;
        const EuroStats &euro = mod.euro;

        String devName;
        if (strlen(info.deviceName) > 0) {
            devName = info.deviceName;
        } else if (stack.model != MODEL_UNKNOWN && m == stack.activeModuleIndex) {
            devName = stack.modelName;
        } else {
            devName = "Device " + String(m);
        }

        // Voltage spread
        int vSpread = (p.voltHighMv > p.voltLowMv) ? (p.voltHighMv - p.voltLowMv) : 0;
        uint16_t minCellV = 65535, maxCellV = 0;
        float minCellT = 100.0f, maxCellT = -40.0f;

        uint8_t numCells = (mod.cellCountParsed > 0) ? mod.cellCountParsed : 15;
        for (uint8_t c = 0; c < numCells; ++c) {
            uint16_t v = mod.cells[c].voltMv;
            if (v > 0) {
                if (v < minCellV) minCellV = v;
                if (v > maxCellV) maxCellV = v;
            }
            float t = mod.cells[c].tempMdeg / 1000.0f;
            if (t > -30.0f) {
                if (t < minCellT) minCellT = t;
                if (t > maxCellT) maxCellT = t;
            }
        }
        if (minCellV == 65535) minCellV = 0;
        if (vSpread == 0 && maxCellV >= minCellV && minCellV > 0) vSpread = maxCellV - minCellV;
        if (minCellT > maxCellT) { minCellT = p.tempMdeg / 1000.0f; maxCellT = p.tempMdeg / 1000.0f; }

        String barcodeStr = (strlen(info.barcode) > 0) ? String(info.barcode) : "N/A (Slave Unit)";

        ChunkedHtmlSender html(server);
        html += renderHeader("Module #" + String(m) + " (" + devName + ")", "dash");

        // Action Toolbar
        html += "<div style='background:var(--card);border:1px solid var(--border);border-radius:8px;padding:12px 18px;margin-bottom:18px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px;'>\n";
        html += "  <div>\n";
        html += "    <h2 style='margin:0;font-size:1.15rem;color:var(--navy);'>Module #" + String(m) + ": " + devName + "</h2>\n";
        html += "    <span style='font-size:0.82rem;color:#718096;'>Barcode: <b>" + barcodeStr + "</b> &bull; Base State: <b id='modBaseState'>" + String(p.baseState) + "</b></span>\n";
        html += "  </div>\n";
        html += "  <div style='display:flex;gap:8px;'>\n";
        html += "    <a class='btn btn-primary' href='/refresh_module?m=" + String(m) + "'>🔄 Poll Module Data</a>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // Structured Cards above the rack (Charge & Power, Health, Temperatures, Metadata)
        html += "<div class='grid'>\n";
        
        // Card 1: Charge & Power with Circular Gauge
        float modVolt = p.voltMv / 1000.0f;
        float modCurr = p.currMa / 1000.0f;
        float modPower = modVolt * modCurr;
        int soc = p.socPercent;
        float dashoffset = 263.89f - (263.89f * soc / 100.0f);

        html += "  <div class='card card-green'>\n";
        html += "    <h3>⚡ Charge & Power</h3>\n";
        html += "    <div style='display:flex;align-items:center;gap:18px;'>\n";
        html += "      <svg viewBox='0 0 100 100' style='width:96px;height:96px;flex-shrink:0;'>\n";
        html += "        <circle cx='50' cy='50' r='42' stroke='var(--border)' stroke-width='8' fill='none'/>\n";
        html += "        <circle id='modGaugeCircle' cx='50' cy='50' r='42' stroke='url(#pylonGrad)' stroke-width='8' fill='none' stroke-dasharray='263.89' stroke-dashoffset='" + String(dashoffset, 1) + "' stroke-linecap='round' transform='rotate(-90 50 50)'/>\n";
        html += "        <text id='modGaugeText' x='50' y='57' text-anchor='middle' font-weight='800' font-size='22' fill='var(--navy)'>" + String(soc) + "%</text>\n";
        html += "      </svg>\n";
        String mCurrCol = (modCurr > 0.05f) ? "color:#16a34a;" : ((modCurr < -0.05f) ? "color:#dc2626;" : "color:var(--navy);");
        String mPwrCol = (modPower > 1.0f) ? "color:#16a34a;" : ((modPower < -1.0f) ? "color:#dc2626;" : "color:var(--navy);");

        String maxLimitsStr = "N/A (Slave)";
        if (info.valid && (info.maxChargeCurrentMa != 0 || info.maxDischargeCurrentMa != 0)) {
            float maxChgA = abs(info.maxChargeCurrentMa) / 1000.0f;
            float maxDsgA = abs(info.maxDischargeCurrentMa) / 1000.0f;
            if (fabsf(maxChgA - maxDsgA) < 0.1f) {
                maxLimitsStr = "&plusmn;" + String(maxChgA, 0) + " A";
            } else {
                maxLimitsStr = "+" + String(maxChgA, 0) + "/-" + String(maxDsgA, 0) + " A";
            }
        }

        html += "      <div style='font-size:0.86rem;line-height:1.5;'>\n";
        html += "        <div>Volt: <b id='modVolt'>" + String(modVolt, 2) + " V</b></div>\n";
        html += "        <div>Curr: <b id='modCurr' style='" + mCurrCol + "'>" + String(modCurr, 2) + " A</b></div>\n";
        html += "        <div>Power: <b id='modPower' style='" + mPwrCol + "'>" + String(modPower, 1) + " W</b></div>\n";
        html += "        <div>Limits: <b>" + maxLimitsStr + "</b></div>\n";
        html += "      </div>\n";
        html += "    </div>\n";
        html += "  </div>\n";

        // Card 2: Health & Lifetime
        String sohStr = "N/A";
        if (st.valid && st.sohPercent > 0) {
            sohStr = String(st.sohPercent) + "%";
            if (strlen(st.sohStatus) > 0 && strcmp(st.sohStatus, "Normal") != 0) {
                sohStr += " (" + String(st.sohStatus) + ")";
            }
        } else if (strlen(p.sohState) > 0) {
            sohStr = String(p.sohState);
        } else if (st.valid && strlen(st.sohStatus) > 0 && strcmp(st.sohStatus, "Normal") != 0) {
            sohStr = String(st.sohStatus);
        }

        String sohTimesStr;
        if (!st.valid) {
            sohTimesStr = "N/A";
        } else if (st.sohTimes > 0) {
            sohTimesStr = "<b style='color:#dc2626;'>" + String(st.sohTimes) + "</b>";
        } else {
            sohTimesStr = "<b>0</b>";
        }

        html += "  <div class='card card-teal'>\n";
        html += "    <h3>🛡️ Health & Cycles</h3>\n";
        html += "    <div style='font-size:0.86rem;line-height:1.6;'>\n";
        html += "      <div>State of Health (SOH): <b>" + sohStr + "</b></div>\n";
        html += "      <div>SOH Times: " + sohTimesStr + "</div>\n";
        html += "      <div>Charge Cycles: <b>" + (st.valid ? String(st.cycleTimes) : "N/A (Slave Unit)") + "</b></div>\n";
        html += "      <div>Spread (&Delta;V): <b id='modSpread'>" + String(vSpread) + " mV</b></div>\n";
        String dsgStr = "N/A";
        if (st.valid) {
            float dsgAh = getDischargedCapAh(st, stack.model);
            if (dsgAh >= 10000.0f) {
                dsgStr = String(dsgAh, 0) + " Ah";
            } else {
                dsgStr = String(dsgAh, 1) + " Ah";
            }
        }
        html += "      <div>Discharged: <b>" + dsgStr + "</b></div>\n";
        if (euro.valid) {
            html += "      <div>Energy Throughput: <b>" + String((uint32_t)euro.energyThroughputWh / 1000.0f, 1) + " kWh</b></div>\n";
        }
        html += "    </div>\n";
        html += "  </div>\n";

        // Card 3: Temperatures
        html += "  <div class='card card-navy'>\n";
        html += "    <h3>🌡️ Thermal Telemetry</h3>\n";
        html += "    <div style='font-size:0.86rem;line-height:1.6;'>\n";
        html += "      <div>Battery Pack: <b id='modPackTemp'>" + String(p.tempMdeg / 1000.0f, 1) + " °C</b></div>\n";
        html += "      <div>MOSFET Temp: <b id='modMosTemp'>" + (p.mosTempMdeg > 0 ? (String(p.mosTempMdeg / 1000.0f, 1) + " °C") : "N/A") + "</b></div>\n";
        html += "      <div>Min Cell Temp: <b id='modMinTemp'>" + String(minCellT, 1) + " °C</b></div>\n";
        html += "      <div>Max Cell Temp: <b id='modMaxTemp'>" + String(maxCellT, 1) + " °C</b></div>\n";
        html += "      <div>Temp Delta (&Delta;T): <b id='modDeltaTemp'>" + String(maxCellT - minCellT, 1) + " °C</b></div>\n";
        html += "    </div>\n";
        html += "  </div>\n";

        // Card 4: Hardware & Metadata
        String boardDisplay = "N/A";
        if (info.valid) {
            if (strlen(info.boardVersion) > 0 && strlen(info.board) > 0 && strcmp(info.board, info.boardVersion) != 0) {
                boardDisplay = String(info.board) + " (" + String(info.boardVersion) + ")";
            } else if (strlen(info.boardVersion) > 0) {
                boardDisplay = String(info.boardVersion);
            } else if (strlen(info.board) > 0) {
                boardDisplay = String(info.board);
            }
        } else {
            boardDisplay = "N/A (Slave Unit)";
        }

        String specDisplay = "N/A (Slave Unit)";
        if (info.valid) {
            if (info.cellCount > 0 && strlen(info.specification) > 0) {
                specDisplay = String(info.cellCount) + "S (" + String(info.specification) + ")";
            } else if (strlen(info.specification) > 0) {
                specDisplay = String(info.specification);
            } else if (info.cellCount > 0) {
                specDisplay = String(info.cellCount) + "S";
            }
        }

        html += "  <div class='card'>\n";
        html += "    <h3>📋 Hardware Spec</h3>\n";
        html += "    <div style='font-size:0.86rem;line-height:1.6;'>\n";
        html += "      <div>Board: <b>" + boardDisplay + "</b></div>\n";
        html += "      <div>Firmware: <b>" + (info.valid ? (String(info.mainSoftVersion) + " (" + String(info.softVersion) + ")") : "N/A (Slave Unit)") + "</b></div>\n";
        html += "      <div>Release Date: <b>" + (info.valid && strlen(info.releaseDate) > 0 ? String(info.releaseDate) : "N/A") + "</b></div>\n";
        html += "      <div>Specification: <b>" + specDisplay + "</b></div>\n";
        html += "      <div>Barcode: <b>" + barcodeStr + "</b></div>\n";
        html += "    </div>\n";
        html += "  </div>\n";

        html += "</div>\n";

        // 19" Rack Battery Chassis Section
        html += "<div style='background:var(--card);border:1px solid var(--border);border-radius:10px;padding:20px;margin-bottom:22px;box-shadow:0 4px 12px rgba(23,28,97,0.04);'>\n";
        html += "  <div style='display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px;margin-bottom:16px;'>\n";
        html += "    <div>\n";
        html += "      <h3 style='margin:0;font-size:1.05rem;color:var(--navy);'>🔋 19\" Rack Battery Cell Matrix</h3>\n";
        html += "      <span style='font-size:0.82rem;color:#718096;'>Physical cell arrangement (" + String(numCells) + " cells in series) &bull; Dynamic cell voltage & thermal visualization</span>\n";
        html += "    </div>\n";
        // Thermal Color Scale Legend
        html += "    <div class='cell-legend'>\n";
        html += "      <span style='font-weight:600;color:var(--navy);'>Thermal:</span>\n";
        html += "      <div style='display:flex;align-items:center;gap:4px;'><span style='width:9px;height:9px;border-radius:50%;background:#38bdf8;'></span><span style='color:#64748b;'>&lt;18°C</span></div>\n";
        html += "      <div style='display:flex;align-items:center;gap:4px;'><span style='width:9px;height:9px;border-radius:50%;background:#22c55e;'></span><span style='color:#64748b;'>18–25°C</span></div>\n";
        html += "      <div style='display:flex;align-items:center;gap:4px;'><span style='width:9px;height:9px;border-radius:50%;background:#84cc16;'></span><span style='color:#64748b;'>26–32°C</span></div>\n";
        html += "      <div style='display:flex;align-items:center;gap:4px;'><span style='width:9px;height:9px;border-radius:50%;background:#f59e0b;'></span><span style='color:#64748b;'>33–40°C</span></div>\n";
        html += "      <div style='display:flex;align-items:center;gap:4px;'><span style='width:9px;height:9px;border-radius:50%;background:#ef4444;'></span><span style='color:#64748b;'>&gt;40°C</span></div>\n";
        html += "    </div>\n";
        html += "  </div>\n";

        // Chassis Outer Enclosure
        html += "  <div style='background:#1e293b;border:3px solid #0f172a;border-radius:10px;padding:12px 16px;box-shadow:inset 0 2px 8px rgba(0,0,0,0.5);position:relative;'>\n";
        
        // Rack Ears & Handles
        html += "    <div style='position:absolute;left:4px;top:50%;transform:translateY(-50%);width:6px;height:45px;background:#475569;border-radius:3px;'></div>\n";
        html += "    <div style='position:absolute;right:4px;top:50%;transform:translateY(-50%);width:6px;height:45px;background:#475569;border-radius:3px;'></div>\n";

        // Inner Cell Array
        html += "    <div style='background:#0f172a;border:1px solid #334155;border-radius:6px;padding:12px 10px;display:flex;gap:6px;justify-content:space-between;overflow-x:auto;'>\n";

        auto getCellTempColor = [](float t) -> String {
            if (t <= 0.0f) return "#22c55e";
            int hue;
            if (t < 18.0f) {
                hue = 195;
            } else if (t <= 25.0f) {
                hue = (int)(195.0f - ((t - 18.0f) / 7.0f) * 60.0f);
            } else if (t <= 32.0f) {
                hue = (int)(135.0f - ((t - 25.0f) / 7.0f) * 50.0f);
            } else if (t <= 40.0f) {
                hue = (int)(85.0f - ((t - 32.0f) / 8.0f) * 50.0f);
            } else {
                hue = (t >= 48.0f) ? 0 : (int)(35.0f - ((t - 40.0f) / 8.0f) * 35.0f);
                if (hue < 0) hue = 0;
            }
            return "hsl(" + String(hue) + ",80%,44%)";
        };

        for (uint8_t c = 0; c < numCells; ++c) {
            uint16_t v = mod.cells[c].voltMv;
            float t = mod.cells[c].tempMdeg / 1000.0f;
            bool bal = mod.cells[c].balance;

            // Voltage fill percent (operating range: 3000mV to 3450mV)
            int fill = 0;
            if (v >= 3000) {
                fill = (int)(((v - 3000) / 450.0f) * 100.0f);
                if (fill > 100) fill = 100;
                if (fill < 8) fill = 8;
            }

            html += "      <div style='flex:1;min-width:44px;display:flex;flex-direction:column;align-items:center;'>\n";
            html += "        <div style='width:100%;height:180px;background:rgba(255,255,255,0.03);border:1.5px solid #475569;border-radius:4px;position:relative;overflow:hidden;display:flex;flex-direction:column;justify-content:flex-end;'>\n";
            
            // Dynamic fill bar (gradient from cell temperature color at top to turquoise brand base at bottom)
            String tempColor = getCellTempColor(t);
            html += "          <div id='cFill_" + String(c) + "' style='position:absolute;bottom:0;left:0;right:0;height:" + String(fill) + "%;background:linear-gradient(180deg," + tempColor + ",#00b3ba);opacity:0.88;transition:all 0.3s;'></div>\n";

            // Cell Data Labels
            html += "          <div style='position:relative;z-index:2;width:100%;text-align:center;color:#ffffff;font-size:0.72rem;font-weight:700;line-height:1.3;padding:6px 1px;display:flex;flex-direction:column;gap:3px;'>\n";
            String vCellStr = String(v / 1000.0f, 3);
            uint8_t cSoc = mod.cells[c].socPercent;
            if (cSoc == 0 && mod.power.valid && mod.power.socPercent > 0 && mod.cells[c].coulombMah == 0) {
                cSoc = mod.power.socPercent;
            }
            html += "            <div id='cSoc_" + String(c) + "' style='font-size:0.68rem;color:#ffffff;font-weight:700;'>" + (v > 0 ? (String(cSoc) + "%") : "--%") + "</div>\n";
            html += "            <div id='cVolt_" + String(c) + "'>" + (v > 0 ? (vCellStr + "V") : "--V") + "</div>\n";
            html += "            <div id='cTemp_" + String(c) + "' style='font-size:0.68rem;color:#cbd5e1;'>" + String(t, 1) + " °C</div>\n";

            // SOH per cell (supported on US3000C)
            bool isModelC = (stack.model == MODEL_US3000C || (stack.model != MODEL_US3000D && mod.cells[c].sohValid) || strstr(info.deviceName, "US3000C") != nullptr);
            if (isModelC) {
                if (mod.cells[c].sohValid) {
                    if (mod.cells[c].sohCount > 0) {
                        html += "            <div id='cSoh_" + String(c) + "' style='font-size:0.64rem;font-weight:700;color:#cbd5e1;' title='SOH Status: " + String(mod.cells[c].sohStatus) + "'>SOH: <b style='color:#ef4444;'>" + String(mod.cells[c].sohCount) + "</b></div>\n";
                    } else {
                        html += "            <div id='cSoh_" + String(c) + "' style='font-size:0.64rem;font-weight:700;color:#cbd5e1;' title='SOH Status: " + String(mod.cells[c].sohStatus) + "'>SOH: 0</div>\n";
                    }
                } else {
                    html += "            <div id='cSoh_" + String(c) + "' style='font-size:0.64rem;font-weight:700;color:#64748b;' title='SOH not yet polled'>SOH: -</div>\n";
                }
            }

            if (bal) {
                html += "            <span id='cBal_" + String(c) + "' style='background:#f59e0b;color:#000000;font-size:0.62rem;font-weight:800;border-radius:3px;padding:1px 3px;margin:2px auto;line-height:1;'>BAL</span>\n";
            } else {
                html += "            <span id='cBal_" + String(c) + "' style='visibility:hidden;font-size:0.62rem;font-weight:800;border-radius:3px;padding:1px 3px;margin:2px auto;line-height:1;'>BAL</span>\n";
            }
            html += "          </div>\n";

            html += "        </div>\n";
            html += "        <div style='font-size:0.75rem;font-weight:700;color:#94a3b8;margin-top:6px;'>" + String(c) + "</div>\n";
            html += "      </div>\n";
        }

        html += "    </div>\n";
        html += "  </div>\n"; // End inner & chassis
        html += "</div>\n"; // End 19\" Rack Battery Chassis Section

        // BMS Alarm & Protection Registers (Only rendered when BMS lifetime statistics are available)
        if (st.valid) {
            auto alarmRow = [&](const String &name, const String &desc, uint32_t count, bool isCritical = false) {
                String badge;
                if (count == 0) {
                    badge = "<span class='badge badge-ok'>Clear (0)</span>";
                } else {
                    String badgeClass = isCritical ? "badge badge-danger" : "badge badge-warn";
                    String label = (count == 1) ? "1 Event" : (String(count) + " Events");
                    badge = "<span class='" + badgeClass + "'>" + label + "</span>";
                }
                return "            <tr><td><b>" + name + "</b></td><td>" + desc + "</td><td style='text-align:right;'>" + badge + "</td></tr>\n";
            };

            html += "<div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;margin-bottom:22px;'>\n";

            // Table 1: Current & Hardware Protections
            html += "  <div class='table-card' style='display:flex;flex-direction:column;'>\n";
            html += "    <div class='table-header'><h3>⚡ Current & Hardware Protections</h3></div>\n";
            html += "    <div style='flex:1;overflow-x:auto;width:100%;'>\n";
            html += "      <table style='width:100%;min-width:100%;font-size:0.84rem;'>\n";
            html += "        <thead><tr><th style='width:24%;'>Event</th><th style='width:50%;'>Description</th><th style='width:26%;text-align:right;'>Status</th></tr></thead>\n";
            html += "        <tbody>\n";
            html += alarmRow("COC", "Charge Over-Current Cut-off", st.cocTimes);
            html += alarmRow("COCA", "Charge Over-Current Alarm", st.cocaTimes);
            html += alarmRow("DOC", "Discharge Over-Current Cut-off", st.docTimes);
            html += alarmRow("DOCA", "Discharge Over-Current Alarm", st.docaTimes);
            html += alarmRow("SC", "Short Circuit Protection", st.scTimes, true);
            html += alarmRow("RV", "Reverse Voltage Protection", st.rvTimes, true);
            html += alarmRow("Input OV", "Input Over-Voltage Protection", st.inputOvTimes);
            html += alarmRow("BMICERR", "BMS AFE/IC Hardware Fault", st.bmicErrTimes, true);
            html += alarmRow("LifeAlarm", "Lifetime Critical Alarms", st.lifeAlarmTimes, true);
            html += alarmRow("LifeWarn", "Lifetime System Warnings", st.lifeWarnTimes);
            html += alarmRow("Bat SLP", "Battery Sleep Transitions", st.batSlpTimes);
            html += alarmRow("Pwr SLP", "Power Bus Sleep Transitions", st.pwrSlpTimes);
            html += "        </tbody>\n";
            html += "      </table>\n";
            html += "    </div>\n";
            html += "  </div>\n";

            // Table 2: Voltage & Thermal Protections
            html += "  <div class='table-card' style='display:flex;flex-direction:column;'>\n";
            html += "    <div class='table-header'><h3>🌡️ Voltage & Thermal Protections</h3></div>\n";
            html += "    <div style='flex:1;overflow-x:auto;width:100%;'>\n";
            html += "      <table style='width:100%;min-width:100%;font-size:0.84rem;'>\n";
            html += "        <thead><tr><th style='width:24%;'>Event</th><th style='width:50%;'>Description</th><th style='width:26%;text-align:right;'>Status</th></tr></thead>\n";
            html += "        <tbody>\n";
            html += alarmRow("Bat OV", "Battery Over-Voltage Cut-off", st.batOvTimes);
            html += alarmRow("Bat HV", "Battery High-Voltage Warning", st.batHvTimes);
            html += alarmRow("Bat LV", "Battery Low-Voltage Warning", st.batLvTimes);
            html += alarmRow("Bat UV", "Battery Under-Voltage Cut-off", st.batUvTimes);
            html += alarmRow("Pwr OV", "Power Bus Over-Voltage Cut-off", st.pwrOvTimes);
            html += alarmRow("Pwr HV", "Power Bus High-Voltage Warning", st.pwrHvTimes);
            html += alarmRow("Pwr LV", "Power Bus Low-Voltage Warning", st.pwrLvTimes);
            html += alarmRow("Pwr UV", "Power Bus Under-Voltage Cut-off", st.pwrUvTimes);
            html += alarmRow("COT", "Charge Over-Temperature", st.cotTimes);
            html += alarmRow("CUT", "Charge Under-Temperature", st.cutTimes);
            html += alarmRow("DOT", "Discharge Over-Temperature", st.dotTimes);
            html += alarmRow("DUT", "Discharge Under-Temperature", st.dutTimes);
            html += "        </tbody>\n";
            html += "      </table>\n";
            html += "    </div>\n";
            html += "  </div>\n";
            html += "</div>\n"; // End dual grid
        }

        // Client-side Live Telemetry AJAX Polling
        bool apiAuthEn = prefs.getBool(NVS_KEY_API_AUTH_ENABLED, false);
        String apiTok = apiAuthEn ? prefs.getString(NVS_KEY_API_TOKEN, "") : "";

        html += "<script>\n";
        html += "(function(){\n";
        html += "  var modId = " + String(m) + ";\n";
        html += "  var apiTok = '" + apiTok + "';\n";
        html += "  function getCellTempColor(t){\n";
        html += "    if (t <= 0) return '#22c55e';\n";
        html += "    var hue = 195;\n";
        html += "    if (t < 18) hue = 195;\n";
        html += "    else if (t <= 25) hue = Math.round(195 - ((t - 18) / 7.0) * 60);\n";
        html += "    else if (t <= 32) hue = Math.round(135 - ((t - 25) / 7.0) * 50);\n";
        html += "    else if (t <= 40) hue = Math.round(85 - ((t - 32) / 8.0) * 50);\n";
        html += "    else hue = (t >= 48) ? 0 : Math.max(0, Math.round(35 - ((t - 40) / 8.0) * 35));\n";
        html += "    return 'hsl(' + hue + ',80%,44%)';\n";
        html += "  }\n";
        html += "  function updateLive(){\n";
        html += "    var url = '/api/module?m=' + modId + (apiTok ? '&token=' + encodeURIComponent(apiTok) : '');\n";
        html += "    fetch(url)\n";
        html += "      .then(function(r){ return r.json(); })\n";
        html += "      .then(function(d){\n";
        html += "        if (!d || !d.valid) return;\n";
        html += "        var el;\n";
        html += "        el = document.getElementById('modGaugeText'); if (el) el.textContent = d.soc + '%';\n";
        html += "        el = document.getElementById('modGaugeCircle'); if (el) el.setAttribute('stroke-dashoffset', (263.89 - (263.89 * d.soc / 100.0)).toFixed(1));\n";
        html += "        el = document.getElementById('modVolt'); if (el) el.textContent = d.volt.toFixed(2) + ' V';\n";
        html += "        el = document.getElementById('modCurr'); if (el) { el.textContent = d.curr.toFixed(2) + ' A'; el.style.color = (d.curr > 0.05) ? '#16a34a' : ((d.curr < -0.05) ? '#dc2626' : 'var(--navy)'); }\n";
        html += "        el = document.getElementById('modPower'); if (el) { el.textContent = d.power.toFixed(1) + ' W'; el.style.color = (d.power > 1.0) ? '#16a34a' : ((d.power < -1.0) ? '#dc2626' : 'var(--navy)'); }\n";
        html += "        el = document.getElementById('modSpread'); if (el) el.textContent = d.v_spread + ' mV';\n";
        html += "        el = document.getElementById('modPackTemp'); if (el) el.textContent = d.pack_temp.toFixed(1) + ' °C';\n";
        html += "        el = document.getElementById('modMosTemp'); if (el) el.textContent = (d.mos_temp !== null) ? (d.mos_temp.toFixed(1) + ' °C') : 'N/A';\n";
        html += "        el = document.getElementById('modMinTemp'); if (el) el.textContent = d.min_temp.toFixed(1) + ' °C';\n";
        html += "        el = document.getElementById('modMaxTemp'); if (el) el.textContent = d.max_temp.toFixed(1) + ' °C';\n";
        html += "        el = document.getElementById('modDeltaTemp'); if (el) el.textContent = d.temp_delta.toFixed(1) + ' °C';\n";
        html += "        el = document.getElementById('modBaseState'); if (el) el.textContent = d.base_state;\n";
        html += "        var fi = document.getElementById('footerScrapeInfo');\n";
        html += "        if (fi && d.scrape_duration_ms !== undefined) {\n";
        html += "          var dur = (d.scrape_duration_ms / 1000.0).toFixed(2);\n";
        html += "          var st = d.scrape_success ? \"<span style='color:#16a34a;font-weight:700;'>OK</span>\" : \"<span style='color:#dc2626;font-weight:700;'>FAIL</span>\";\n";
        html += "          fi.innerHTML = 'Last BMS Scrape: <b>' + dur + 's</b> (' + st + ')';\n";
        html += "        }\n";
        html += "        if (d.cells) {\n";
        html += "          d.cells.forEach(function(c, i){\n";
        html += "            var s = document.getElementById('cSoc_' + i); if (s) s.textContent = (c.soc ? c.soc : d.soc) + '%';\n";
        html += "            var v = document.getElementById('cVolt_' + i); if (v) v.textContent = (c.v / 1000.0).toFixed(3) + 'V';\n";
        html += "            var t = document.getElementById('cTemp_' + i); if (t) t.textContent = c.t.toFixed(1) + ' °C';\n";
        html += "            var b = document.getElementById('cBal_' + i); if (b) b.style.visibility = c.bal ? 'visible' : 'hidden';\n";
        html += "            var f = document.getElementById('cFill_' + i);\n";
        html += "            if (f) {\n";
        html += "              var fill = 0; if (c.v >= 3000) fill = Math.min(100, Math.max(8, ((c.v - 3000) / 450.0) * 100));\n";
        html += "              f.style.height = fill + '%';\n";
        html += "              f.style.background = 'linear-gradient(180deg,' + getCellTempColor(c.t) + ',#00b3ba)';\n";
        html += "            }\n";
        html += "            var soh = document.getElementById('cSoh_' + i);\n";
        html += "            if (soh && c.soh_valid) {\n";
        html += "              soh.title = 'SOH Status: ' + c.soh_status;\n";
        html += "              soh.innerHTML = (c.soh_count > 0) ? (\"SOH: <b style='color:#ef4444;'>\" + c.soh_count + \"</b>\") : \"SOH: 0\";\n";
        html += "            }\n";
        html += "          });\n";
        html += "        }\n";
        html += "      }).catch(function(e){});\n";
        html += "  }\n";
        html += "  setInterval(updateLive, 5000);\n";
        html += "})();\n";
        html += "</script>\n";

        html += renderFooter();
        html.finish();
    }

    void handleRefreshModule() {
        uint8_t m = 1;
        if (server.hasArg("m")) {
            m = server.arg("m").toInt();
        }
        if (m < 1 || m > MAX_MODULES) m = 1;

        pylonSerial.pollModuleOnDemand(stack, m);

        server.sendHeader("Location", "/module?m=" + String(m));
        server.send(303);
    }

    // =========================================================================
    // 3. Settings Page (`/settings`)
    // =========================================================================
    void handleSettingsPage() {
        ChunkedHtmlSender html(server);
        html += renderHeader("Settings", "settings");

        bool authEn = prefs.getBool(NVS_KEY_AUTH_ENABLED, false);
        String authUser = prefs.getString(NVS_KEY_AUTH_USER, "admin");
        String authPass = prefs.getString(NVS_KEY_AUTH_PASS, "");

        bool apiEn = prefs.getBool(NVS_KEY_API_AUTH_ENABLED, false);
        String apiTok = prefs.getString(NVS_KEY_API_TOKEN, "");
        if (apiTok.length() == 0) {
            apiTok = "psm_" + String((uint32_t)ESP.getEfuseMac(), HEX) + String(millis(), HEX);
        }

        uint32_t fastSec = prefs.getUInt(NVS_KEY_FAST_POLL_SEC, 60);
        uint32_t slowSec = prefs.getUInt(NVS_KEY_SLOW_POLL_SEC, 300);

        bool mqEn = prefs.getBool(NVS_KEY_MQTT_ENABLED, false);
        String mqSrv = prefs.getString(NVS_KEY_MQTT_SERVER, "");
        uint16_t mqPrt = prefs.getUShort(NVS_KEY_MQTT_PORT, 1883);
        String mqUsr = prefs.getString(NVS_KEY_MQTT_USER, "");
        String mqPwd = prefs.getString(NVS_KEY_MQTT_PASS, "");
        String mqPfx = prefs.getString(NVS_KEY_MQTT_PREFIX, "homeassistant/sensor/pylontech");

        bool staticEn = prefs.getBool(NVS_KEY_STATIC_IP_EN, false);
        String staticIp = prefs.getString(NVS_KEY_STATIC_IP, isApMode ? "192.168.4.1" : WiFi.localIP().toString());
        String staticMask = prefs.getString(NVS_KEY_STATIC_MASK, isApMode ? "255.255.255.0" : WiFi.subnetMask().toString());
        String staticGw = prefs.getString(NVS_KEY_STATIC_GW, isApMode ? "192.168.4.1" : WiFi.gatewayIP().toString());
        String staticDns = prefs.getString(NVS_KEY_STATIC_DNS, isApMode ? "192.168.4.1" : WiFi.dnsIP().toString());

        bool ntpEn = prefs.getBool(NVS_KEY_NTP_ENABLED, true);
        String ntpSrv = prefs.getString(NVS_KEY_NTP_SERVER, NTP_DEFAULT_SERVER);
        String savedCity = prefs.getString(NVS_KEY_TZ_CITY, DEFAULT_TZ_CITY);
        bool timeFormat24h = prefs.getBool(NVS_KEY_TIME_FORMAT_24H, true);

        // System Management Card (Placed at the top above parameters)
        html += "<div class='table-card' style='margin-bottom:20px;padding:20px;'>\n";
        html += "  <h3 style='margin:0 0 14px 0;color:var(--navy);font-size:1.05rem;'>⚙️ System Actions</h3>\n";
        html += "  <div style='display:flex;gap:12px;flex-wrap:wrap;'>\n";
        html += "    <a href='/wifi' class='btn btn-outline'>📶 Reconfigure WiFi</a>\n";
        html += "    <a href='/update' class='btn btn-outline'>🚀 Firmware Update (OTA)</a>\n";
        html += "    <a href='/restart' onclick=\"return confirm('Restart ESP32?');\" class='btn btn-outline'>🔄 Restart Device</a>\n";
        html += "    <a href='/reset_wifi' onclick=\"return confirm('Factory reset WiFi settings? Device will start in AP mode.');\" class='btn btn-outline'>⚠️ Factory Reset WiFi</a>\n";
        html += "  </div>\n";
        html += "</div>\n";

        html += "<form method='POST' action='/save_settings'>\n";

        // Network & Device Identity Card
        String devHost = prefs.getString(NVS_KEY_HOSTNAME, "");
        String defaultHost = getDefaultHostname();

        html += "<div class='table-card' style='margin-bottom:20px;padding:20px;'>\n";
        html += "  <h3 style='margin:0 0 14px 0;color:var(--navy);font-size:1.05rem;'>🌐 Network & Device Identity</h3>\n";
        html += "  <div style='margin-bottom:16px;max-width:440px;'>\n";
        html += "    <label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Device Hostname (mDNS / OTA / DHCP):</label>\n";
        html += "    <input type='text' name='dev_host' value='" + devHost + "' placeholder='" + defaultHost + "' maxlength='32' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;font-family:monospace;'>\n";
        html += "    <small style='color:#718096;display:block;margin-top:4px;'>Local access URL: <code>http://" + (devHost.length() > 0 ? devHost : defaultHost) + ".local/</code>. Leave empty to use auto-generated default (<code>" + defaultHost + "</code>).</small>\n";
        html += "  </div>\n";
        html += "  <label style='display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;margin-bottom:12px;'>\n";
        html += "    <input type='checkbox' name='ip_static' value='1'" + String(staticEn ? " checked" : "") + "> Use Static IP Configuration (instead of DHCP)\n";
        html += "  </label>\n";
        html += "  <div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;'>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Static IP Address:</label><input type='text' name='ip_addr' value='" + staticIp + "' placeholder='192.168.5.134' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Subnet Mask:</label><input type='text' name='ip_mask' value='" + staticMask + "' placeholder='255.255.255.0' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Default Gateway:</label><input type='text' name='ip_gw' value='" + staticGw + "' placeholder='192.168.5.1' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Primary DNS Server:</label><input type='text' name='ip_dns' value='" + staticDns + "' placeholder='192.168.5.1' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "  </div>\n";
        html += "  <small style='color:#718096;display:block;margin-top:8px;'>When unchecked, device dynamically receives network parameters via DHCP from your router.</small>\n";
        html += "</div>\n";

        // Time Synchronization & Timezone Card
        bool isSynced = (time(nullptr) > 1577836800);
        String currentTimeStr = "Waiting for sync (RTC uncalibrated)";
        String lastSyncStr = "Never / Just booted";
        time_t nowT = time(nullptr);
        if (isSynced) {
            struct tm ti;
            localtime_r(&nowT, &ti);
            char tBuf[40];
            if (timeFormat24h) {
                strftime(tBuf, sizeof(tBuf), "%Y-%m-%d %H:%M:%S", &ti);
            } else {
                strftime(tBuf, sizeof(tBuf), "%Y-%m-%d %I:%M:%S %p", &ti);
            }
            currentTimeStr = String(tBuf);

            time_t syncT = (lastNtpSyncTimestamp > 0) ? lastNtpSyncTimestamp : nowT;
            struct tm sTi;
            localtime_r(&syncT, &sTi);
            char sBuf[40];
            if (timeFormat24h) {
                strftime(sBuf, sizeof(sBuf), "%Y-%m-%d %H:%M:%S", &sTi);
            } else {
                strftime(sBuf, sizeof(sBuf), "%Y-%m-%d %I:%M:%S %p", &sTi);
            }
            long diffSec = (long)(nowT - syncT);
            if (diffSec < 60) {
                lastSyncStr = String(sBuf) + " (" + String(diffSec) + "s ago)";
            } else {
                lastSyncStr = String(sBuf) + " (" + String(diffSec / 60) + "m ago)";
            }
        }

        html += "<div class='table-card' style='margin-bottom:20px;padding:20px;'>\n";
        html += "  <div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:14px;flex-wrap:wrap;gap:10px;'>\n";
        html += "    <h3 style='margin:0;color:var(--navy);font-size:1.05rem;'>🕒 Time Synchronization & Timezone</h3>\n";
        if (isSynced) {
            html += "    <span class='badge badge-ok'>Synced (OK)</span>\n";
        } else {
            html += "    <span class='badge badge-warn'>Waiting for sync</span>\n";
        }
        html += "  </div>\n";

        // Real-time status banner
        html += "  <div style='background:var(--bg);border:1px solid var(--border);border-radius:8px;padding:12px 16px;margin-bottom:16px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px;'>\n";
        html += "    <div>\n";
        html += "      <div style='font-size:0.88rem;color:var(--text);font-weight:600;'>Current Device Time: <span style='color:var(--navy);font-family:monospace;font-size:0.95rem;font-weight:700;margin-left:4px;'>" + currentTimeStr + "</span></div>\n";
        html += "      <div style='font-size:0.80rem;color:#718096;margin-top:4px;'>Last Sync: <b>" + lastSyncStr + "</b> &bull; Sync Interval: <b>Every 1 hour (3600s)</b></div>\n";
        html += "    </div>\n";
        html += "    <div><a href='/sync_ntp?redirect=/settings' class='btn btn-outline' style='padding:5px 12px;font-size:0.80rem;'>🔄 Sync Time Now</a></div>\n";
        html += "  </div>\n";

        html += "  <label style='display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;margin-bottom:12px;'>\n";
        html += "    <input type='checkbox' name='ntp_en' value='1'" + String(ntpEn ? " checked" : "") + "> Enable NTP Network Time Protocol\n";
        html += "  </label>\n";
        html += "  <div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;'>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>NTP Server:</label><input type='text' name='ntp_srv' value='" + ntpSrv + "' placeholder='pool.ntp.org' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>City & Timezone:</label>\n";
        html += "      <select name='tz_idx' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;background:var(--card);color:var(--text);'>\n";
        for (size_t i = 0; i < TIMEZONE_COUNT; ++i) {
            bool sel = (savedCity == TIMEZONE_LIST[i].city);
            html += "        <option value='" + String(i) + "'" + (sel ? " selected" : "") + ">" + String(TIMEZONE_LIST[i].city) + "</option>\n";
        }
        html += "      </select>\n";
        html += "    </div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Time Format:</label>\n";
        html += "      <select name='time_fmt' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;background:var(--card);color:var(--text);'>\n";
        html += "        <option value='24'" + String(timeFormat24h ? " selected" : "") + ">24 Hours (e.g. 20:15:00)</option>\n";
        html += "        <option value='12'" + String(!timeFormat24h ? " selected" : "") + ">12 Hours (e.g. 8:15:00 PM)</option>\n";
        html += "      </select>\n";
        html += "    </div>\n";
        html += "  </div>\n";
        html += "  <small style='color:#718096;display:block;margin-top:8px;'>Device time adjusts automatically for Daylight Saving Time (DST). Background daemon resynchronizes clock every 1 hour to correct hardware timer drift.</small>\n";
        html += "</div>\n";

        // Security Card
        html += "<div class='table-card' style='margin-bottom:20px;padding:20px;'>\n";
        html += "  <h3 style='margin:0 0 14px 0;color:var(--navy);font-size:1.05rem;'>🔒 Web & API Security</h3>\n";
        html += "  <div style='margin-bottom:16px;'>\n";
        html += "    <label style='display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;'>\n";
        html += "      <input type='checkbox' name='auth_en' value='1'" + String(authEn ? " checked" : "") + "> Enable HTTP Basic Authentication for Web UI\n";
        html += "    </label>\n";
        html += "    <div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;margin-top:10px;'>\n";
        html += "      <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Admin Username:</label><input type='text' name='auth_usr' placeholder='Username' value='" + authUser + "' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "      <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Admin Password:</label><input type='password' name='auth_pwd' placeholder='Leave blank to keep' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    </div>\n";
        html += "  </div>\n";
        html += "  <hr style='border:0;border-top:1px solid var(--border);margin:16px 0;'>\n";
        html += "  <div>\n";
        html += "    <label style='display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;'>\n";
        html += "      <input type='checkbox' name='api_en' value='1'" + String(apiEn ? " checked" : "") + "> Require Bearer Token for REST API (<code>/api/data</code>)\n";
        html += "    </label>\n";
        html += "    <div style='display:flex;gap:10px;margin-top:10px;flex-wrap:wrap;'>\n";
        html += "      <input type='text' id='api_tok_field' name='api_tok' value='" + apiTok + "' style='font-family:monospace;padding:8px 12px;border:1px solid #cbd5e1;border-radius:6px;flex:1;min-width:140px;max-width:100%;'>\n";
        html += "      <button type='button' onclick='genToken()' class='btn btn-outline'>🔑 Generate New Token</button>\n";
        html += "    </div>\n";
        html += "    <small style='color:#718096;display:block;margin-top:5px;'>External systems authenticate with header: <code>Authorization: Bearer &lt;token&gt;</code></small>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // Polling Cadence Card
        html += "<div class='table-card' style='margin-bottom:20px;padding:20px;'>\n";
        html += "  <h3 style='margin:0 0 14px 0;color:var(--navy);font-size:1.05rem;'>⏱️ Polling Intervals</h3>\n";
        html += "  <div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;'>\n";
        html += "    <div><label style='font-size:0.85rem;font-weight:600;display:block;margin-bottom:4px;'>Fast Poll Interval (Seconds):</label><input type='number' min='15' max='300' name='fast_sec' value='" + String(fastSec) + "' style='width:100%;padding:8px 12px;border:1px solid #cbd5e1;border-radius:6px;'><small style='color:#718096;display:block;margin-top:4px;'>Queries pwr + bat (cell voltages, balancing)</small></div>\n";
        html += "    <div><label style='font-size:0.85rem;font-weight:600;display:block;margin-bottom:4px;'>Slow Poll Interval (Seconds):</label><input type='number' min='30' max='1800' name='slow_sec' value='" + String(slowSec) + "' style='width:100%;padding:8px 12px;border:1px solid #cbd5e1;border-radius:6px;'><small style='color:#718096;display:block;margin-top:4px;'>Queries stat + info + soh/euro (alarms, cycles)</small></div>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // Home Assistant MQTT Card
        html += "<div class='table-card' style='margin-bottom:20px;padding:20px;'>\n";
        html += "  <div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:14px;'>\n";
        html += "    <h3 style='margin:0;color:var(--navy);font-size:1.05rem;'>🏠 Home Assistant MQTT Integration</h3>\n";
        html += "    <span class='badge " + String(mqttClient.isConnected() ? "badge-ok'>Connected" : "badge-warn'>Disconnected") + "</span>\n";
        html += "  </div>\n";
        html += "  <label style='display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;margin-bottom:14px;'>\n";
        html += "    <input type='checkbox' name='mq_en' value='1'" + String(mqEn ? " checked" : "") + "> Enable MQTT Client & Auto-Discovery\n";
        html += "  </label>\n";
        html += "  <div style='display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px;'>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Broker IP / Hostname:</label><input type='text' name='mq_srv' value='" + mqSrv + "' placeholder='192.168.1.50' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Broker Port:</label><input type='number' name='mq_prt' value='" + String(mqPrt) + "' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Username (Optional):</label><input type='text' name='mq_usr' value='" + mqUsr + "' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "    <div><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Password (Optional):</label><input type='password' name='mq_pwd' placeholder='Leave blank to keep' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "  </div>\n";
        html += "  <div style='margin-top:14px;'><label style='font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;'>Topic Prefix:</label><input type='text' name='mq_pfx' value='" + mqPfx + "' style='width:100%;padding:8px 10px;border:1px solid #cbd5e1;border-radius:6px;'></div>\n";
        html += "</div>\n";

        // Save Button
        html += "<div style='margin-bottom:24px;'><button type='submit' class='btn btn-primary' style='padding:10px 24px;font-size:0.95rem;'>💾 Save Settings</button></div>\n";
        html += "</form>\n";

        html += "<script>\n";
        html += "function genToken(){let r='';const c='abcdef0123456789';for(let i=0;i<32;i++)r+=c.charAt(Math.floor(Math.random()*c.length));document.getElementById('api_tok_field').value='psm_'+r;}\n";
        html += "</script>\n";

        html += renderFooter();
        html.finish();
    }

    void handleSaveSettings() {
        if (server.hasArg("dev_host")) {
            String rawHost = server.arg("dev_host");
            rawHost.trim();
            rawHost.toLowerCase();
            String cleanHost = "";
            for (size_t i = 0; i < rawHost.length(); ++i) {
                char c = rawHost[i];
                if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
                    cleanHost += c;
                }
            }
            if (cleanHost.length() > 32) cleanHost = cleanHost.substring(0, 32);
            prefs.putString(NVS_KEY_HOSTNAME, cleanHost);
            String effectiveHost = (cleanHost.length() > 0) ? cleanHost : getDefaultHostname();
            peerDiscovery.setSelfHostname(effectiveHost);
            if (WiFi.status() == WL_CONNECTED) {
                MDNS.end();
                MDNS.begin(effectiveHost.c_str());
                MDNS.addService("http", "tcp", 80);
                MDNS.addService("pylon-smart", "tcp", 80);
                MDNS.addServiceTxt("pylon-smart", "tcp", "ver", FIRMWARE_VERSION);
                if (stack.modelName[0] != '\0' && strcmp(stack.modelName, "Unknown") != 0) {
                    MDNS.addServiceTxt("pylon-smart", "tcp", "model", (const char*)stack.modelName);
                }
                ArduinoOTA.setHostname(effectiveHost.c_str());
            }
        }

        bool staticEn = server.hasArg("ip_static");
        prefs.putBool(NVS_KEY_STATIC_IP_EN, staticEn);
        if (server.hasArg("ip_addr")) prefs.putString(NVS_KEY_STATIC_IP, server.arg("ip_addr"));
        if (server.hasArg("ip_mask")) prefs.putString(NVS_KEY_STATIC_MASK, server.arg("ip_mask"));
        if (server.hasArg("ip_gw")) prefs.putString(NVS_KEY_STATIC_GW, server.arg("ip_gw"));
        if (server.hasArg("ip_dns")) prefs.putString(NVS_KEY_STATIC_DNS, server.arg("ip_dns"));

        bool ntpEn = server.hasArg("ntp_en");
        prefs.putBool(NVS_KEY_NTP_ENABLED, ntpEn);
        if (server.hasArg("ntp_srv")) prefs.putString(NVS_KEY_NTP_SERVER, server.arg("ntp_srv"));

        if (server.hasArg("tz_idx")) {
            int idx = server.arg("tz_idx").toInt();
            if (idx >= 0 && (size_t)idx < TIMEZONE_COUNT) {
                prefs.putString(NVS_KEY_TZ_CITY, TIMEZONE_LIST[idx].city);
                prefs.putString(NVS_KEY_TZ_POSIX, TIMEZONE_LIST[idx].posix);
            }
        }

        if (server.hasArg("time_fmt")) {
            bool is24 = (server.arg("time_fmt") != "12");
            prefs.putBool(NVS_KEY_TIME_FORMAT_24H, is24);
        }

        triggerNtpSync();

        bool authEn = server.hasArg("auth_en");
        prefs.putBool(NVS_KEY_AUTH_ENABLED, authEn);
        if (server.hasArg("auth_usr")) {
            prefs.putString(NVS_KEY_AUTH_USER, server.arg("auth_usr"));
        }
        if (server.hasArg("auth_pwd") && server.arg("auth_pwd").length() > 0) {
            prefs.putString(NVS_KEY_AUTH_PASS, server.arg("auth_pwd"));
        }

        bool apiEn = server.hasArg("api_en");
        prefs.putBool(NVS_KEY_API_AUTH_ENABLED, apiEn);
        if (server.hasArg("api_tok") && server.arg("api_tok").length() > 0) {
            prefs.putString(NVS_KEY_API_TOKEN, server.arg("api_tok"));
        }

        if (server.hasArg("fast_sec")) {
            uint32_t fs = server.arg("fast_sec").toInt();
            if (fs >= 15 && fs <= 300) prefs.putUInt(NVS_KEY_FAST_POLL_SEC, fs);
        }
        if (server.hasArg("slow_sec")) {
            uint32_t ss = server.arg("slow_sec").toInt();
            if (ss >= 30 && ss <= 1800) prefs.putUInt(NVS_KEY_SLOW_POLL_SEC, ss);
        }

        bool mqEn = server.hasArg("mq_en");
        prefs.putBool(NVS_KEY_MQTT_ENABLED, mqEn);
        if (server.hasArg("mq_srv")) prefs.putString(NVS_KEY_MQTT_SERVER, server.arg("mq_srv"));
        if (server.hasArg("mq_prt")) prefs.putUShort(NVS_KEY_MQTT_PORT, (uint16_t)server.arg("mq_prt").toInt());
        if (server.hasArg("mq_usr")) prefs.putString(NVS_KEY_MQTT_USER, server.arg("mq_usr"));
        if (server.hasArg("mq_pwd") && server.arg("mq_pwd").length() > 0) {
            prefs.putString(NVS_KEY_MQTT_PASS, server.arg("mq_pwd"));
        }
        if (server.hasArg("mq_pfx")) prefs.putString(NVS_KEY_MQTT_PREFIX, server.arg("mq_pfx"));

        mqttClient.loadConfig();

        server.sendHeader("Location", "/settings");
        server.send(303);
    }

    // =========================================================================
    // 3a. Discovered Network Peers API (`/api/peers`)
    // =========================================================================
    void handleApiPeers() {
        auto peers = peerDiscovery.getPeers();
        String json;
        json.reserve(128 * (peers.size() + 1));
        json = "[";
        for (size_t i = 0; i < peers.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"name\":\"" + peers[i].hostname + "\",";
            json += "\"ip\":\"" + peers[i].ip.toString() + "\",";
            json += "\"port\":" + String(peers[i].port) + ",";
            json += "\"model\":\"" + peers[i].model + "\",";
            json += "\"ver\":\"" + peers[i].version + "\"}";
        }
        json += "]";
        server.send(200, "application/json", json);
    }

    // =========================================================================
    // 3b. Live Module Telemetry API (`/api/module?m=N`)
    // =========================================================================
    void handleApiModuleData() {
        uint8_t m = 1;
        if (server.hasArg("m")) {
            m = server.arg("m").toInt();
        }
        if (m < 1 || m > MAX_MODULES) m = 1;

        const BatteryModule &mod = stack.modules[m];
        const ModulePower &p = mod.power;

        float modVolt = p.voltMv / 1000.0f;
        float modCurr = p.currMa / 1000.0f;
        float modPower = modVolt * modCurr;
        int soc = p.socPercent;

        int vSpread = (p.voltHighMv > p.voltLowMv) ? (p.voltHighMv - p.voltLowMv) : 0;
        uint16_t minCellV = 65535, maxCellV = 0;
        float minCellT = 100.0f, maxCellT = -40.0f;

        uint8_t numCells = (mod.cellCountParsed > 0) ? mod.cellCountParsed : 15;
        for (uint8_t c = 0; c < numCells; ++c) {
            uint16_t v = mod.cells[c].voltMv;
            if (v > 0) {
                if (v < minCellV) minCellV = v;
                if (v > maxCellV) maxCellV = v;
            }
            float t = mod.cells[c].tempMdeg / 1000.0f;
            if (t > -30.0f) {
                if (t < minCellT) minCellT = t;
                if (t > maxCellT) maxCellT = t;
            }
        }
        if (minCellV == 65535) minCellV = 0;
        if (vSpread == 0 && maxCellV >= minCellV && minCellV > 0) vSpread = maxCellV - minCellV;
        if (minCellT > maxCellT) { minCellT = p.tempMdeg / 1000.0f; maxCellT = p.tempMdeg / 1000.0f; }

        ChunkedResponseSender json(server, "application/json; charset=utf-8", 512);
        json += "{\"m\":"; json += String(m);
        json += ",\"valid\":"; json += (p.valid ? "true" : "false");
        json += ",\"soc\":"; json += String(soc);
        json += ",\"volt\":"; json += String(modVolt, 2);
        json += ",\"curr\":"; json += String(modCurr, 2);
        json += ",\"power\":"; json += String(modPower, 1);
        json += ",\"v_spread\":"; json += String(vSpread);
        json += ",\"pack_temp\":"; json += String(p.tempMdeg / 1000.0f, 1);
        json += ",\"mos_temp\":"; json += (p.mosTempMdeg > 0 ? String(p.mosTempMdeg / 1000.0f, 1) : "null");
        json += ",\"min_temp\":"; json += String(minCellT, 1);
        json += ",\"max_temp\":"; json += String(maxCellT, 1);
        json += ",\"temp_delta\":"; json += String(maxCellT - minCellT, 1);
        json += ",\"base_state\":\""; json += p.baseState; json += "\"";
        json += ",\"scrape_success\":"; json += (stack.scrapeSuccess ? "true" : "false");
        json += ",\"scrape_duration_ms\":"; json += String(stack.scrapeDurationMs);
        json += ",\"cells\":[";
        for (uint8_t c = 0; c < numCells; ++c) {
            if (c > 0) json += ",";
            uint8_t cSoc = mod.cells[c].socPercent;
            if (cSoc == 0 && p.valid && p.socPercent > 0 && mod.cells[c].coulombMah == 0) {
                cSoc = p.socPercent;
            }
            json += "{\"v\":"; json += String(mod.cells[c].voltMv);
            json += ",\"soc\":"; json += String(cSoc);
            json += ",\"t\":"; json += String(mod.cells[c].tempMdeg / 1000.0f, 1);
            json += ",\"bal\":"; json += (mod.cells[c].balance ? "true" : "false");
            json += ",\"soh_valid\":"; json += (mod.cells[c].sohValid ? "true" : "false");
            json += ",\"soh_count\":"; json += String(mod.cells[c].sohCount);
            json += ",\"soh_status\":\""; json += mod.cells[c].sohStatus; json += "\"}";
        }
        json += "]}";
        json.finish();
    }

    // =========================================================================
    // 4. REST API Endpoint (`/api/data`)
    // =========================================================================
    void handleApiData() {
        StackAnalytics a = calculateStackAnalytics();
        bool brief = server.hasArg("brief");

        ChunkedResponseSender json(server, "application/json; charset=utf-8", 1024);

        json += "{\"stack\":{";
        json += "\"model\":\""; json += stack.modelName; json += "\",";
        json += "\"modules_detected\":"; json += String(stack.moduleCount); json += ",";
        json += "\"voltage\":"; json += String(a.stackVolt, 2); json += ",";
        json += "\"current\":"; json += String(a.stackCurr, 2); json += ",";
        json += "\"power\":"; json += String(a.stackPower, 1); json += ",";
        json += "\"soc\":"; json += String((int)round(a.avgSoc)); json += ",";
        json += "\"soh\":"; json += (a.activeSoh >= 0 ? String(a.activeSoh) : "null"); json += ",";
        json += "\"scrape_success\":"; json += (stack.scrapeSuccess ? "true" : "false"); json += ",";
        json += "\"scrape_duration_ms\":"; json += String(stack.scrapeDurationMs); json += ",";
        json += "\"last_scrape\":"; json += String(stack.lastScrapeTimestamp); json += ",";
        json += "\"lowest_cell_v\":"; json += String(a.lowestCellV / 1000.0f, 3); json += ",";
        json += "\"lowest_cell_mod\":"; json += String(a.lowestMod); json += ",";
        json += "\"lowest_cell_idx\":"; json += String(a.lowestCellIdx); json += ",";
        json += "\"highest_cell_v\":"; json += String(a.highestCellV / 1000.0f, 3); json += ",";
        json += "\"highest_cell_mod\":"; json += String(a.highestMod); json += ",";
        json += "\"highest_cell_idx\":"; json += String(a.highestCellIdx); json += ",";
        json += "\"spread_mv\":"; json += String(a.spreadMv); json += ",";
        json += "\"avg_cell_v\":"; json += String((a.avgCellV > 0) ? (a.avgCellV / 1000.0) : 0.0, 3); json += ",";
        json += "\"std_dev_mv\":"; json += String(a.stdDev, 1); json += ",";
        json += "\"found_cell_t\":"; json += (a.foundCellT ? "true" : "false"); json += ",";
        json += "\"min_cell_t\":"; json += String(a.minCellT, 1); json += ",";
        json += "\"min_cell_t_mod\":"; json += String(a.minCellTMod); json += ",";
        json += "\"min_cell_t_idx\":"; json += String(a.minCellTIdx); json += ",";
        json += "\"max_cell_t\":"; json += String(a.maxCellT, 1); json += ",";
        json += "\"max_cell_t_mod\":"; json += String(a.maxCellTMod); json += ",";
        json += "\"max_cell_t_idx\":"; json += String(a.maxCellTIdx);
        json += "},";

        json += "\"modules\":[";
        bool firstMod = true;
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present) continue;

            if (!firstMod) json += ",";
            firstMod = false;

            String devName;
            if (strlen(mod.info.deviceName) > 0) {
                devName = mod.info.deviceName;
            } else if (stack.model != MODEL_UNKNOWN && m == stack.activeModuleIndex) {
                devName = stack.modelName;
            } else {
                devName = "Device " + String(m);
            }

            int vSpread = (mod.power.voltHighMv > mod.power.voltLowMv) ? (mod.power.voltHighMv - mod.power.voltLowMv) : 0;
            if (vSpread == 0 && mod.cellCountParsed > 0) {
                uint16_t vMin = 65535, vMax = 0;
                for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                    if (mod.cells[c].voltMv > 0) {
                        if (mod.cells[c].voltMv < vMin) vMin = mod.cells[c].voltMv;
                        if (mod.cells[c].voltMv > vMax) vMax = mod.cells[c].voltMv;
                    }
                }
                if (vMax >= vMin && vMin > 0) vSpread = vMax - vMin;
            }

            int mSoh = (mod.stats.sohPercent > 0) ? mod.stats.sohPercent : getEffectiveSoh(mod.stats);

            json += "{\"id\":"; json += String(m);
            json += ",\"device\":\""; json += devName; json += "\"";
            json += ",\"voltage\":"; json += String(mod.power.voltMv / 1000.0f, 3);
            json += ",\"current\":"; json += String(mod.power.currMa / 1000.0f, 2);
            json += ",\"soc\":"; json += String(mod.power.socPercent);
            json += ",\"soh\":"; json += (mSoh > 0 ? String(mSoh) : (strlen(mod.power.sohState) > 0 && strcmp(mod.power.sohState, "Normal") == 0 ? "100" : "null"));
            json += ",\"soh_times\":"; json += (mod.stats.valid ? String(mod.stats.sohTimes) : "null");
            json += ",\"volt_spread_mv\":"; json += String(vSpread);
            json += ",\"temp_c\":"; json += String(mod.power.tempMdeg / 1000.0f, 1);
            json += ",\"mos_temp_c\":"; json += (mod.power.mosTempMdeg > 0 ? String(mod.power.mosTempMdeg / 1000.0f, 1) : "null");
            json += ",\"base_state\":\""; json += mod.power.baseState; json += "\"";
            json += ",\"barcode\":\""; json += mod.info.barcode; json += "\"";
            json += ",\"release_date\":\""; json += mod.info.releaseDate; json += "\"";

            if (!brief) {
                json += ",\"cells\":[";
                uint8_t numCells = (mod.cellCountParsed > 0) ? mod.cellCountParsed : 15;
                for (uint8_t c = 0; c < numCells; ++c) {
                    if (c > 0) json += ",";
                    json += "{\"cell\":"; json += String(c);
                    json += ",\"voltage_mv\":"; json += String(mod.cells[c].voltMv);
                    json += ",\"soc\":"; json += String(mod.cells[c].socPercent);
                    json += ",\"temp_c\":"; json += String(mod.cells[c].tempMdeg / 1000.0f, 1);
                    json += ",\"balance\":"; json += (mod.cells[c].balance ? "true" : "false");
                    if (mod.cells[c].sohValid) {
                        json += ",\"soh_count\":"; json += String(mod.cells[c].sohCount);
                        json += ",\"soh_status\":\""; json += mod.cells[c].sohStatus; json += "\"";
                    }
                    json += "}";
                }
                json += "]";
            }
            json += "}";
        }
        json += "]";

        SystemStats sys = SystemMonitor::getSnapshot();
        json += ",\"system\":{";
        json += "\"uptime_sec\":"; json += String(sys.uptimeSec);
        json += ",\"cpu_usage_pct\":"; json += String(sys.cpuLoadPct, 1);
        json += ",\"cpu_freq_mhz\":"; json += String(sys.cpuFreqMHz);
        json += ",\"cpu_temp_c\":"; json += String(sys.cpuTempC, 1);
        json += ",\"cpu_cores\":"; json += String(sys.cpuCores);
        json += ",\"chip_model\":\""; json += sys.chipModel; json += "\"";
        json += ",\"chip_revision\":"; json += String(sys.chipRevision);
        json += ",\"reset_reason\":\""; json += sys.resetReason; json += "\"";
        json += ",\"heap_free_bytes\":"; json += String(sys.heapFree);
        json += ",\"heap_total_bytes\":"; json += String(sys.heapTotal);
        json += ",\"heap_min_free_bytes\":"; json += String(sys.heapMinFree);
        json += ",\"heap_max_alloc_bytes\":"; json += String(sys.heapMaxAlloc);
        json += ",\"heap_fragmentation_pct\":"; json += String(sys.heapFragPct);
        json += ",\"psram_size_bytes\":"; json += String(sys.psramTotal);
        json += ",\"psram_free_bytes\":"; json += String(sys.psramFree);
        json += ",\"flash_size_bytes\":"; json += String(sys.flashSize);
        json += ",\"sketch_size_bytes\":"; json += String(sys.sketchSize);
        json += ",\"sketch_free_bytes\":"; json += String(sys.sketchFree);
        json += ",\"wifi_rssi_dbm\":"; json += String(sys.wifiRssi);
        json += ",\"wifi_signal_pct\":"; json += String(sys.wifiSignalPct);
        json += ",\"wifi_ssid\":\""; json += sys.wifiSsid; json += "\"";
        json += ",\"wifi_bssid\":\""; json += sys.wifiBssid; json += "\"";
        json += ",\"wifi_channel\":"; json += String(sys.wifiChannel);
        json += ",\"wifi_ip\":\""; json += sys.wifiIp; json += "\"";
        json += ",\"wifi_mac\":\""; json += sys.wifiMac; json += "\"";
        json += ",\"wifi_status\":\""; json += sys.wifiStatus; json += "\"";
        json += "}";

        json += "}";
        json.finish();
    }

    // =========================================================================
    // 5. Console Log Page (`/log`)
    // =========================================================================
    void handleLogPage() {
        ChunkedHtmlSender html(server);
        html += renderHeader("Console Log", "log");

        // Action Toolbar
        html += "<div style='background:var(--card);border:1px solid var(--border);border-radius:8px;padding:12px 16px;margin-bottom:15px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px;'>\n";
        html += "  <div style='display:flex;align-items:center;gap:12px;'>\n";
        html += "    <label style='display:flex;align-items:center;gap:6px;font-size:0.86rem;font-weight:600;color:var(--navy);cursor:pointer;'>\n";
        html += "      <input type='checkbox' id='autoRefresh' checked onchange='toggleAutoRefresh()'> Auto-refresh (" + String(LOG_AUTO_REFRESH_INTERVAL_SEC) + "s)\n";
        html += "    </label>\n";
        html += "    <button onclick='fetchLogNow()' class='btn btn-outline' style='padding:5px 12px;font-size:0.80rem;'>🔄 Refresh Now</button>\n";
        html += "  </div>\n";
        html += "  <div style='display:flex;align-items:center;gap:8px;'>\n";
        html += "    <a class='btn btn-primary' href='/poll_now?redirect=/log' style='padding:5px 12px;font-size:0.80rem;'>⚡ Poll Now</a>\n";
        html += "    <button id='themeBtn' onclick='toggleTheme()' class='btn btn-outline' style='padding:5px 12px;font-size:0.80rem;'>☀️ Light Theme</button>\n";
        html += "    <a class='btn btn-resume' href='/log/clear' style='padding:5px 12px;font-size:0.80rem;'>🗑️ Clear Log</a>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // Commands Bar + Custom Command Input!
        html += "<div style='background:var(--card);border:1px solid var(--border);border-radius:8px;padding:10px 16px;margin-bottom:15px;display:flex;align-items:center;flex-wrap:wrap;gap:8px;'>\n";
        html += "  <span style='font-size:0.85rem;font-weight:700;color:var(--navy);margin-right:4px;'>Quick Commands:</span>\n";
        
        auto cmdBtn = [&](const String &name) {
            return "  <button class='btn btn-outline' style='padding:5px 12px;font-family:monospace;font-weight:700;' onclick=\"sendCmd('" + name + "')\">⚡ " + name + "</button>\n";
        };

        html += cmdBtn("stat");
        html += cmdBtn("info");
        html += cmdBtn("bat");
        html += cmdBtn("pwr");
        if (stack.model == MODEL_US3000D) {
            html += cmdBtn("euro");
        } else {
            html += cmdBtn("soh");
        }

        // Custom command input & Send button
        html += "  <div style='display:flex;gap:6px;margin-left:auto;align-items:center;width:100%;max-width:320px;'>\n";
        html += "    <input type='text' id='customCmd' placeholder='Custom cmd (e.g. bat 1)...' style='flex:1;padding:6px 10px;border:1px solid #cbd5e1;border-radius:6px;font-family:monospace;font-size:0.84rem;' onkeypress=\"if(event.key==='Enter')sendCustom();\">\n";
        html += "    <button onclick='sendCustom()' class='btn btn-primary' style='padding:6px 14px;'>📤 Send</button>\n";
        html += "  </div>\n";
        html += "</div>\n";

        // Terminal Output Card (Responsive percentage height: 62vh, min 480px)
        html += "<div id='termWrap' class='term-dark' style='border-radius:10px;padding:16px;box-shadow:0 4px 14px rgba(0,0,0,0.15);overflow:hidden;display:flex;flex-direction:column;height:62vh;min-height:480px;'>\n";
        html += "  <pre id='consoleOutput' style='margin:0;font-family:SFMono-Regular,Consolas,Monaco,monospace;font-size:0.88rem;line-height:1.5;white-space:pre-wrap;word-break:break-all;height:100%;overflow-y:auto;'></pre>\n";
        html += "</div>\n";

        // JavaScript for Console, Syntax Colors & Theme
        html += "<script>\n";
        html += "let intervalId=null;\n";
        html += "function colorize(raw){if(!raw)return'';raw=raw.replace(/\\r+/g,'');let lines=raw.split('\\n');let out='';for(let i=0;i<lines.length;i++){let l=lines[i];if(l.trim()===''){out+='\\n';continue;}let esc=l.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');esc=esc.replace(/^(\\[\\d{4}-\\d{2}-\\d{2}[^\\]]+\\]|\\[\\d{2}:\\d{2}:\\d{2}\\])/,'<span class=\"ts\">$1</span>');if(l.indexOf('[ERROR]')!==-1){out+='<span class=\"line-err\">'+esc+'</span>\\n';}else if(l.indexOf('TX &gt;&gt;')!==-1||l.indexOf('TX >>')!==-1){out+='<span class=\"line-tx\">'+esc+'</span>\\n';}else if(l.indexOf('[SYSTEM]')!==-1){out+='<span class=\"line-sys\">'+esc+'</span>\\n';}else if(l.indexOf('[INFO]')!==-1){out+='<span class=\"line-info\">'+esc+'</span>\\n';}else{out+=esc+'\\n';}}return out;}\n";
        html += "function updateContent(text){let el=document.getElementById('consoleOutput');let isAtBottom=(el.scrollHeight-el.scrollTop<=el.clientHeight+60);el.innerHTML=colorize(text);if(isAtBottom){el.scrollTop=el.scrollHeight;}}\n";
        html += "function sendCmd(c){fetch('/cmd?c='+encodeURIComponent(c)+'&ajax=1').then(()=>{setTimeout(fetchLogNow,400);});}\n";
        html += "function sendCustom(){let inp=document.getElementById('customCmd');let c=inp.value.trim();if(c){sendCmd(c);inp.value='';}}\n";
        html += "function fetchLogNow(){fetch('/log/raw').then(r=>r.text()).then(t=>{updateContent(t);});}\n";
        html += "function toggleAutoRefresh(){let cb=document.getElementById('autoRefresh');if(cb.checked){startAutoRefresh();}else{clearInterval(intervalId);intervalId=null;}}\n";
        html += "function startAutoRefresh(){if(intervalId)clearInterval(intervalId);intervalId=setInterval(fetchLogNow," + String(LOG_AUTO_REFRESH_INTERVAL_SEC * 1000) + ");}\n";
        html += "function applyTheme(th){let wrap=document.getElementById('termWrap');let btn=document.getElementById('themeBtn');if(th==='light'){wrap.className='term-light';btn.innerText='🌙 Dark Theme';}else{wrap.className='term-dark';btn.innerText='☀️ Light Theme';}localStorage.setItem('pylonLogTheme',th);}\n";
        html += "function toggleTheme(){let cur=localStorage.getItem('pylonLogTheme')==='light'?'dark':'light';applyTheme(cur);}\n";
        html += "window.onload=function(){let savedTheme=localStorage.getItem('pylonLogTheme')||'dark';applyTheme(savedTheme);fetchLogNow();startAutoRefresh();};\n";
        html += "</script>\n";

        html += renderFooter();
        html.finish();
    }

    // =========================================================================
    // 6. WiFi Setup & OTA Pages
    // =========================================================================
    void handleApPortal() {
        struct ScannedNet {
            String ssid;
            int32_t rssi;
            bool isLocked;
        };
        std::vector<ScannedNet> networks;

        int numNets = WiFi.scanNetworks();
        if (numNets > 0) {
            for (int i = 0; i < numNets; ++i) {
                String s = WiFi.SSID(i);
                if (s.length() == 0) continue;
                int32_t r = WiFi.RSSI(i);
                bool locked = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

                bool found = false;
                for (auto &net : networks) {
                    if (net.ssid == s) {
                        found = true;
                        if (r > net.rssi) net.rssi = r;
                        break;
                    }
                }
                if (!found) {
                    networks.push_back({s, r, locked});
                }
            }
            WiFi.scanDelete();
            std::sort(networks.begin(), networks.end(), [](const ScannedNet &a, const ScannedNet &b) {
                return a.rssi > b.rssi;
            });
        }

        ChunkedHtmlSender html(server);
        html += renderHeader("WiFi Configuration", "settings");
        html += "<div class='table-card' style='max-width:540px;margin:20px auto;padding:24px;'>\n";
        html += "  <div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;'>\n";
        html += "    <h3 style='margin:0;color:var(--navy);font-size:1.1rem;'>📶 Connect to Home WiFi</h3>\n";
        html += "    <a href='/wifi' class='btn btn-outline' style='padding:5px 12px;font-size:0.80rem;'>🔄 Rescan WiFi</a>\n";
        html += "  </div>\n";

        if (networks.size() > 0) {
            html += "  <label style='font-size:0.85rem;font-weight:600;color:var(--text);display:block;margin-bottom:6px;'>Available Networks (click to select):</label>\n";
            html += "  <div style='max-height:190px;overflow-y:auto;border:1px solid var(--border);border-radius:6px;margin-bottom:16px;background:var(--card);'>\n";
            for (size_t i = 0; i < networks.size(); ++i) {
                const auto &net = networks[i];
                int pct = (net.rssi <= -100) ? 0 : ((net.rssi >= -50) ? 100 : (2 * (net.rssi + 100)));
                String sigCol = (pct >= 70) ? "#16a34a" : ((pct >= 40) ? "#d97706" : "#dc2626");
                String icon = net.isLocked ? "🔒" : "🔓";
                String safeSsid = net.ssid;
                safeSsid.replace("'", "\\'");
                safeSsid.replace("\"", "&quot;");

                html += "    <div onclick=\"selectWifi('" + safeSsid + "')\" style='padding:9px 12px;display:flex;justify-content:space-between;align-items:center;cursor:pointer;border-bottom:1px solid #edf2f7;transition:background 0.15s;' onmouseover=\"this.style.background='#e2e8f0'\" onmouseout=\"this.style.background='transparent'\">\n";
                html += "      <div style='font-weight:700;font-size:0.88rem;color:var(--navy);display:flex;align-items:center;gap:6px;'>📶 " + net.ssid + " <span style='font-size:0.75rem;'>" + icon + "</span></div>\n";
                html += "      <div style='font-size:0.80rem;font-weight:700;color:" + sigCol + ";'>" + String(net.rssi) + " dBm (" + String(pct) + "%)</div>\n";
                html += "    </div>\n";
            }
            html += "  </div>\n";
        } else {
            html += "  <div style='background:#fef3c7;border:1px solid #fde68a;border-radius:6px;padding:10px 14px;margin-bottom:16px;font-size:0.85rem;color:#92400e;'>⚠️ No wireless networks found during scan. You can enter your SSID manually below.</div>\n";
        }

        html += "  <form method='POST' action='/save'>\n";
        html += "    <div style='margin-bottom:14px;'><label style='font-size:0.85rem;font-weight:600;'>WiFi Network Name (SSID):</label>\n";
        html += "      <input type='text' id='ssidInput' name='ssid' required placeholder='Select from list above or type SSID' style='width:100%;padding:9px 12px;border:1px solid #cbd5e1;border-radius:6px;margin-top:4px;'></div>\n";
        html += "    <div style='margin-bottom:18px;'><label style='font-size:0.85rem;font-weight:600;'>WiFi Password:</label>\n";
        html += "      <input type='password' id='passInput' name='pass' placeholder='Enter WiFi password' style='width:100%;padding:9px 12px;border:1px solid #cbd5e1;border-radius:6px;margin-top:4px;'></div>\n";
        html += "    <button type='submit' class='btn btn-primary' style='width:100%;padding:10px;font-size:0.95rem;justify-content:center;'>💾 Save and Connect</button>\n";
        html += "  </form>\n";
        html += "</div>\n";

        html += "<script>\n";
        html += "function selectWifi(name){let inp=document.getElementById('ssidInput');if(inp){inp.value=name;let pwd=document.getElementById('passInput');if(pwd)pwd.focus();}}\n";
        html += "</script>\n";

        html += renderFooter();
        html.finish();
    }

    void handleSaveWifi() {
        if (!server.hasArg("ssid")) {
            server.send(400, "text/plain", "Missing SSID");
            return;
        }

        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);

        String html = "<!DOCTYPE html>\n<html>\n<body style='font-family:sans-serif;text-align:center;padding:50px;'>\n";
        html += "  <h2 style='color:#77b243;'>Credentials Saved!</h2>\n";
        html += "  <p>Connecting to <b>" + ssid + "</b>... Restarting device...</p>\n";
        html += "  <script>setTimeout(function(){window.location.href='/';},10000);</script>\n";
        html += "</body>\n</html>\n";

        server.send(200, "text/html", html);
        delay(1500);
        ESP.restart();
    }

    void handleOtaPage() {
        ChunkedHtmlSender html(server);
        html += renderHeader("Firmware Update", "settings");
        html += "<div class='table-card' style='max-width:500px;margin:20px auto;padding:24px;'>\n";
        html += "  <h3 style='margin:0 0 16px 0;color:var(--navy);font-size:1.1rem;'>🚀 Over-The-Air (OTA) Firmware Flash</h3>\n";
        html += "  <form method='POST' action='/update' enctype='multipart/form-data'>\n";
        html += "    <input type='file' name='firmware' accept='.bin' required style='margin-bottom:16px;width:100%;'>\n";
        html += "    <button type='submit' class='btn btn-primary' style='width:100%;padding:10px;justify-content:center;'>Flash Firmware</button>\n";
        html += "  </form>\n";
        html += "</div>\n";
        html += renderFooter();
        html.finish();
    }

    void handleOtaUpload() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            consoleLog.logInfo("[OTA] Update started: " + upload.filename);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                consoleLog.logInfo("[OTA] Update success: " + String(upload.totalSize) + " bytes");
            } else {
                Update.printError(Serial);
            }
        }
    }
};
