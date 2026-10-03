#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include "Config.h"
#include "Timezones.h"
#include "BmsModel.h"
#include "BmsPhysics.h"
#include "BmsProtocol.h"
#include "BmsUart.h"
#include "ConsoleLog.h"
#include "SystemStats.h"
#include "PeerDiscovery.h"

extern time_t lastNtpSyncTimestamp;
extern void triggerNtpSync();
extern bool isApMode;
extern void saveRackConfigToNvs();

// Pylon BMS Emulator SVG Vector Logo (Battery + Telemetry Pulse)
static const char PYLON_EMULATOR_LOGO_SVG[] PROGMEM = 
R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 330 44" style="height:42px;width:auto;display:block;">
  <defs>
    <linearGradient id="pylonGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#3c5922"/>
      <stop offset="100%" stop-color="#00595d"/>
    </linearGradient>
  </defs>
  <g transform="translate(2, 2)">
    <path d="M24 10 A15 15 0 0 1 37 23" fill="none" stroke="url(#pylonGrad)" stroke-width="2.8" stroke-linecap="round"/>
    <path d="M29 5 A22 22 0 0 1 44 20" fill="none" stroke="url(#pylonGrad)" stroke-width="2.8" stroke-linecap="round"/>
    <rect x="8" y="2" width="7.5" height="3.5" rx="1.2" fill="url(#pylonGrad)"/>
    <rect x="2.5" y="5.5" width="18.5" height="34" rx="4.5" fill="none" stroke="url(#pylonGrad)" stroke-width="2.8"/>
    <path d="M12.5 12 L7.5 22.5 L12.5 22.5 L10.5 31 L17 20.5 L12 20.5 Z" fill="url(#pylonGrad)"/>
  </g>
  <text class="brand-pylon" x="52" y="29.5" font-family="system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif" font-weight="800" font-size="21.5" fill="#171c61" letter-spacing="1.2">PYLON</text>
  <text class="brand-sub" x="135" y="29.5" font-family="system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif" font-weight="600" font-size="15" fill="#00595d" letter-spacing="1.6">BMS</text>
  <text class="brand-sub" x="178" y="29.5" font-family="system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif" font-weight="600" font-size="15" fill="#00595d" letter-spacing="1.6">EMULATOR</text>
</svg>)rawliteral";

// Pylon BMS Emulator SVG Favicon (50% Darker Gradient, Transparent Background)
static const char PYLON_EMULATOR_FAVICON_SVG[] PROGMEM = 
R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 44 44">
  <defs>
    <linearGradient id="favPylonGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#3c5922"/>
      <stop offset="100%" stop-color="#00595d"/>
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
R"rawliteral(
:root{
  --navy:#171c61;--green:#77b243;--teal:#00b3ba;--bg:#f4f6fa;--card:#ffffff;--text:#2d3748;--text-secondary:#64748b;--border:#e2e8f0;--text-muted:#64748b;
  --accent:#00b3ba;--radius:10px;--radius-sm:6px;
  --font-mono:Consolas,'Cascadia Mono',SFMono-Regular,Menlo,Monaco,monospace;
  --font-sans:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  --rack-bg:#1e293b;--rack-border:#475569;
}
html.dark{
  --navy:#38bdf8;--green:#4ade80;--teal:#2dd4bf;--bg:#0b1120;--card:#151e32;--text:#cbd5e1;--text-secondary:#94a3b8;--border:#22324d;--text-muted:#94a3b8;
  --accent:#38bdf8;--rack-bg:#0f172a;--rack-border:#334155;
}
*{box-sizing:border-box;margin:0;padding:0;font-family:var(--font-sans);}
body{background:var(--bg);color:var(--text);transition:background 0.2s,color 0.2s;}
.mono{font-family:var(--font-mono);font-variant-numeric:tabular-nums;}
.container{max-width:1150px;margin:20px auto;padding:0 20px;width:100%;}

/* Navbar */
.header-wrap{position:sticky;top:0;z-index:1000;width:100%;}
.top-accent{height:4px;background:#e2e8f0;position:relative;overflow:hidden;width:100%;}
html.dark .top-accent{background:#1e293b;}
.top-accent-bar{height:100%;width:100%;background:linear-gradient(90deg,var(--green),var(--teal));box-shadow:0 0 8px rgba(0,179,186,0.6);}
.navbar{position:relative;background:var(--card);border-bottom:1px solid var(--border);box-shadow:0 2px 8px rgba(23,28,97,0.04);padding:10px 0;width:100%;}
html.dark .navbar{box-shadow:0 2px 10px rgba(0,0,0,0.4);}
.nav-inner{max-width:1150px;margin:0 auto;padding:0 20px;width:100%;display:flex;align-items:center;justify-content:space-between;gap:16px;}
.brand{display:flex;align-items:center;gap:12px;text-decoration:none;flex-shrink:0;}
html.dark .brand-pylon{fill:#f8fafc!important;}
html.dark .brand-sub{fill:#2dd4bf!important;}
.nav-links{display:flex;gap:8px;align-items:center;justify-content:center;flex-wrap:wrap;margin:0 auto;}
.nav-link{padding:6px 14px;border-radius:6px;font-size:0.88rem;font-weight:600;text-decoration:none;color:#1e293b;border:1.5px solid #94a3b8;background:#f1f5f9;box-shadow:0 1px 2px rgba(0,0,0,0.04);transition:all 0.18s;display:inline-flex;align-items:center;gap:6px;}
.nav-link:hover{color:#ffffff;border-color:#007378;background:#008b91;box-shadow:0 2px 6px rgba(0,139,145,0.25);transform:translateY(-1px);}
.nav-link.active{background:var(--navy);color:#ffffff;border:1.5px solid #0f1240;font-weight:700;box-shadow:0 2px 5px rgba(23,28,97,0.25);}
.nav-link.active:hover{background:#23297a;color:#ffffff;}
html.dark .nav-link{color:#e2e8f0;background:#1e293b;border-color:#334155;}
html.dark .nav-link:hover{background:#0d9488;border-color:#14b8a6;color:#ffffff;}
html.dark .nav-link.active{background:#0284c7;border-color:#38bdf8;color:#ffffff;}
html.dark .nav-link.active:hover{background:#0369a1;}
.nav-right{display:flex;align-items:center;gap:10px;flex-shrink:0;}
.nav-actions{display:flex;align-items:center;flex-shrink:0;margin-left:auto;}
.theme-switch{display:inline-flex;align-items:center;background:#f1f5f9;border:1.5px solid #cbd5e1;border-radius:20px;padding:2px;gap:2px;}
.theme-btn{background:transparent;border:none;border-radius:16px;padding:4px 7px;display:inline-flex;align-items:center;justify-content:center;color:#64748b;cursor:pointer;transition:all 0.15s;line-height:1;}
.theme-btn:hover{color:#0f172a;}
.theme-btn.active{background:#ffffff;color:#0f172a;box-shadow:0 1px 2px rgba(0,0,0,0.15);}
html.dark .theme-switch{background:#1e293b;border-color:#334155;}
html.dark .theme-btn{color:#94a3b8;}
html.dark .theme-btn:hover{color:#ffffff;}
html.dark .theme-btn.active{background:#334155;color:#f8fafc;box-shadow:0 1px 2px rgba(0,0,0,0.3);}

/* Switches & Toggles */
.switch-wrap{display:inline-flex;align-items:center;gap:9px;cursor:pointer;user-select:none;padding:4px 10px;border-radius:20px;background:#f8fafc;border:1px solid #cbd5e1;transition:all 0.15s;}
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

/* Cards & Controls */
.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:18px 20px;margin-bottom:20px;box-shadow:0 4px 12px rgba(23,28,97,0.04);position:relative;overflow:hidden;}
html.dark .card{box-shadow:0 4px 14px rgba(0,0,0,0.3);}
.card::before{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:linear-gradient(90deg,var(--green),var(--teal));}
.card-header{display:flex;justify-content:space-between;align-items:center;min-height:38px;margin-bottom:16px;padding-bottom:12px;border-bottom:1px solid var(--border);}
.card-title{font-size:1.05rem;font-weight:700;display:flex;align-items:center;gap:8px;color:var(--navy);}
html.dark .card-title{color:var(--navy);}
.btn{padding:7px 14px;border-radius:6px;font-size:0.86rem;font-weight:600;text-decoration:none;display:inline-flex;align-items:center;gap:6px;transition:all 0.18s;cursor:pointer;border:none;}
.btn-primary{background:linear-gradient(135deg,var(--green),var(--teal));color:#ffffff;border:1.5px solid #009aa0;box-shadow:0 2px 6px rgba(0,179,186,0.25);}
.btn-primary:hover{opacity:0.92;transform:translateY(-1px);}
html.dark .btn-primary{background:linear-gradient(135deg,#166534,#115e59);color:#ffffff;border:1.5px solid #14b8a6;box-shadow:0 2px 6px rgba(0,0,0,0.3);}
html.dark .btn-primary:hover{background:linear-gradient(135deg,#15803d,#0f766e);border-color:#2dd4bf;}
.btn-navy{background:var(--navy);color:#ffffff;border:1.5px solid #0f1240;box-shadow:0 2px 5px rgba(23,28,97,0.2);}
.btn-navy:hover{background:#23297a;transform:translateY(-1px);}
html.dark .btn-navy{background:#0369a1;border-color:#38bdf8;color:#ffffff;}
html.dark .btn-navy:hover{background:#0284c7;}
.btn-secondary,.btn-outline{background:#f1f5f9;color:#1e293b;border:1.5px solid #94a3b8;box-shadow:0 1px 2px rgba(0,0,0,0.04);}
.btn-secondary:hover,.btn-outline:hover{background:#008b91;border-color:#007378;color:#ffffff;box-shadow:0 2px 6px rgba(0,139,145,0.25);transform:translateY(-1px);}
html.dark .btn-secondary,html.dark .btn-outline{background:#1e293b;color:#e2e8f0;border-color:#334155;}
html.dark .btn-secondary:hover,html.dark .btn-outline:hover{background:#0d9488;border-color:#14b8a6;color:#ffffff;}
.btn-resume{background:#fef2f2;color:#b91c1c;border:1.5px solid #f87171;font-weight:700;box-shadow:0 1px 3px rgba(239,68,68,0.12);}
.btn-resume:hover{background:#dc2626;border-color:#b91c1c;color:#ffffff;transform:translateY(-1px);}
html.dark .btn-resume{background:#450a0a;color:#fca5a5;border-color:#991b1b;}
html.dark .btn-resume:hover{background:#dc2626;color:#ffffff;}
.btn-danger{background:#dc2626;color:#ffffff;border:1.5px solid #b91c1c;}
.btn-danger:hover{background:#b91c1c;transform:translateY(-1px);}
.btn-warning{background:#f59e0b;color:#0f172a;font-weight:700;}
.form-group{margin-bottom:14px;}
.form-group label{display:block;margin-bottom:6px;font-size:0.82rem;font-weight:700;color:var(--text-muted);text-transform:uppercase;letter-spacing:0.04em;}
.form-control{width:100%;padding:8px 12px;background:var(--bg);border:1px solid var(--border);color:var(--text);border-radius:var(--radius-sm);font-size:0.9rem;}
.form-control:focus{outline:none;border-color:var(--teal);}
html.dark .form-control{background:#0f172a;border-color:#22324d;}
input[type="range"]{width:100%;accent-color:var(--teal);cursor:pointer;}
.range-val{font-weight:700;color:var(--navy);font-family:var(--font-mono);font-size:1.02rem;text-transform:none;}
.grid-2{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:20px;}
.grid-4{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px;}
.alert{padding:12px 16px;border-radius:var(--radius-sm);margin-bottom:16px;font-size:0.9rem;display:flex;align-items:center;justify-content:space-between;gap:12px;}
.alert-warning{background:#fffbeb;border:1.5px solid #f59e0b;color:#92400e;}
html.dark .alert-warning{background:#451a03;border-color:#78350f;color:#fde68a;}
.wifi-net-item{padding:9px 12px;display:flex;justify-content:space-between;align-items:center;cursor:pointer;border-bottom:1px solid var(--border);transition:background 0.15s;}
.wifi-net-item:hover{background:#f1f5f9;}
html.dark .wifi-net-item:hover{background:#1e293b;}
.wifi-net-item:last-child{border-bottom:none;}
.cell-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(115px,1fr));gap:10px;}
.cell-box{background:#edf2f7;border:1px solid #cbd5e1;border-radius:6px;padding:8px 6px;text-align:center;display:flex;flex-direction:column;gap:5px;align-items:center;}
html.dark .cell-box{background:#090e1a;border-color:#1e293b;}
.cell-title{font-size:0.80rem;font-weight:700;color:var(--navy);margin-bottom:1px;}
.cell-row{display:flex;align-items:center;justify-content:center;gap:6px;font-size:0.75rem;width:100%;}
.cell-lbl{color:var(--text-muted);font-weight:700;font-size:0.72rem;min-width:28px;text-align:right;}
.cell-input{width:54px;padding:2px 4px;text-align:center;font-size:0.78rem;font-weight:700;font-family:var(--font-mono);border:1px solid var(--border);border-radius:4px;background:var(--card);color:var(--text);}
.cell-input:focus{outline:none;border-color:var(--teal);}
.cell-bal-row{margin-top:2px;padding-top:4px;border-top:1px dashed rgba(100,116,139,0.3);width:100%;display:flex;justify-content:center;}
.cell-bal-label{display:flex;align-items:center;gap:4px;cursor:pointer;font-size:0.75rem;font-weight:700;color:var(--text);user-select:none;}

/* 19" Rack Visualizer & Proportional Chassis Heights */
.rack-container{background:var(--rack-bg);border:3px solid var(--rack-border);border-radius:12px;padding:18px 16px;margin:0 auto;max-width:840px;box-shadow:inset 0 0 24px rgba(0,0,0,0.6);}
.rack-chassis{display:flex;flex-direction:column;gap:10px;}
.battery-unit{background:#181c24;border:2px solid #334155;border-radius:6px;display:flex;align-items:center;justify-content:space-between;position:relative;cursor:pointer;transition:all 0.18s;box-shadow:0 4px 10px rgba(0,0,0,0.4);text-decoration:none;color:inherit;}
.battery-unit.h-2u{min-height:96px;padding:12px 16px;}
.battery-unit.h-2u .rack-handle{height:52px;}
.battery-unit.h-3u{min-height:144px;padding:16px 16px;}
.battery-unit.h-3u .rack-handle{height:88px;}
.battery-unit.h-4u{min-height:176px;padding:20px 16px;}
.battery-unit.h-4u .rack-handle{height:112px;}
.battery-unit:hover{border-color:var(--teal);transform:translateY(-1px);box-shadow:0 6px 16px rgba(0,179,186,0.25);}
.battery-unit.master{border-left:6px solid #38bdf8;}
.battery-unit.slave{border-left:6px solid #64748b;}
.rack-ear{width:16px;height:100%;display:flex;flex-direction:column;justify-content:space-between;align-items:center;padding:4px 0;}
.rack-screw{width:8px;height:8px;border-radius:50%;background:#475569;border:1px solid #64748b;}
.rack-handle{width:6px;background:#64748b;border-radius:3px;}
.panel-left{display:flex;align-items:center;gap:14px;}
.panel-col{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px;}
.pwr-switch{width:32px;height:18px;background:#0f172a;border:2px solid #475569;border-radius:3px;position:relative;}
.pwr-switch-btn{width:11px;height:12px;background:#ef4444;position:absolute;right:2px;top:1px;border-radius:2px;box-shadow:0 0 4px rgba(239,68,68,0.5);}
.dip-sw{background:#dc2626;border:1px solid #991b1b;border-radius:2px;padding:1px 3px;display:flex;gap:2px;}
.dip-pin{width:3px;height:7px;background:#ffffff;border-radius:1px;}
.rj45-block{display:flex;flex-direction:column;align-items:center;gap:1px;}
.rj45-port{width:22px;height:18px;background:#ffffff;border:1px solid #94a3b8;border-radius:3px;display:flex;align-items:center;justify-content:center;font-size:9px;color:#000;font-weight:800;}
.rj45-label{font-size:8px;color:#94a3b8;font-weight:700;text-transform:uppercase;letter-spacing:0.04em;}
.led-group{display:flex;align-items:center;gap:8px;}
.led-item{display:flex;align-items:center;gap:3px;font-size:8px;font-weight:700;color:#94a3b8;}
.led{width:9px;height:9px;border-radius:50%;background:#334155;}
.led.run{background:#22c55e;box-shadow:0 0 8px #22c55e;}
.led.alm{background:#334155;}
.led.alm.active{background:#ef4444;box-shadow:0 0 8px #ef4444;}
.soc-bar{display:flex;gap:2px;background:#0f172a;padding:3px;border-radius:3px;border:1px solid #334155;}
.soc-led{width:5px;height:11px;background:#334155;border-radius:1px;}
.soc-led.on{background:#22c55e;box-shadow:0 0 4px #22c55e;}
.panel-center{text-align:center;}
.bat-model-tag{font-size:1.20rem;font-weight:900;color:#ffffff;letter-spacing:0.05em;}
.bat-role-tag{font-size:0.74rem;font-weight:800;color:var(--teal);text-transform:uppercase;letter-spacing:0.05em;}
.panel-right{display:flex;align-items:center;gap:14px;}
.terminals{display:flex;gap:6px;}
.term-post{width:22px;height:22px;border-radius:4px;display:flex;align-items:center;justify-content:center;font-size:12px;font-weight:900;color:#ffffff;border:2px solid #ffffff;}
.term-post.neg{background:#0f172a;border-color:#475569;}
.term-post.pos{background:#ea580c;border-color:#f97316;box-shadow:0 0 8px rgba(249,115,22,0.4);}
.unit-stats{background:rgba(0,0,0,0.4);border:1px solid #334155;border-radius:6px;padding:6px 12px;text-align:right;min-width:145px;}
.unit-stats .v{font-size:1.05rem;font-weight:800;color:#38bdf8;font-family:var(--font-mono);}
.unit-stats .sub{font-size:0.75rem;color:#94a3b8;font-weight:600;font-family:var(--font-mono);}
@media(max-width:768px){.battery-unit{flex-wrap:wrap;gap:12px;justify-content:center;}.panel-left,.panel-center,.panel-right{justify-content:center;}.rack-ear{display:none;}}

/* Bottom Diagnostics & Tables */
.sys-card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:16px 20px;box-shadow:0 4px 12px rgba(23,28,97,0.04);margin-bottom:20px;}
html.dark .sys-card{box-shadow:0 4px 14px rgba(0,0,0,0.3);}
.sys-hdr{color:var(--navy);font-weight:700;font-size:0.92rem;letter-spacing:0.06em;margin-bottom:12px;border-bottom:2px solid var(--teal);padding-bottom:6px;display:flex;align-items:center;gap:8px;}
.sys-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px 32px;font-size:0.86rem;}
@media(max-width:960px){.sys-grid{grid-template-columns:1fr;gap:14px;}}
.sys-row{display:flex;justify-content:space-between;align-items:center;padding:5px 0;border-bottom:1px dashed #e2e8f0;gap:10px;white-space:nowrap;}
html.dark .sys-row{border-bottom-color:#22324d;}
.sys-label{color:#64748b;font-weight:600;white-space:nowrap;flex-shrink:0;}
html.dark .sys-label{color:#94a3b8;}
.sys-val{color:var(--navy);font-weight:700;font-family:var(--font-mono);font-size:0.88rem;white-space:nowrap;text-align:right;}
.table-card{background:var(--card);border:1px solid var(--border);border-radius:10px;box-shadow:0 4px 12px rgba(23,28,97,0.04);overflow-x:auto;width:100%;box-sizing:border-box;margin-bottom:20px;}
html.dark .table-card{box-shadow:0 4px 14px rgba(0,0,0,0.3);}
.table-header{padding:14px 18px;border-bottom:1px solid var(--border);background:#fafcff;display:flex;justify-content:space-between;align-items:center;}
html.dark .table-header{background:#182238;}
.table-header h3{margin:0;color:var(--navy);font-size:1.05rem;font-weight:700;display:flex;align-items:center;gap:8px;}
table{width:100%;min-width:100%;border-collapse:collapse;table-layout:auto;}
th{background:var(--navy);color:#ffffff;font-size:0.82rem;text-transform:uppercase;letter-spacing:0.05em;padding:11px 12px;text-align:left;font-weight:700;}
html.dark th{background:#1e293b;color:#f8fafc;}
td{padding:11px 12px;border-bottom:1px solid #edf2f7;font-size:0.90rem;color:var(--text);}
html.dark td{border-bottom:1px solid #1e293b;color:#cbd5e1;}
tr:nth-child(even){background:#fafcff;}
tr:hover{background:#f1f7f9;}
html.dark tr:nth-child(even){background:#131b2e;}
html.dark tr:hover{background:#1b2640;}
.badge{padding:3px 8px;border-radius:12px;font-size:0.75rem;font-weight:600;display:inline-flex;align-items:center;justify-content:center;box-sizing:border-box;border:1px solid transparent;line-height:1.2;height:24px;}
.badge-ok{background:#eaf6ea;color:#2d7a2d;border:1px solid #c3e6c3;}
.badge-warn{background:#fff8e6;color:#b7791f;border:1px solid #fbd38d;}
.badge-danger{background:#fef2f2;color:#dc2626;border:1px solid #fca5a5;}
.badge-master{background:#0284c7;color:#ffffff;border:1px solid #38bdf8;}
.badge-slave{background:#475569;color:#f8fafc;border:1px solid #475569;}
html.dark .badge-ok{background:#14532d;color:#86efac;border:1px solid #166534;}
html.dark .badge-warn{background:#78350f;color:#fde68a;border:1px solid #92400e;}
html.dark .badge-danger{background:#7f1d1d;color:#fca5a5;border:1px solid #991b1b;}
html.dark .badge-slave{background:#334155;color:#f8fafc;border:1px solid #334155;}
.term-dark{background:#0f172a;border:1px solid #1e293b;color:#e2e8f0;}
.term-dark .line-rx{color:#22c55e;font-weight:700;}
.term-dark .line-tx{color:#38bdf8;font-weight:700;}
.term-dark .line-err{color:#f87171;font-weight:700;}
.term-dark .line-warn{color:#fbbf24;font-weight:700;}
.term-dark .line-sys{color:#facc15;}
.term-dark .line-info{color:#a3e635;}
.term-dark .ts{color:#64748b;}
.term-light{background:#ffffff;border:1px solid #cbd5e1;color:#1e293b;}
.term-light .line-rx{color:#15803d;font-weight:700;}
.term-light .line-tx{color:#0284c7;font-weight:700;}
.term-light .line-err{color:#dc2626;font-weight:700;}
.term-light .line-warn{color:#d97706;font-weight:700;}
.term-light .line-sys{color:#d97706;}
.term-light .line-info{color:#15803d;}
.footer{margin:30px 0 20px 0;text-align:center;font-size:0.80rem;color:var(--text-muted);}
@media(min-width:1250px){.nav-actions{position:absolute;right:24px;top:50%;transform:translateY(-50%);}}
)rawliteral";

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
        server.send(200, "text/html; charset=utf-8", "");
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

class BmsWebPortal {
private:
    WebServer server;
    Preferences &prefs;
    BmsUartHandler &uartHandler;

    // Helper: Master hierarchy sorting
    static bool compareModuleHierarchy(const ModuleData &a, const ModuleData &b) {
        const ModelDescriptor &da = getModelDescriptor(a.model_type);
        const ModelDescriptor &db = getModelDescriptor(b.model_type);
        if (da.hierarchy_rank != db.hierarchy_rank) {
            return da.hierarchy_rank > db.hierarchy_rank; // Higher rank first
        }
        return a.fw_version.compareTo(b.fw_version) > 0;
    }

    // Common HTML Header template
    static String getHtmlHeader(const String &activePage, const String &title) {
        String html;
        html.reserve(2048);
        html += F("<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">");
        html += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">");
        html += "<title>" + title + " - Pylon BMS Emulator</title>";
        html += F("<link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.svg\">");
        html += F("<link rel=\"icon\" type=\"image/x-icon\" href=\"/favicon.ico\">");
        html += "<link rel=\"stylesheet\" href=\"/style.css?v=" + String(FIRMWARE_VERSION) + "\">";
        html += F("<script>"
            "(function(){"
            "try{"
            "var t=localStorage.getItem('pylon_theme')||'system';"
            "var d=false;"
            "if(t==='dark')d=true;"
            "else if(t==='light')d=false;"
            "else{"
            "var osDark=window.matchMedia&&window.matchMedia('(prefers-color-scheme: dark)').matches;"
            "var hr=new Date().getHours()+new Date().getMinutes()/60;"
            "d=osDark||(hr>=19||hr<7);"
            "}"
            "if(d)document.documentElement.classList.add('dark');"
            "else document.documentElement.classList.remove('dark');"
            "}catch(e){}"
            "})();"
            "function isDarkTheme(m){"
            "if(m==='dark')return true;"
            "if(m==='light')return false;"
            "var osDark=window.matchMedia&&window.matchMedia('(prefers-color-scheme: dark)').matches;"
            "var hr=new Date().getHours()+new Date().getMinutes()/60;"
            "return osDark||(hr>=19||hr<7);"
            "}"
            "function setTheme(m){"
            "try{"
            "if(m==='system')localStorage.removeItem('pylon_theme');"
            "else localStorage.setItem('pylon_theme',m);"
            "}catch(e){}"
            "applyThemeUI();"
            "}"
            "function applyThemeUI(){"
            "var m='system';"
            "try{m=localStorage.getItem('pylon_theme')||'system';}catch(e){}"
            "var d=isDarkTheme(m);"
            "if(d)document.documentElement.classList.add('dark');"
            "else document.documentElement.classList.remove('dark');"
            "var bl=document.getElementById('themeBtnLight'),bd=document.getElementById('themeBtnDark'),bs=document.getElementById('themeBtnSystem');"
            "if(bl)bl.className='theme-btn'+(m==='light'?' active':'');"
            "if(bd)bd.className='theme-btn'+(m==='dark'?' active':'');"
            "if(bs)bs.className='theme-btn'+(m==='system'?' active':'');"
            "}"
            "if(window.matchMedia){"
            "try{"
            "window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change',function(){"
            "if(!localStorage.getItem('pylon_theme'))applyThemeUI();"
            "});"
            "}catch(e){}"
            "}"
            "document.addEventListener('DOMContentLoaded',applyThemeUI);"
            "</script></head><body>");

        // Header element
        html += F("<div class=\"header-wrap\"><div class=\"top-accent\"><div class=\"top-accent-bar\"></div></div>");
        html += F("<div class=\"navbar\"><div class=\"nav-inner\">");
        html += "<a href=\"/\" class=\"brand\">" + String(FPSTR(PYLON_EMULATOR_LOGO_SVG)) + "</a>";
        
        html += F("<div class=\"nav-links\">");
        html += "<a class=\"nav-link" + String(activePage == "dashboard" ? " active" : "") + "\" href=\"/\">📊 Dashboard</a>";
        html += "<a class=\"nav-link" + String(activePage == "console" ? " active" : "") + "\" href=\"/console\">📟 Console Log</a>";
        html += "<a class=\"nav-link" + String(activePage == "settings" ? " active" : "") + "\" href=\"/settings\">⚙️ Settings</a>";
        html += F("<a class=\"nav-link\" href=\"/metrics\" target=\"_blank\">📈 Metrics</a>");
        html += F("<a class=\"nav-link\" href=\"/api/stack\" target=\"_blank\">🔌 API</a>");
        html += F("</div>");

        html += F("<div class=\"nav-actions\">");
        html += F("<div class=\"theme-switch\" role=\"group\" aria-label=\"Theme switcher\">");
        html += F("<button type=\"button\" class=\"theme-btn\" id=\"themeBtnLight\" onclick=\"setTheme('light')\" title=\"Light Theme\"><svg width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"4\"/><path d=\"M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41\"/></svg></button>");
        html += F("<button type=\"button\" class=\"theme-btn\" id=\"themeBtnDark\" onclick=\"setTheme('dark')\" title=\"Dark Theme\"><svg width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z\"/></svg></button>");
        html += F("<button type=\"button\" class=\"theme-btn\" id=\"themeBtnSystem\" onclick=\"setTheme('system')\" title=\"System Theme\"><svg width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"2\"/><line x1=\"8\" y1=\"21\" x2=\"16\" y2=\"21\"/><line x1=\"12\" y1=\"17\" x2=\"12\" y2=\"21\"/></svg></button>");
        html += F("</div></div>");
        html += F("</div></div></div><div class=\"container\">");

        if (isApMode) {
            html += F("<div class=\"alert alert-warning\" style=\"margin-bottom:18px;\">");
            html += F("<span>📡 <b>Standalone AP Mode (192.168.4.1)</b> &bull; Offline Field Diagnostic Mode</span>");
            html += F("<a href=\"/wifi\" style=\"font-weight:700;color:inherit;text-decoration:underline;\">Configure WiFi &rarr;</a>");
            html += F("</div>");
        }

        return html;
    }

    static String getHtmlFooter() {
        return "</div><div class=\"footer\">Pylon BMS Emulator v" + String(FIRMWARE_VERSION) + " &bull; Build: " + String(FIRMWARE_BUILD_DATE) + " " + String(FIRMWARE_BUILD_TIME) + "</div></body></html>";
    }

    bool checkWebAuth() {
        bool authEnabled = prefs.getBool(NVS_KEY_AUTH_ENABLED, false);
        if (!authEnabled) return true;

        String user = prefs.getString(NVS_KEY_AUTH_USER, "admin");
        String pass = prefs.getString(NVS_KEY_AUTH_PASS, "admin");

        if (!server.authenticate(user.c_str(), pass.c_str())) {
            server.requestAuthentication(BASIC_AUTH, "Pylon BMS Emulator");
            return false;
        }
        return true;
    }

    bool checkApiAuth() {
        bool apiAuth = prefs.getBool(NVS_KEY_API_AUTH_ENABLED, false);
        if (!apiAuth) {
            return checkWebAuth();
        }

        String expectedToken = prefs.getString(NVS_KEY_API_TOKEN, "");
        if (expectedToken.length() == 0) return true;

        // Check Authorization header: Bearer <token>
        if (server.hasHeader("Authorization")) {
            String authHeader = server.header("Authorization");
            if (authHeader.startsWith("Bearer ") && authHeader.substring(7) == expectedToken) {
                return true;
            }
        }

        // Check query parameter ?token=<token> or ?api_token=<token>
        if (server.hasArg("token") && server.arg("token") == expectedToken) {
            return true;
        }
        if (server.hasArg("api_token") && server.arg("api_token") == expectedToken) {
            return true;
        }

        // Also allow valid Web Basic Auth session if enabled
        if (prefs.getBool(NVS_KEY_AUTH_ENABLED, false)) {
            String user = prefs.getString(NVS_KEY_AUTH_USER, "admin");
            String pass = prefs.getString(NVS_KEY_AUTH_PASS, "admin");
            if (server.authenticate(user.c_str(), pass.c_str())) {
                return true;
            }
        }

        server.send(401, "application/json", "{\"error\":\"Unauthorized\",\"message\":\"Invalid or missing Bearer token (Authorization: Bearer <token> or ?token=<token>)\"}");
        return false;
    }

    void handleCaptiveRedirect() {
        server.sendHeader("Location", String("http://") + server.client().localIP().toString() + "/", true);
        server.send(302, "text/plain", "");
    }

public:
    BmsWebPortal(Preferences &p, BmsUartHandler &uart) : server(80), prefs(p), uartHandler(uart) {}

    void begin() {
        const char* headerKeys[] = {"Authorization"};
        server.collectHeaders(headerKeys, 1);
        setupRoutes();
        server.begin();
    }

    void loop() {
        server.handleClient();
    }

private:
    void setupRoutes() {
        server.on("/", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleDashboard();
        });
        server.on("/module", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleModuleDetail();
        });
        server.on("/console", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleConsole();
        });
        server.on("/settings", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleSettings();
        });
        server.on("/metrics", HTTP_GET, [this]() {
            handleMetrics();
        });
        server.on("/update", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleOtaPage();
        });
        server.on("/update", HTTP_POST, [this]() {
            if (!checkWebAuth()) return;
            handleOtaSuccess();
        }, [this]() {
            handleOtaUpload();
        });

        // Dedicated WiFi Portal & Save
        server.on("/wifi", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            handleWifiPage();
        });
        server.on("/save", HTTP_POST, [this]() {
            if (!checkWebAuth()) return;
            handleSaveWifi();
        });

        // Stylesheet & Favicons
        server.on("/style.css", [this]() {
            server.sendHeader("Cache-Control", "no-cache, must-revalidate");
            server.send_P(200, "text/css; charset=utf-8", COMMON_CSS);
        });
        server.on("/favicon.ico", [this]() {
            server.send_P(200, "image/svg+xml", PYLON_EMULATOR_FAVICON_SVG);
        });
        server.on("/favicon.svg", [this]() {
            server.send_P(200, "image/svg+xml", PYLON_EMULATOR_FAVICON_SVG);
        });

        // API Endpoints for Web UI & Automated AI Testing
        server.on("/api/data", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiStackData();
        });
        server.on("/api/stack", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiStackData();
        });
        server.on("/api/stack/control", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiStackControl();
        });
        server.on("/api/stack/control", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiStackControl();
        });
        server.on("/api/module/update", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiModuleUpdate();
        });
        server.on("/api/module/add", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiModuleAdd();
        });
        server.on("/api/module/remove", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiModuleRemove();
        });
        server.on("/api/module/delete", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiModuleDelete();
        });
        server.on("/api/hierarchy/autosort", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiHierarchyAutoSort();
        });
        server.on("/api/test/scenario", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiTestScenario();
        });
        server.on("/api/test/reset", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiTestReset();
        });
        server.on("/api/set", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/set", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/stack/set", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/stack/set", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/module/set", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/module/set", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/cell/set", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/cell/set", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiSet();
        });
        server.on("/api/console/lines", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiConsoleLines();
        });
        server.on("/api/console/raw", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiConsoleRaw();
        });
        server.on("/log/raw", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiConsoleRaw();
        });
        server.on("/api/console/send", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiConsoleSend();
        });
        server.on("/api/console/clear", HTTP_POST, [this]() {
            if (!checkApiAuth()) return;
            handleApiConsoleClear();
        });
        server.on("/log/clear", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            g_consoleLog.clear();
            server.sendHeader("Location", "/console");
            server.send(303);
        });
        server.on("/api/wifi/scan", HTTP_GET, [this]() {
            if (!checkApiAuth()) return;
            handleApiWifiScan();
        });
        server.on("/api/settings/save", HTTP_POST, [this]() {
            if (!checkWebAuth()) return;
            handleApiSettingsSave();
        });
        server.on("/sync_ntp", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            triggerNtpSync();
            server.sendHeader("Location", "/settings");
            server.send(303);
        });
        server.on("/api/system/restart", HTTP_POST, [this]() {
            if (!checkWebAuth()) return;
            server.send(200, "application/json", "{\"status\":\"ok\"}");
            delay(500);
            ESP.restart();
        });
        server.on("/restart", HTTP_GET, [this]() {
            if (!checkWebAuth()) return;
            server.send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='6;url=/'><style>body{background:#0f172a;color:#22c55e;font-family:sans-serif;text-align:center;padding:50px;}</style></head><body><h2>Restarting ESP32...</h2><p>Redirecting in 6s...</p></body></html>");
            delay(500);
            ESP.restart();
        });
        server.on("/reset_wifi", [this]() {
            if (!checkWebAuth()) return;
            prefs.remove(NVS_KEY_WIFI_SSID);
            prefs.remove(NVS_KEY_WIFI_PASS);
            server.send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='6;url=/'><style>body{background:#0f172a;color:#ef4444;font-family:sans-serif;text-align:center;padding:50px;}</style></head><body><h2>WiFi credentials erased.</h2><p>Restarting in AP mode...</p><p>Redirecting in 6s...</p></body></html>");
            delay(1000);
            ESP.restart();
        });

        // Captive portal redirects
        server.on("/generate_204", [this]() { handleCaptiveRedirect(); });
        server.on("/fwlink", [this]() { handleCaptiveRedirect(); });
        server.on("/hotspot-detect.html", [this]() { handleCaptiveRedirect(); });
        server.onNotFound([this]() {
            if (isApMode) {
                handleCaptiveRedirect();
            } else {
                server.sendHeader("Location", "/");
                server.send(302, "text/plain", "");
            }
        });
    }

    // =========================================================================
    // 1. Dashboard Page
    // =========================================================================
    void handleDashboard() {
        String warning;
        bool isHierarchyOk = validateMasterHierarchy(warning);

        ChunkedHtmlSender html(server);
        html += getHtmlHeader("dashboard", "Dashboard");

        // WiFi disconnected banner (in AP mode)
        if (WiFi.status() != WL_CONNECTED) {
            html += F("<div class=\"alert alert-warning\">");
            html += F("<div><strong>⚠️ WiFi Not Connected:</strong> Emulator is running in Access Point mode. Connect to your local WiFi for network monitoring.</div>");
            html += F("<a href=\"/settings\" class=\"btn btn-warning\" style=\"white-space:nowrap;\">📶 Configure WiFi →</a>");
            html += F("</div>");
        }

        // Hierarchy Alert if invalid
        if (!isHierarchyOk) {
            html += "<div class=\"alert alert-warning\" style=\"background:rgba(239,68,68,0.15);border:1.5px solid #ef4444;color:#fca5a5;\">";
            html += "<div><strong>⚠️ Pylontech Master Hierarchy Violation:</strong> " + warning + "</div>";
            html += F("<button class=\"rack-btn\" onclick=\"autoSortHierarchy()\" style=\"background:#ef4444;border-color:#f87171;\">⚡ Auto-Sort by Master Rules</button>");
            html += F("</div>");
        }

        // Global Rack Controls Card
        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🎛️ Global Stack Controls (Physics Simulation)</div>");
        html += F("<div style=\"display:flex;align-items:center;gap:12px;flex-wrap:wrap;\">");
        html += F("<label class=\"switch-wrap\" title=\"Simulate dynamic household inverter load, solar charge cycles, and realistic decimal fluctuations\">"
                  "<span style=\"font-size:0.82rem;font-weight:700;\">⚡ Dynamic Inverter Simulation</span>"
                  "<div class=\"switch\"><input type=\"checkbox\" id=\"inverterSimSwitch\"");
        if (g_stack.sim_inverter_enabled) html += F(" checked");
        html += F(" onchange=\"toggleInverterSim(this.checked)\"><span class=\"switch-slider\"></span></div></label>");
        html += "<span id=\"stackStatusBadge\" class=\"badge " + String(g_stack.global_current_a > 0.05 ? "badge-ok" : (g_stack.global_current_a < -0.05 ? "badge-warn" : "badge-slave")) + "\">";
        html += String(g_stack.global_current_a > 0.05 ? "Charging" : (g_stack.global_current_a < -0.05 ? "Discharging" : "Idle")) + "</span></div></div>";

        html += F("<div class=\"grid-4\">");

        // SOC Slider
        html += F("<div class=\"form-group\">");
        html += "<label>Stack SOC: <span id=\"socDisplay\" class=\"range-val\">" + String((int)g_stack.global_soc) + "%</span></label>";
        html += "<input type=\"range\" id=\"socSlider\" min=\"0\" max=\"100\" value=\"" + String((int)g_stack.global_soc) + "\" oninput=\"sendStackCtrl('soc',this.value)\">";
        html += F("<div style=\"display:flex;gap:4px;margin-top:4px;flex-wrap:wrap;justify-content:center;\">");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSoc(43)\">43%</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSoc(56)\">56%</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSoc(98)\">98%</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSoc(100)\">100%</button>");
        html += F("</div></div>");

        // Current Flow
        html += F("<div class=\"form-group\">");
        html += "<label>Current Flow: <span id=\"curDisplay\" class=\"range-val\">" + String(g_stack.global_current_a, 1) + " A</span></label>";
        html += "<input type=\"range\" id=\"curSlider\" min=\"-80\" max=\"80\" step=\"0.5\" value=\"" + String(g_stack.global_current_a, 1) + "\" oninput=\"sendStackCtrl('current',this.value)\">";
        html += F("<div style=\"display:flex;gap:4px;margin-top:4px;flex-wrap:wrap;justify-content:center;\">");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setCur(-25)\">-25A</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setCur(-16)\">-16A</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setCur(0)\">0A</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setCur(16)\">+16A</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setCur(25)\">+25A</button>");
        html += F("</div></div>");

        // Base Temperature
        html += F("<div class=\"form-group\">");
        html += "<label>Base Temperature: <span id=\"tempDisplay\" class=\"range-val\">" + String(g_stack.global_temp_c, 1) + " °C</span></label>";
        html += "<input type=\"range\" id=\"tempSlider\" min=\"-5\" max=\"55\" step=\"0.1\" value=\"" + String(g_stack.global_temp_c, 1) + "\" oninput=\"sendStackCtrl('temp',this.value)\">";
        html += F("<div style=\"display:flex;gap:4px;margin-top:4px;flex-wrap:wrap;justify-content:center;\">");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setTemp(16.2)\">16.2°C</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setTemp(22.4)\">22.4°C</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setTemp(24.5)\">24.5°C</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setTemp(32.7)\">32.7°C</button>");
        html += F("</div></div>");

        // Cell Deviation / Spread
        html += F("<div class=\"form-group\">");
        html += "<label>Cell Imbalance Spread: <span id=\"spreadDisplay\" class=\"range-val\">" + String(g_stack.cell_spread_mv) + " mV</span></label>";
        html += "<input type=\"range\" id=\"spreadSlider\" min=\"0\" max=\"50\" value=\"" + String(g_stack.cell_spread_mv) + "\" oninput=\"sendStackCtrl('spread',this.value)\">";
        html += F("<div style=\"display:flex;gap:4px;margin-top:4px;flex-wrap:wrap;justify-content:center;\">");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSpread(1)\">1mV</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSpread(13)\">13mV</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSpread(24)\">24mV</button>");
        html += F("<button type=\"button\" class=\"btn btn-secondary\" style=\"padding:2px 6px;font-size:0.75rem;\" onclick=\"setSpread(34)\">34mV</button>");
        html += F("</div></div>");

        html += F("</div></div>");

        // 19" Server Rack Simulation Card
        html += F("<div class=\"card\">");
        html += "<div class=\"card-header\"><div class=\"card-title\">🔋 19\" Battery Rack Simulation (" + String(g_stack.module_count) + " Modules)</div>";
        html += F("<div style=\"display:flex;gap:8px;flex-wrap:wrap;\">");
        html += F("<button class=\"btn btn-secondary\" onclick=\"autoSortHierarchy()\" title=\"Auto sort stack so newest/highest generation unit is Master\">✨ Auto-Sort</button>");
        html += F("<button class=\"btn btn-primary\" onclick=\"addModule()\">➕ Add Battery</button>");
        html += F("</div></div>");

        // 19" Server Rack Cabinet Container
        html += F("<div class=\"rack-container\">");
        html += F("<div class=\"rack-chassis\">");

        for (uint8_t i = 0; i < g_stack.module_count; i++) {
            const ModuleData &mod = g_stack.modules[i];
            const ModelDescriptor &desc = getModelDescriptor(mod.model_type);
            bool isMaster = (i == 0);

            float modVolts = mod.voltage_mv / 1000.0f;
            float modAmps = mod.current_ma / 1000.0f;
            int socVal = (int)mod.soc;

            int numOn = (int)round((mod.soc / 100.0f) * 6.0f);
            if (numOn > 6) numOn = 6;
            if (numOn < 0) numOn = 0;
            String socLedsHtml = "";
            for (int l = 0; l < 6; l++) {
                socLedsHtml += "<div class=\"soc-led " + String(l < numOn ? "on" : "") + "\"></div>";
            }

            String isAlarmStr = (mod.volt_st != "Normal" || mod.temp_st != "Normal" || mod.curr_st != "Normal" || mod.b_v_st != "Normal") ? "active" : "";
            String hClass = (desc.height_mm <= 100) ? "h-2u" : ((desc.height_mm >= 150) ? "h-4u" : "h-3u");
            html += "<div class=\"battery-unit " + hClass + " " + String(isMaster ? "master" : "slave") + "\" onclick=\"location.href='/module?id=" + String(mod.id) + "'\">";
            
            // Left Rack Ear
            html += F("<div class=\"rack-ear\"><div class=\"rack-screw\"></div><div class=\"rack-handle\"></div><div class=\"rack-screw\"></div></div>");

            // Panel Left (Stacked Controls: DIP switch + Power switch + RJ45 in Col 1, SW + RUN/ALM LEDs + SOC bar in Col 2)
            html += F("<div class=\"panel-left\">");
            html += F("<div class=\"panel-col\">");
            html += F("<div style=\"display:flex;align-items:center;gap:6px;\">");
            html += F("<div class=\"dip-sw\" title=\"ADD DIP switch\"><div class=\"dip-pin\"></div><div class=\"dip-pin\"></div><div class=\"dip-pin\"></div><div class=\"dip-pin\"></div></div>");
            html += F("<div class=\"pwr-switch\" title=\"POWER ON/OFF\"><div class=\"pwr-switch-btn\"></div></div>");
            html += F("</div>");
            html += "<div class=\"rj45-block\"><div class=\"rj45-port\">" + String(isMaster ? "⚡" : "⏚") + "</div><div class=\"rj45-label\">" + String(isMaster ? "Console" : "Link 0") + "</div></div>";
            html += F("</div>");
            html += F("<div class=\"panel-col\">");
            html += "<div class=\"led-group\"><div style=\"width:10px;height:10px;border-radius:50%;background:#ef4444;border:1px solid #b91c1c;\" title=\"SW Start Button\"></div><div class=\"led-item\"><div class=\"led run\" title=\"RUN LED\"></div><span>RUN</span></div><div class=\"led-item\"><div class=\"led alm " + isAlarmStr + "\" title=\"ALM LED\"></div><span>ALM</span></div></div>";
            html += "<div class=\"soc-bar\" id=\"modSocBar_" + String(mod.id) + "\">" + socLedsHtml + "</div>";
            html += F("</div></div>");

            // Panel Center
            html += F("<div class=\"panel-center\">");
            html += "<div class=\"bat-model-tag\">" + String(desc.name) + "</div>";
            html += "<div class=\"bat-role-tag\">" + String(isMaster ? "MASTER MODULE #1" : "SLAVE MODULE #" + String(mod.id)) + "</div>";
            html += F("</div>");

            // Panel Right (2 Negative (-) and 2 Positive (+) terminals like real Pylontech front panel)
            html += F("<div class=\"panel-right\">");
            html += F("<div class=\"terminals\"><div class=\"term-post neg\" title=\"Negative Power Terminal\">-</div><div class=\"term-post neg\" title=\"Negative Power Terminal\">-</div><div class=\"term-post pos\" title=\"Positive Power Terminal\">+</div><div class=\"term-post pos\" title=\"Positive Power Terminal\">+</div></div>");
            html += F("<div class=\"unit-stats\">");
            html += "<div id=\"modVolt_" + String(mod.id) + "\" class=\"v mono\">" + String(modVolts, 2) + " V</div>";
            String currSign = (modAmps > 0.01f ? "+" : "");
            html += "<div id=\"modSub_" + String(mod.id) + "\" class=\"sub mono\">" + currSign + String(modAmps, 2) + " A | " + String(socVal) + "% SOC</div>";
            html += F("</div></div>");

            // Right Rack Ear
            html += F("<div class=\"rack-ear\"><div class=\"rack-screw\"></div><div class=\"rack-handle\"></div><div class=\"rack-screw\"></div></div>");

            html += F("</div>");
        }

        html += F("</div></div></div>");

        // Bottom Diagnostics Cards: System Diagnostics (3 cols)
        SystemDiagnostics diag = SystemStatsManager::getDiagnostics();
        html += F("<div class=\"sys-card\">");
        html += F("<div class=\"sys-hdr\">⚙️ System Status & Diagnostics</div>");
        html += F("<div class=\"sys-grid\">");
        
        // Column 1
        html += F("<div>");
        html += "<div class=\"sys-row\"><span class=\"sys-label\">IP address</span><span class=\"sys-val mono\">" + diag.wifiIp + "</span></div>";
        String host = getDeviceHostname(prefs);
        html += "<div class=\"sys-row\"><span class=\"sys-label\">Hostname</span><span class=\"sys-val\">" + host + ".local</span></div>";
        int pct = (diag.wifiRssi <= -100) ? 0 : ((diag.wifiRssi >= -50) ? 100 : (2 * (diag.wifiRssi + 100)));
        html += "<div class=\"sys-row\"><span class=\"sys-label\">WiFi signal</span><span class=\"sys-val mono\">" + String(diag.wifiRssi) + " dBm (" + String(pct) + "%)</span></div>";
        html += "<div class=\"sys-row\"><span class=\"sys-label\">WiFi SSID</span><span class=\"sys-val\">" + diag.wifiSsid + "</span></div>";
        html += F("</div>");

        // Column 2
        html += F("<div>");
        html += F("<div class=\"sys-row\"><span class=\"sys-label\">Battery link</span><span class=\"sys-val\" style=\"color:var(--green);\">Active Responder</span></div>");
        html += "<div class=\"sys-row\"><span class=\"sys-label\">Battery units</span><span class=\"sys-val mono\">" + String(g_stack.module_count) + "</span></div>";
        uint32_t up = diag.uptimeSec;
        uint32_t hours = up / 3600;
        uint32_t mins = (up % 3600) / 60;
        String upStr = String(hours) + "h " + String(mins) + "m";
        if (hours == 0) upStr = String(mins) + "m " + String(up % 60) + "s";
        html += "<div class=\"sys-row\"><span class=\"sys-label\">Uptime</span><span class=\"sys-val mono\">" + upStr + "</span></div>";
        html += "<div class=\"sys-row\"><span class=\"sys-label\">RS232 Baud</span><span class=\"sys-val mono\">" + String(SERIAL_BAUD_RATE) + " (8N1)</span></div>";
        html += F("</div>");

        // Column 3
        html += F("<div>");
        html += "<div class=\"sys-row\"><span class=\"sys-label\">Free RAM</span><span class=\"sys-val mono\">" + String(diag.freeHeapBytes / 1024) + " KB (" + String(diag.heapFragmentationPct) + "% frag)</span></div>";
        html += "<div class=\"sys-row\"><span class=\"sys-label\">CPU Load / Temp</span><span class=\"sys-val mono\">" + String(diag.cpuLoadPct, 1) + "% / " + String(diag.internalTempC, 1) + " °C</span></div>";
        html += "<div class=\"sys-row\"><span class=\"sys-label\">Reset reason</span><span class=\"sys-val\">" + diag.resetReason + "</span></div>";
        html += "<div class=\"sys-row\"><span class=\"sys-label\">Chip</span><span class=\"sys-val\">" + diag.chipModel + " (" + String(diag.cpuFreqMhz) + " MHz)</span></div>";
        html += F("</div>");

        html += F("</div></div>");

        // Pylon Smart Monitors on Network (Table Card)
        std::vector<DiscoveredPeer> peers = g_peerDiscovery.getPeers();
        html += F("<div class=\"table-card\">");
        html += "<div class=\"table-header\"><h3><span>📡</span> Pylon Smart Monitors on Network</h3>";
        html += "<span class=\"badge " + String(peers.empty() ? "badge-slave" : "badge-ok") + "\">" + String(peers.size()) + " Online</span></div>";
        html += F("<table><thead><tr><th>DEVICE NAME</th><th>IP ADDRESS</th><th>BATTERY MODEL</th><th>MODULES</th><th>FIRMWARE</th><th>ACTION</th></tr></thead><tbody>");
        
        if (peers.empty()) {
            html += F("<tr><td colspan=\"6\" style=\"text-align:center;color:var(--text-muted);padding:18px;\">No active Pylon Smart Monitors discovered on LAN yet. (mDNS scanning every 60s)</td></tr>");
        } else {
            for (const auto &peer : peers) {
                html += "<tr>";
                html += "<td><strong>" + peer.hostname + ".local</strong></td>";
                html += "<td class=\"mono\">" + peer.ip.toString() + "</td>";
                html += "<td>" + String(peer.model.length() > 0 ? peer.model : "US3000C") + "</td>";
                html += "<td>" + String(peer.moduleCount > 0 ? String(peer.moduleCount) : "1") + "</td>";
                html += "<td><span class=\"badge badge-ok\">" + String(peer.version.length() > 0 ? peer.version : "v1.2.0") + "</span></td>";
                html += "<td><a href=\"http://" + peer.ip.toString() + "\" target=\"_blank\" class=\"btn btn-outline\" style=\"padding:4px 10px;font-size:0.8rem;\">Open Dashboard ↗</a></td>";
                html += "</tr>";
            }
        }
        html += F("</tbody></table></div>");

        // Embedded JS for instant real-time AJAX controls & dynamic inverter simulation
        html += F("<script>"
            "var ctrlDebounce=null,simPollTimer=null;"
            "function applyStackState(res){"
            "if(!res)return;"
            "if(res.soc!==undefined){"
            "var sEl=document.getElementById('socDisplay');if(sEl)sEl.textContent=parseFloat(res.soc).toFixed(1)+'%';"
            "var sSld=document.getElementById('socSlider');if(sSld&&document.activeElement!==sSld)sSld.value=res.soc;"
            "}"
            "if(res.current!==undefined){"
            "var cur=parseFloat(res.current);"
            "var sgn=cur>0?'+':'';"
            "var cEl=document.getElementById('curDisplay');if(cEl)cEl.textContent=sgn+cur.toFixed(2)+' A';"
            "var cSld=document.getElementById('curSlider');if(cSld&&document.activeElement!==cSld)cSld.value=cur.toFixed(1);"
            "var badge=document.getElementById('stackStatusBadge');"
            "if(badge){"
            "badge.textContent=(cur>0.05?'Charging':(cur<-0.05?'Discharging':'Idle'));"
            "badge.className='badge '+(cur>0.05?'badge-ok':(cur<-0.05?'badge-warn':'badge-slave'));"
            "}"
            "}"
            "if(res.temp!==undefined){"
            "var tEl=document.getElementById('tempDisplay');if(tEl)tEl.textContent=parseFloat(res.temp).toFixed(1)+' °C';"
            "var tSld=document.getElementById('tempSlider');if(tSld&&document.activeElement!==tSld)tSld.value=res.temp;"
            "}"
            "if(res.spread!==undefined){"
            "var spEl=document.getElementById('spreadDisplay');if(spEl)spEl.textContent=res.spread+' mV';"
            "var spSld=document.getElementById('spreadSlider');if(spSld&&document.activeElement!==spSld)spSld.value=res.spread;"
            "}"
            "if(res.inverter_sim!==undefined){"
            "var sw=document.getElementById('inverterSimSwitch');if(sw&&sw.checked!==res.inverter_sim)sw.checked=res.inverter_sim;"
            "}"
            "if(res.modules){"
            "res.modules.forEach(function(m){"
            "var vEl=document.getElementById('modVolt_'+m.id);if(vEl)vEl.textContent=m.volt.toFixed(2)+' V';"
            "var sEl=document.getElementById('modSub_'+m.id);"
            "if(sEl){var sgn=m.curr>0.001?'+':'';sEl.textContent=sgn+m.curr.toFixed(2)+' A | '+Math.round(m.soc)+'% SOC';}"
            "var bar=document.getElementById('modSocBar_'+m.id);"
            "if(bar){"
            "var nOn=Math.max(0,Math.min(6,Math.round((m.soc/100)*6)));"
            "var leds=bar.querySelectorAll('.soc-led');"
            "leds.forEach(function(l,idx){if(idx<nOn)l.classList.add('on');else l.classList.remove('on');});"
            "}"
            "});"
            "}"
            "}"
            "function pollSimState(){"
            "fetch('/api/stack/control').then(function(r){return r.json();}).then(function(res){"
            "applyStackState(res);"
            "if(res.inverter_sim){simPollTimer=setTimeout(pollSimState,1000);}"
            "}).catch(function(){"
            "var sw=document.getElementById('inverterSimSwitch');if(sw&&sw.checked)simPollTimer=setTimeout(pollSimState,2000);"
            "});"
            "}"
            "function toggleInverterSim(enabled){"
            "if(simPollTimer)clearTimeout(simPollTimer);"
            "var data=new URLSearchParams();data.append('inverter_sim',enabled?'1':'0');"
            "fetch('/api/stack/control',{method:'POST',body:data}).then(function(r){return r.json();}).then(function(res){"
            "applyStackState(res);"
            "if(enabled){simPollTimer=setTimeout(pollSimState,1000);}"
            "});"
            "}"
            "function sendStackCtrl(param,val){"
            "var sw=document.getElementById('inverterSimSwitch');if(sw&&sw.checked){sw.checked=false;if(simPollTimer)clearTimeout(simPollTimer);}"
            "if(param==='soc')document.getElementById('socDisplay').textContent=parseFloat(val).toFixed(1)+'%';"
            "if(param==='current')document.getElementById('curDisplay').textContent=(parseFloat(val)>0?'+':'')+parseFloat(val).toFixed(2)+' A';"
            "if(param==='temp')document.getElementById('tempDisplay').textContent=parseFloat(val).toFixed(1)+' °C';"
            "if(param==='spread')document.getElementById('spreadDisplay').textContent=val+' mV';"
            "var cur=parseFloat(document.getElementById('curSlider').value);"
            "var badge=document.getElementById('stackStatusBadge');"
            "if(badge){"
            "badge.textContent=(cur>0.05?'Charging':(cur<-0.05?'Discharging':'Idle'));"
            "badge.className='badge '+(cur>0.05?'badge-ok':(cur<-0.05?'badge-warn':'badge-slave'));"
            "}"
            "if(ctrlDebounce)clearTimeout(ctrlDebounce);"
            "ctrlDebounce=setTimeout(function(){"
            "var data=new URLSearchParams();"
            "data.append('soc',document.getElementById('socSlider').value);"
            "data.append('current',document.getElementById('curSlider').value);"
            "data.append('temp',document.getElementById('tempSlider').value);"
            "data.append('spread',document.getElementById('spreadSlider').value);"
            "data.append('inverter_sim','0');"
            "fetch('/api/stack/control',{method:'POST',body:data}).then(function(r){return r.json();}).then(function(res){"
            "applyStackState(res);"
            "});"
            "},80);"
            "}"
            "function setSoc(v){document.getElementById('socSlider').value=v;sendStackCtrl('soc',v);}"
            "function setCur(v){document.getElementById('curSlider').value=v;sendStackCtrl('current',v);}"
            "function setTemp(v){document.getElementById('tempSlider').value=v;sendStackCtrl('temp',v);}"
            "function setSpread(v){document.getElementById('spreadSlider').value=v;sendStackCtrl('spread',v);}"
            "function addModule(){fetch('/api/module/add',{method:'POST'}).then(function(){location.reload();});}"
            "function removeModule(){fetch('/api/module/remove',{method:'POST'}).then(function(){location.reload();});}"
            "function autoSortHierarchy(){fetch('/api/hierarchy/autosort',{method:'POST'}).then(function(){location.reload();});}"
            "document.addEventListener('DOMContentLoaded',function(){"
            "var sw=document.getElementById('inverterSimSwitch');if(sw&&sw.checked){simPollTimer=setTimeout(pollSimState,1000);}"
            "});"
            "</script>");

        html += getHtmlFooter();
    }

    // =========================================================================
    // 2. Module Detail & Editor Page (/module?id=N)
    // =========================================================================
    void handleModuleDetail() {
        int modId = server.hasArg("id") ? server.arg("id").toInt() : 1;
        if (modId < 1 || modId > g_stack.module_count) modId = 1;

        ModuleData &mod = g_stack.modules[modId - 1];
        const ModelDescriptor &desc = getModelDescriptor(mod.model_type);
        bool isMaster = (modId == 1);

        ChunkedHtmlSender html(server);
        html += getHtmlHeader("dashboard", "Module " + String(mod.id) + " Detail");

        // Module Switcher Tabs
        html += F("<div style=\"display:flex;gap:8px;margin-bottom:16px;overflow-x:auto;padding-bottom:4px;\">");
        for (uint8_t i = 0; i < g_stack.module_count; i++) {
            uint8_t id = i + 1;
            html += "<a href=\"/module?id=" + String(id) + "\" class=\"btn " + String(id == modId ? "btn-navy" : "btn-secondary") + "\">";
            html += String(id == 1 ? "★ Master (#1)" : "Slave #" + String(id)) + "</a>";
        }
        html += F("</div>");

        html += "<form id=\"moduleForm\" onsubmit=\"saveModule(event)\">";
        html += "<input type=\"hidden\" name=\"id\" value=\"" + String(mod.id) + "\">";

        // Card 1: Module Identity & General Configuration
        html += F("<div class=\"card\">");
        html += "<div class=\"card-header\"><div class=\"card-title\">⚙️ Module #" + String(mod.id) + " Identity & Hardware Configuration</div>";
        html += "<span class=\"badge " + String(isMaster ? "badge-master" : "badge-slave") + "\">" + String(isMaster ? "MASTER MODULE" : "SLAVE MODULE") + "</span></div>";

        html += F("<div class=\"grid-4\">");
        html += F("<div class=\"form-group\"><label>Model Type</label><select name=\"model_type\" class=\"form-control\">");
        for (size_t m = 0; m < MODEL_COUNT; m++) {
            html += "<option value=\"" + String(MODEL_DESCRIPTORS[m].type) + "\"" + String(mod.model_type == MODEL_DESCRIPTORS[m].type ? " selected" : "") + ">" + String(MODEL_DESCRIPTORS[m].name) + "</option>";
        }
        html += F("</select></div>");

        html += "<div class=\"form-group\"><label>State of Health (SOH %)</label><input type=\"number\" step=\"0.1\" min=\"1\" max=\"100\" name=\"soh_pct\" class=\"form-control\" value=\"" + String(mod.soh_pct, 1) + "\"></div>";
        html += "<div class=\"form-group\"><label>Firmware Version</label><input type=\"text\" name=\"fw_version\" class=\"form-control\" value=\"" + mod.fw_version + "\"></div>";
        html += "<div class=\"form-group\"><label>Barcode / S/N</label><input type=\"text\" name=\"barcode\" class=\"form-control\" value=\"" + mod.barcode + "\"></div>";
        html += "<div class=\"form-group\"><label>Pack Voltage (V)</label><input type=\"text\" class=\"form-control\" value=\"" + String(mod.voltage_mv / 1000.0f, 2) + " V\" readonly style=\"opacity:0.7;\"></div>";
        html += F("</div></div>");

        // Card 2: Cell Matrix
        html += F("<div class=\"card\">");
        html += "<div class=\"card-header\"><div class=\"card-title\">🔋 Cell Voltage, Balance & SOH Matrix (" + String(mod.cell_count) + " Cells)</div></div>";
        html += F("<div class=\"cell-grid\">");
        for (uint8_t c = 0; c < mod.cell_count; c++) {
            html += F("<div class=\"cell-box\">");
            html += "<div class=\"cell-title\">Cell #" + String(c + 1) + "</div>";
            html += "<div class=\"cell-row\"><span class=\"cell-lbl\">mV:</span>";
            html += "<input type=\"number\" name=\"cell_v_" + String(c) + "\" class=\"cell-input\" value=\"" + String(mod.cells[c].voltage_mv) + "\" min=\"2500\" max=\"3650\"></div>";
            if (desc.supports_soh) {
                html += "<div class=\"cell-row\"><span class=\"cell-lbl\">SOH:</span>";
                html += "<input type=\"number\" name=\"cell_soh_" + String(c) + "\" class=\"cell-input\" min=\"0\" max=\"99\" value=\"" + String(mod.cells[c].soh_count) + "\"></div>";
            }
            html += "<div class=\"cell-bal-row\"><label class=\"cell-bal-label\"><input type=\"checkbox\" name=\"cell_bal_" + String(c) + "\"" + String(mod.cells[c].balancing ? " checked" : "") + "> BAL</label></div>";
            html += F("</div>");
        }
        html += F("</div></div>");

        // Card 3: Current & Hardware Protections
        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">⚡ Current & Hardware Protections ('stat' Command Telemetry)</div></div>");
        html += F("<div class=\"grid-4\">");
        html += "<div class=\"form-group\"><label title=\"Charge Over-Current Cut-off\">COC Times (Charge Cut-off)</label><input type=\"number\" name=\"coc_times\" class=\"form-control\" value=\"" + String(mod.coc_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Charge Over-Current Alarm\">COCA Times (Charge Alarm)</label><input type=\"number\" name=\"coca_times\" class=\"form-control\" value=\"" + String(mod.coca_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Discharge Over-Current Cut-off\">DOC Times (Discharge Cut-off)</label><input type=\"number\" name=\"doc_times\" class=\"form-control\" value=\"" + String(mod.doc_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Discharge Over-Current Alarm\">DOCA Times (Discharge Alarm)</label><input type=\"number\" name=\"doca_times\" class=\"form-control\" value=\"" + String(mod.doca_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Short Circuit Protection\">SC Times (Short Circuit)</label><input type=\"number\" name=\"sc_times\" class=\"form-control\" value=\"" + String(mod.sc_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Reverse Voltage Protection\">RV Times (Reverse Voltage)</label><input type=\"number\" name=\"rv_times\" class=\"form-control\" value=\"" + String(mod.rv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Input Over-Voltage Protection\">Input OV Times</label><input type=\"number\" name=\"input_ov_times\" class=\"form-control\" value=\"" + String(mod.input_ov_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"BMS AFE/IC Hardware Fault\">BMICERR Times (Hardware Fault)</label><input type=\"number\" name=\"bmic_err_times\" class=\"form-control\" value=\"" + String(mod.bmic_err_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Lifetime Critical Alarms\">LifeAlarm Times</label><input type=\"number\" name=\"life_alarm_times\" class=\"form-control\" value=\"" + String(mod.life_alarm_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Lifetime System Warnings\">LifeWarn Times</label><input type=\"number\" name=\"life_warn_times\" class=\"form-control\" value=\"" + String(mod.life_warn_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Battery Sleep Transitions\">Bat SLP Times (Battery Sleep)</label><input type=\"number\" name=\"bat_slp_times\" class=\"form-control\" value=\"" + String(mod.bat_slp_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Power Bus Sleep Transitions\">Pwr SLP Times (Power Bus Sleep)</label><input type=\"number\" name=\"pwr_slp_times\" class=\"form-control\" value=\"" + String(mod.pwr_slp_times) + "\"></div>";
        html += F("</div></div>");

        // Card 4: Voltage & Thermal Protections
        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🌡️ Voltage & Thermal Protections ('stat' Command Telemetry)</div></div>");
        html += F("<div class=\"grid-4\">");
        html += "<div class=\"form-group\"><label title=\"Battery Over-Voltage Cut-off\">Bat OV Times (Over-Voltage)</label><input type=\"number\" name=\"bat_ov_times\" class=\"form-control\" value=\"" + String(mod.bat_ov_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Battery High-Voltage Warning\">Bat HV Times (High-Voltage)</label><input type=\"number\" name=\"bat_hv_times\" class=\"form-control\" value=\"" + String(mod.bat_hv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Battery Low-Voltage Warning\">Bat LV Times (Low-Voltage)</label><input type=\"number\" name=\"bat_lv_times\" class=\"form-control\" value=\"" + String(mod.bat_lv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Battery Under-Voltage Cut-off\">Bat UV Times (Under-Voltage)</label><input type=\"number\" name=\"bat_uv_times\" class=\"form-control\" value=\"" + String(mod.bat_uv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Power Bus Over-Voltage Cut-off\">Pwr OV Times</label><input type=\"number\" name=\"pwr_ov_times\" class=\"form-control\" value=\"" + String(mod.pwr_ov_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Power Bus High-Voltage Warning\">Pwr HV Times</label><input type=\"number\" name=\"pwr_hv_times\" class=\"form-control\" value=\"" + String(mod.pwr_hv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Power Bus Low-Voltage Warning\">Pwr LV Times</label><input type=\"number\" name=\"pwr_lv_times\" class=\"form-control\" value=\"" + String(mod.pwr_lv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Power Bus Under-Voltage Cut-off\">Pwr UV Times</label><input type=\"number\" name=\"pwr_uv_times\" class=\"form-control\" value=\"" + String(mod.pwr_uv_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Charge Over-Temperature\">COT Times (Charge Over-Temp)</label><input type=\"number\" name=\"cot_times\" class=\"form-control\" value=\"" + String(mod.cot_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Charge Under-Temperature\">CUT Times (Charge Under-Temp)</label><input type=\"number\" name=\"cut_times\" class=\"form-control\" value=\"" + String(mod.cut_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Discharge Over-Temperature\">DOT Times (Discharge Over-Temp)</label><input type=\"number\" name=\"dot_times\" class=\"form-control\" value=\"" + String(mod.dot_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Discharge Under-Temperature\">DUT Times (Discharge Under-Temp)</label><input type=\"number\" name=\"dut_times\" class=\"form-control\" value=\"" + String(mod.dut_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Charge High-Temperature\">CHT Times (Charge High-Temp)</label><input type=\"number\" name=\"cht_times\" class=\"form-control\" value=\"" + String(mod.cht_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Charge Low-Temperature\">CLT Times (Charge Low-Temp)</label><input type=\"number\" name=\"clt_times\" class=\"form-control\" value=\"" + String(mod.clt_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Discharge High-Temperature\">DHT Times (Discharge High-Temp)</label><input type=\"number\" name=\"dht_times\" class=\"form-control\" value=\"" + String(mod.dht_times) + "\"></div>";
        html += "<div class=\"form-group\"><label title=\"Discharge Low-Temperature\">DLT Times (Discharge Low-Temp)</label><input type=\"number\" name=\"dlt_times\" class=\"form-control\" value=\"" + String(mod.dlt_times) + "\"></div>";
        html += F("</div></div>");

        // Card 5: Operational Duration & Diagnostic Counters
        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">⏱️ Operational Duration & Diagnostic Counters ('stat')</div></div>");
        html += F("<div class=\"grid-4\">");
        html += "<div class=\"form-group\"><label>Cycle Times</label><input type=\"number\" name=\"cycle_times\" class=\"form-control\" value=\"" + String(mod.cycle_times) + "\"></div>";
        html += "<div class=\"form-group\"><label>SOH Incident Times</label><input type=\"number\" name=\"soh_times\" class=\"form-control\" value=\"" + String(mod.soh_times) + "\"></div>";
        html += "<div class=\"form-group\"><label>Emergency Shut Times</label><input type=\"number\" name=\"shut_times\" class=\"form-control\" value=\"" + String(mod.shut_times) + "\"></div>";
        html += "<div class=\"form-group\"><label>BMS Reset Times</label><input type=\"number\" name=\"rst_times\" class=\"form-control\" value=\"" + String(mod.rst_times) + "\"></div>";
        html += "<div class=\"form-group\"><label>Power On Times</label><input type=\"number\" name=\"power_on_times\" class=\"form-control\" value=\"" + String(mod.power_on_times) + "\"></div>";
        html += "<div class=\"form-group\"><label>Idle Duration / Times</label><input type=\"number\" name=\"idle_times\" class=\"form-control\" value=\"" + String(mod.idle_times) + "\"></div>";
        html += "<div class=\"form-group\"><label>Charge Times / Duration</label><input type=\"number\" name=\"chg_times_or_secs\" class=\"form-control\" value=\"" + String(mod.chg_times_or_secs) + "\"></div>";
        html += "<div class=\"form-group\"><label>Discharge Times / Duration</label><input type=\"number\" name=\"dsg_times_or_secs\" class=\"form-control\" value=\"" + String(mod.dsg_times_or_secs) + "\"></div>";
        html += "<div class=\"form-group\"><label>Total Discharged Cap (mAh/Ah)</label><input type=\"number\" name=\"dsg_cap_total\" class=\"form-control\" value=\"" + String(mod.dsg_cap_total) + "\"></div>";
        html += F("</div></div>");

        // Card 6: European Efficiency Stats ('euro' - if Model D)
        if (desc.supports_euro) {
            html += F("<div class=\"card\">");
            html += F("<div class=\"card-header\"><div class=\"card-title\">🇪🇺 European Efficiency Stats ('euro' Command Telemetry)</div></div>");
            html += F("<div class=\"grid-4\">");
            html += "<div class=\"form-group\"><label>SOH (%)</label><input type=\"number\" step=\"0.1\" name=\"euro_soh\" class=\"form-control\" value=\"" + String(mod.euro.soh_pct, 1) + "\"></div>";
            html += "<div class=\"form-group\"><label>Life Expectance (Days)</label><input type=\"number\" name=\"euro_life\" class=\"form-control\" value=\"" + String(mod.euro.life_expect_days) + "\"></div>";
            html += "<div class=\"form-group\"><label>Energy Throughput (kWh)</label><input type=\"number\" step=\"0.1\" name=\"euro_energy\" class=\"form-control\" value=\"" + String(mod.euro.energy_thro_kwh, 1) + "\"></div>";
            html += "<div class=\"form-group\"><label>Round Trip Efficiency (%)</label><input type=\"number\" step=\"0.1\" name=\"euro_eff\" class=\"form-control\" value=\"" + String(mod.euro.round_trip_eff_pct, 1) + "\"></div>";
            html += F("</div></div>");
        }

        // Card 7: Live Fault & Alarm Injection
        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🚨 Live Fault & Alarm State Injection</div></div>");
        html += F("<div class=\"grid-4\">");

        html += F("<div class=\"form-group\"><label>Voltage State (Volt.St)</label><select name=\"volt_st\" class=\"form-control\">");
        html += "<option value=\"Normal\"" + String(mod.volt_st == "Normal" ? " selected" : "") + ">Normal</option>";
        html += "<option value=\"High\"" + String(mod.volt_st == "High" ? " selected" : "") + ">High (Warning)</option>";
        html += "<option value=\"Low\"" + String(mod.volt_st == "Low" ? " selected" : "") + ">Low (Warning)</option>";
        html += "<option value=\"Over\"" + String(mod.volt_st == "Over" ? " selected" : "") + ">Over (Alarm)</option>";
        html += "<option value=\"Under\"" + String(mod.volt_st == "Under" ? " selected" : "") + ">Under (Alarm)</option>";
        html += F("</select></div>");

        html += F("<div class=\"form-group\"><label>Current State (Curr.St)</label><select name=\"curr_st\" class=\"form-control\">");
        html += "<option value=\"Normal\"" + String(mod.curr_st == "Normal" ? " selected" : "") + ">Normal</option>";
        html += "<option value=\"High\"" + String(mod.curr_st == "High" ? " selected" : "") + ">High (Warning)</option>";
        html += "<option value=\"Over\"" + String(mod.curr_st == "Over" ? " selected" : "") + ">Over (Alarm)</option>";
        html += F("</select></div>");

        html += F("<div class=\"form-group\"><label>Temperature State (Temp.St)</label><select name=\"temp_st\" class=\"form-control\">");
        html += "<option value=\"Normal\"" + String(mod.temp_st == "Normal" ? " selected" : "") + ">Normal</option>";
        html += "<option value=\"High\"" + String(mod.temp_st == "High" ? " selected" : "") + ">High (Warning)</option>";
        html += "<option value=\"Low\"" + String(mod.temp_st == "Low" ? " selected" : "") + ">Low (Warning)</option>";
        html += "<option value=\"Over\"" + String(mod.temp_st == "Over" ? " selected" : "") + ">Over (Alarm)</option>";
        html += F("</select></div>");

        html += F("<div class=\"form-group\"><label>Battery Voltage State (B.V.St)</label><select name=\"b_v_st\" class=\"form-control\">");
        html += "<option value=\"Normal\"" + String(mod.b_v_st == "Normal" ? " selected" : "") + ">Normal</option>";
        html += "<option value=\"Alarm\"" + String(mod.b_v_st == "Alarm" ? " selected" : "") + ">Alarm</option>";
        html += "<option value=\"Error\"" + String(mod.b_v_st == "Error" ? " selected" : "") + ">Error</option>";
        html += F("</select></div>");

        html += F("</div>");

        html += F("<div style=\"display:flex;justify-content:flex-end;gap:10px;align-items:center;margin-top:20px;padding-top:16px;border-top:1px solid var(--border);flex-wrap:wrap;\">");
        html += F("<a href=\"/\" class=\"btn btn-secondary\">Cancel</a>");
        html += F("<button type=\"button\" class=\"btn btn-danger\" onclick=\"deleteModule()\">🗑️ Remove Battery</button>");
        html += F("<button type=\"submit\" class=\"btn btn-primary\">💾 Save Changes</button>");
        html += F("</div></div></form>");

        // Save & Delete JS handlers
        html += F("<script>"
            "function saveModule(e){"
            "e.preventDefault();"
            "var formData=new FormData(document.getElementById('moduleForm'));"
            "var params=new URLSearchParams();"
            "for(var pair of formData.entries()){params.append(pair[0],pair[1]);}"
            "fetch('/api/module/update',{method:'POST',body:params}).then(function(){location.href='/';});"
            "}"
            "function deleteModule(){"
            "var id=document.querySelector('input[name=\"id\"]').value;"
            "if(confirm('Are you sure you want to remove Module #'+id+'?')){"
            "var params=new URLSearchParams();params.append('id',id);"
            "fetch('/api/module/delete',{method:'POST',body:params}).then(function(r){"
            "if(r.ok){location.href='/';}"
            "else{r.json().then(function(e){alert(e.error||'Cannot remove module');});}"
            "});"
            "}"
            "}"
            "</script>");

        html += getHtmlFooter();
    }

    // =========================================================================
    // 3. Clean Serial Console (/console)
    // =========================================================================
    void handleConsole() {
        ChunkedHtmlSender html(server);
        html += getHtmlHeader("console", "Console Log");

        // Top Toolbar
        html += F("<div style=\"background:var(--card);border:1px solid var(--border);border-radius:8px;padding:10px 16px;margin-bottom:15px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px;\">");
        html += F("<div style=\"display:flex;align-items:center;gap:12px;font-size:0.86rem;color:var(--text);\">");
        html += F("<label style=\"display:flex;align-items:center;gap:6px;cursor:pointer;\"><input type=\"checkbox\" id=\"autoRefresh\" checked onchange=\"toggleAutoRefresh()\"><span>Auto-refresh log (every 1s)</span></label>");
        html += F("<button onclick=\"fetchLogNow()\" class=\"btn btn-outline\" style=\"padding:5px 12px;font-size:0.80rem;\">🔄 Refresh Now</button>");
        html += F("</div>");
        html += F("<div style=\"display:flex;align-items:center;gap:8px;\">");
        html += F("<button id=\"themeBtn\" onclick=\"toggleTheme()\" class=\"btn btn-outline\" style=\"padding:5px 12px;font-size:0.80rem;\">☀️ Light Theme</button>");
        html += F("<button class=\"btn btn-resume\" onclick=\"clearLog()\" style=\"padding:5px 12px;font-size:0.80rem;\">🗑️ Clear Log</button>");
        html += F("</div></div>");

        // Quick Commands Bar + Custom Command Input (Generated dynamically per Master battery model)
        html += F("<div style=\"background:var(--card);border:1px solid var(--border);border-radius:8px;padding:10px 16px;margin-bottom:15px;display:flex;align-items:center;flex-wrap:wrap;gap:8px;\">");
        html += F("<span style=\"font-size:0.85rem;font-weight:700;color:var(--navy);margin-right:4px;\">Quick Commands:</span>");
        
        auto cmdBtn = [&](const String &name) {
            return "<button class=\"btn btn-outline\" style=\"padding:5px 12px;font-family:var(--font-mono);font-weight:700;\" onclick=\"sendCmd('" + name + "')\">⚡ " + name + "</button>";
        };

        const ModelDescriptor &masterDesc = (g_stack.module_count > 0) ? getModelDescriptor(g_stack.modules[0].model_type) : getModelDescriptor(MODEL_US3000C);

        html += cmdBtn("stat");
        html += cmdBtn("info");
        html += cmdBtn("bat");
        html += cmdBtn("pwr");
        if (masterDesc.supports_euro) {
            html += cmdBtn("euro");
        } else if (masterDesc.supports_soh) {
            html += cmdBtn("soh");
        }

        html += F("<div style=\"display:flex;gap:6px;margin-left:auto;align-items:center;width:100%;max-width:320px;\">");
        html += F("<input type=\"text\" id=\"customCmd\" placeholder=\"Custom cmd (e.g. bat 1)...\" style=\"flex:1;padding:6px 10px;border:1px solid #cbd5e1;border-radius:6px;font-family:var(--font-mono);font-size:0.84rem;\" onkeypress=\"if(event.key==='Enter')sendCustom();\">");
        html += F("<button onclick=\"sendCustom()\" class=\"btn btn-primary\" style=\"padding:6px 14px;\">📤 Send</button>");
        html += F("</div></div>");

        // Terminal Window
        html += F("<div id=\"termWrap\" class=\"term-dark\" style=\"border-radius:10px;padding:16px;box-shadow:0 4px 14px rgba(0,0,0,0.15);overflow:hidden;display:flex;flex-direction:column;height:62vh;min-height:480px;\">");
        html += F("<pre id=\"consoleOutput\" style=\"margin:0;font-family:Consolas,'Cascadia Mono','Cascadia Code',ui-monospace,'SFMono-Regular',Menlo,monospace;font-size:0.82rem;line-height:1.5;white-space:pre-wrap;word-break:break-all;height:100%;overflow-y:auto;\"></pre>");
        html += F("</div>");

        // Terminal JS
        html += F("<script>"
            "var intervalId=null;"
            "function colorize(raw){"
            "if(!raw)return'';"
            "raw=raw.replace(/\\r+/g,'');"
            "var lines=raw.split('\\n');"
            "var out='';"
            "for(var i=0;i<lines.length;i++){"
            "var l=lines[i];"
            "if(l.trim()===''){out+='\\n';continue;}"
            "var esc=l.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');"
            "esc=esc.replace(/^(\\[\\d{4}-\\d{2}-\\d{2}[^\\]]+\\]|\\[\\d{2}:\\d{2}:\\d{2}\\])/,'<span class=\"ts\">$1</span>');"
            "if(l.indexOf('[ERROR]')!==-1){out+='<span class=\"line-err\">'+esc+'</span>\\n';}"
            "else if(l.indexOf('[WARN]')!==-1||l.indexOf('[SECURITY]')!==-1){out+='<span class=\"line-warn\">'+esc+'</span>\\n';}"
            "else if(l.indexOf('>> RX:')!==-1||l.indexOf('RX >>')!==-1||l.indexOf('>>')!==-1){out+='<span class=\"line-rx\">'+esc+'</span>\\n';}"
            "else if(l.indexOf('<< TX:')!==-1||l.indexOf('<<')!==-1||l.indexOf('TX >>')!==-1){out+='<span class=\"line-tx\">'+esc+'</span>\\n';}"
            "else if(l.indexOf('[System]')!==-1||l.indexOf('[SYSTEM]')!==-1){out+='<span class=\"line-sys\">'+esc+'</span>\\n';}"
            "else if(l.indexOf('[INFO]')!==-1){out+='<span class=\"line-info\">'+esc+'</span>\\n';}"
            "else{out+=esc+'\\n';}"
            "}"
            "return out;"
            "}"
            "function updateContent(text){"
            "var el=document.getElementById('consoleOutput');"
            "if(!el)return;"
            "var isAtBottom=(el.scrollHeight-el.scrollTop<=el.clientHeight+60);"
            "el.innerHTML=colorize(text);"
            "if(isAtBottom){el.scrollTop=el.scrollHeight;}"
            "}"
            "function sendCmd(c){"
            "var data=new URLSearchParams();data.append('cmd',c);"
            "fetch('/api/console/send',{method:'POST',body:data}).then(function(){setTimeout(fetchLogNow,200);});"
            "}"
            "function sendCustom(){"
            "var inp=document.getElementById('customCmd');"
            "var c=inp.value.trim();"
            "if(c){sendCmd(c);inp.value='';}"
            "}"
            "function fetchLogNow(){"
            "fetch('/log/raw').then(function(r){return r.text();}).then(function(t){updateContent(t);}).catch(function(e){});"
            "}"
            "function clearLog(){"
            "if(confirm('Clear console log?')){"
            "fetch('/api/console/clear',{method:'POST'}).then(function(){fetchLogNow();});"
            "}"
            "}"
            "function toggleAutoRefresh(){"
            "var cb=document.getElementById('autoRefresh');"
            "if(cb.checked){startAutoRefresh();}"
            "else{clearInterval(intervalId);intervalId=null;}"
            "}"
            "function startAutoRefresh(){"
            "if(intervalId)clearInterval(intervalId);"
            "intervalId=setInterval(fetchLogNow,1000);"
            "}"
            "function applyTheme(th){"
            "var wrap=document.getElementById('termWrap');"
            "var btn=document.getElementById('themeBtn');"
            "if(!wrap||!btn)return;"
            "if(th==='light'){"
            "wrap.className='term-light';"
            "btn.innerText='🌙 Dark Theme';"
            "}else{"
            "wrap.className='term-dark';"
            "btn.innerText='☀️ Light Theme';"
            "}"
            "localStorage.setItem('pylonLogTheme',th);"
            "}"
            "function toggleTheme(){"
            "var cur=localStorage.getItem('pylonLogTheme')==='light'?'dark':'light';"
            "applyTheme(cur);"
            "}"
            "window.onload=function(){"
            "var savedTheme=localStorage.getItem('pylonLogTheme');"
            "if(!savedTheme){"
            "savedTheme=document.documentElement.classList.contains('dark')?'dark':'light';"
            "}"
            "applyTheme(savedTheme);"
            "fetchLogNow();"
            "startAutoRefresh();"
            "};"
            "</script>");

        html += getHtmlFooter();
    }

    // =========================================================================
    // 4. Settings Page (/settings)
    // =========================================================================
    void handleSettings() {
        ChunkedHtmlSender html(server);
        html += getHtmlHeader("settings", "Settings");

        bool staticEn = prefs.getBool(NVS_KEY_STATIC_IP_EN, false);
        String staticIp = prefs.getString(NVS_KEY_STATIC_IP, isApMode ? "192.168.4.1" : WiFi.localIP().toString());
        String staticMask = prefs.getString(NVS_KEY_STATIC_MASK, isApMode ? "255.255.255.0" : WiFi.subnetMask().toString());
        String staticGw = prefs.getString(NVS_KEY_STATIC_GW, isApMode ? "192.168.4.1" : WiFi.gatewayIP().toString());
        String staticDns = prefs.getString(NVS_KEY_STATIC_DNS, isApMode ? "192.168.4.1" : WiFi.dnsIP().toString());

        bool ntpEn = prefs.getBool(NVS_KEY_NTP_ENABLED, true);
        String ntpSrv = prefs.getString(NVS_KEY_NTP_SERVER, NTP_DEFAULT_SERVER);
        String savedCity = prefs.getString(NVS_KEY_TZ_CITY, DEFAULT_TZ_CITY);
        bool timeFormat24h = prefs.getBool(NVS_KEY_TIME_FORMAT_24H, true);

        // System Actions Card (Placed at the top above parameter cards)
        html += F("<div class=\"card\" style=\"margin-bottom:20px;\">");
        html += F("<div class=\"card-header\" style=\"margin-bottom:14px;\"><div class=\"card-title\">⚙️ System Actions</div></div>");
        html += F("<div style=\"display:flex;gap:12px;flex-wrap:wrap;\">");
        html += F("<a href=\"/wifi\" class=\"btn btn-outline\">📶 Reconfigure WiFi</a>");
        html += F("<a href=\"/update\" class=\"btn btn-outline\">🚀 Firmware Update (OTA)</a>");
        html += F("<a href=\"/restart\" onclick=\"return confirm('Restart ESP32?');\" class=\"btn btn-outline\">🔄 Restart Device</a>");
        html += F("<a href=\"/reset_wifi\" onclick=\"return confirm('Factory reset WiFi settings? Device will start in AP mode.');\" class=\"btn btn-outline\">⚠️ Factory Reset WiFi</a>");
        html += F("</div></div>");

        html += F("<form id=\"settingsForm\" onsubmit=\"saveSettings(event)\">");

        // Card 1: Network & Device Identity (Hostname & Static IP)
        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🌐 Network & Device Identity</div></div>");
        
        String devHost = prefs.getString(NVS_KEY_HOSTNAME, "");
        String defaultHost = getDefaultHostname();

        html += F("<div style=\"margin-bottom:16px;max-width:440px;\">");
        html += F("<label style=\"font-size:0.82rem;font-weight:700;color:var(--text-muted);text-transform:uppercase;display:block;margin-bottom:4px;\">Device Hostname (mDNS / OTA / DHCP):</label>");
        html += "<input type=\"text\" name=\"dev_host\" value=\"" + devHost + "\" placeholder=\"" + defaultHost + "\" maxlength=\"32\" class=\"form-control\" style=\"font-family:var(--font-mono);\">";
        html += "<small style=\"color:var(--text-muted);display:block;margin-top:4px;\">Local URL: <code>http://" + (devHost.length() > 0 ? devHost : defaultHost) + ".local/</code></small>";
        html += F("</div>");

        html += F("<label style=\"display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;margin-bottom:12px;cursor:pointer;\">");
        html += "<input type=\"checkbox\" name=\"ip_static\" value=\"1\"" + String(staticEn ? " checked" : "") + "> Use Static IP Configuration (instead of DHCP)";
        html += F("</label>");

        html += F("<div class=\"grid-4\">");
        html += "<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">Static IP Address:</label><input type=\"text\" name=\"ip_addr\" class=\"form-control\" value=\"" + staticIp + "\" placeholder=\"192.168.1.150\"></div>";
        html += "<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">Subnet Mask:</label><input type=\"text\" name=\"ip_mask\" class=\"form-control\" value=\"" + staticMask + "\" placeholder=\"255.255.255.0\"></div>";
        html += "<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">Default Gateway:</label><input type=\"text\" name=\"ip_gw\" class=\"form-control\" value=\"" + staticGw + "\" placeholder=\"192.168.1.1\"></div>";
        html += "<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">Primary DNS Server:</label><input type=\"text\" name=\"ip_dns\" class=\"form-control\" value=\"" + staticDns + "\" placeholder=\"192.168.1.1\"></div>";
        html += F("</div>");
        html += F("<small style=\"color:var(--text-muted);display:block;margin-top:8px;\">When unchecked, device dynamically receives network parameters via DHCP from your router.</small>");
        html += F("</div>");

        // Card 2: Time Synchronization & Timezone (NTP)
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

        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🕒 Time Synchronization & Timezone (NTP)</div>");
        html += "<span class=\"badge " + String(isSynced ? "badge-ok\">Synced (OK)" : "badge-warn\">Waiting for sync") + "</span></div>";

        // Status banner
        html += F("<div style=\"background:var(--bg);border:1px solid var(--border);border-radius:var(--radius-sm);padding:12px 16px;margin-bottom:16px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:12px;\">");
        html += F("<div>");
        html += "<div style=\"font-size:0.88rem;color:var(--text);font-weight:600;\">Current Device Time: <span style=\"color:var(--navy);font-family:var(--font-mono);font-size:0.95rem;font-weight:700;margin-left:4px;\">" + currentTimeStr + "</span></div>";
        html += "<div style=\"font-size:0.80rem;color:var(--text-muted);margin-top:4px;\">Last Sync: <b>" + lastSyncStr + "</b> &bull; Sync Interval: <b>Every 1 hour (3600s)</b></div>";
        html += F("</div>");
        html += F("<div><a href=\"/sync_ntp\" class=\"btn btn-secondary\" style=\"padding:5px 12px;font-size:0.80rem;\">🔄 Sync Time Now</a></div>");
        html += F("</div>");

        html += F("<label style=\"display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;margin-bottom:12px;cursor:pointer;\">");
        html += "<input type=\"checkbox\" name=\"ntp_en\" value=\"1\"" + String(ntpEn ? " checked" : "") + "> Enable NTP Network Time Protocol";
        html += F("</label>");

        html += F("<div class=\"grid-4\">");
        html += "<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">NTP Server:</label><input type=\"text\" name=\"ntp_srv\" class=\"form-control\" value=\"" + ntpSrv + "\" placeholder=\"pool.ntp.org\"></div>";
        html += F("<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">City & Timezone:</label><select name=\"tz_idx\" class=\"form-control\">");
        for (size_t i = 0; i < TIMEZONE_COUNT; ++i) {
            bool sel = (savedCity == TIMEZONE_LIST[i].city);
            html += "<option value=\"" + String(i) + "\"" + (sel ? " selected" : "") + ">" + String(TIMEZONE_LIST[i].city) + "</option>";
        }
        html += F("</select></div>");
        html += F("<div><label style=\"font-size:0.82rem;font-weight:600;display:block;margin-bottom:4px;\">Time Format:</label><select name=\"time_fmt\" class=\"form-control\">");
        html += "<option value=\"24\"" + String(timeFormat24h ? " selected" : "") + ">24 Hours (e.g. 20:15:00)</option>";
        html += "<option value=\"12\"" + String(!timeFormat24h ? " selected" : "") + ">12 Hours (e.g. 8:15:00 PM)</option>";
        html += F("</select></div>");
        html += F("</div>");
        html += F("<small style=\"color:var(--text-muted);display:block;margin-top:8px;\">Device time adjusts automatically for Daylight Saving Time (DST).</small>");
        html += F("</div>");

        // Card 3: Web Security & Access Protection
        bool authEn = prefs.getBool(NVS_KEY_AUTH_ENABLED, false);
        String authUser = prefs.getString(NVS_KEY_AUTH_USER, "admin");
        bool apiEn = prefs.getBool(NVS_KEY_API_AUTH_ENABLED, false);
        String apiTok = prefs.getString(NVS_KEY_API_TOKEN, "");

        html += F("<div class=\"card\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🔒 Web Security & Access Protection</div>");
        html += "<span class=\"badge " + String((authEn || apiEn) ? "badge-ok\">Protected" : "badge-warn\">Open Access") + "</span></div>";

        html += F("<div style=\"margin-bottom:14px;\">");
        html += F("<label style=\"display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;cursor:pointer;\">");
        html += "<input type=\"checkbox\" name=\"auth_en\" value=\"1\"" + String(authEn ? " checked" : "") + "> Enable HTTP Basic Authentication for Web UI";
        html += F("</label>");
        html += F("</div>");

        html += F("<div class=\"grid-2\">");
        html += "<div class=\"form-group\"><label>Admin Username</label><input type=\"text\" name=\"auth_usr\" placeholder=\"Username\" value=\"" + authUser + "\" class=\"form-control\"></div>";
        html += F("<div class=\"form-group\"><label>Admin Password</label><input type=\"password\" name=\"auth_pwd\" placeholder=\"Leave blank to keep current password\" class=\"form-control\"></div>");
        html += F("</div>");
        html += F("<small style=\"color:var(--text-muted);display:block;margin-top:4px;\">When enabled, browser prompts for username and password to access the Web UI and settings.</small>");

        html += F("<hr style=\"border:0;border-top:1px solid var(--border);margin:16px 0;\">");

        html += F("<div style=\"margin-bottom:12px;\">");
        html += F("<label style=\"display:flex;align-items:center;gap:8px;font-weight:600;font-size:0.9rem;cursor:pointer;\">");
        html += "<input type=\"checkbox\" name=\"api_en\" value=\"1\"" + String(apiEn ? " checked" : "") + "> Require Bearer Token for REST API & Parameter Setter (<code>/api/...</code>)";
        html += F("</label>");
        html += F("</div>");

        html += F("<div class=\"form-group\">");
        html += F("<label>API Bearer Token</label>");
        html += F("<div style=\"display:flex;gap:10px;flex-wrap:wrap;\">");
        html += "<input type=\"text\" id=\"api_tok_field\" name=\"api_tok\" value=\"" + apiTok + "\" placeholder=\"Enter or generate secret API token\" class=\"form-control\" style=\"font-family:var(--font-mono);flex:1;min-width:180px;\">";
        html += F("<button type=\"button\" onclick=\"genToken()\" class=\"btn btn-outline\" style=\"white-space:nowrap;\">🔑 Generate New Token</button>");
        html += F("</div>");
        html += F("<small style=\"color:var(--text-muted);display:block;margin-top:6px;\">External scripts and AI agents authenticate with HTTP header <code>Authorization: Bearer &lt;token&gt;</code> or URL parameter <code>?token=&lt;token&gt;</code>.</small>");
        html += F("</div>");
        html += F("</div>");

        html += F("<div style=\"display:flex;justify-content:flex-end;gap:8px;margin-top:20px;padding-top:16px;border-top:1px solid var(--border);\">");
        html += F("<button type=\"submit\" class=\"btn btn-primary\">💾 Save Settings & Reboot</button>");
        html += F("</div></form>");

        html += F("<script>"
            "function genToken(){var r='';var c='abcdef0123456789';for(var i=0;i<32;i++)r+=c.charAt(Math.floor(Math.random()*c.length));var f=document.getElementById('api_tok_field');if(f)f.value='pbe_'+r;}"
            "function saveSettings(e){"
            "e.preventDefault();"
            "var formData=new FormData(document.getElementById('settingsForm'));"
            "var params=new URLSearchParams();"
            "for(var pair of formData.entries()){params.append(pair[0],pair[1]);}"
            "if(confirm('Save settings and reboot device?')){"
            "fetch('/api/settings/save',{method:'POST',body:params}).then(function(){alert('Settings saved. Device rebooting...');location.href='/';});"
            "}"
            "}"
            "</script>");

        html += getHtmlFooter();
    }

    // =========================================================================
    // Dedicated WiFi Portal Page (/wifi)
    // =========================================================================
    void handleWifiPage() {
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
        html += getHtmlHeader("settings", "WiFi Configuration");
        html += F("<div class=\"card\" style=\"max-width:540px;margin:20px auto;\">\n");
        html += F("  <div class=\"card-header\">\n");
        html += F("    <div class=\"card-title\">📶 Connect to Home WiFi</div>\n");
        html += F("    <a href=\"/wifi\" class=\"btn btn-outline\" style=\"padding:4px 10px;font-size:0.80rem;\">🔄 Rescan WiFi</a>\n");
        html += F("  </div>\n");

        if (networks.size() > 0) {
            html += F("  <label style=\"font-size:0.85rem;font-weight:600;display:block;margin-bottom:6px;\">Available Networks (click to select):</label>\n");
            html += F("  <div style=\"max-height:190px;overflow-y:auto;border:1px solid var(--border);border-radius:6px;margin-bottom:16px;background:var(--card);\">\n");
            for (size_t i = 0; i < networks.size(); ++i) {
                const auto &net = networks[i];
                int pct = (net.rssi <= -100) ? 0 : ((net.rssi >= -50) ? 100 : (2 * (net.rssi + 100)));
                String sigCol = (pct >= 70) ? "#16a34a" : ((pct >= 40) ? "#d97706" : "#dc2626");
                String icon = net.isLocked ? "🔒" : "🔓";
                String safeSsid = net.ssid;
                safeSsid.replace("'", "\\'");
                safeSsid.replace("\"", "&quot;");

                html += "    <div class=\"wifi-net-item\" onclick=\"selectWifi('" + safeSsid + "')\" style=\"padding:8px 12px;display:flex;justify-content:space-between;align-items:center;cursor:pointer;border-bottom:1px solid var(--border);\">\n";
                html += "      <div style=\"font-weight:700;font-size:0.88rem;color:var(--navy);display:flex;align-items:center;gap:6px;\">📶 " + net.ssid + " <span style=\"font-size:0.75rem;\">" + icon + "</span></div>\n";
                html += "      <div style=\"font-size:0.80rem;font-weight:700;color:" + sigCol + ";font-family:var(--font-mono);\">" + String(net.rssi) + " dBm (" + String(pct) + "%)</div>\n";
                html += "    </div>\n";
            }
            html += F("  </div>\n");
        } else {
            html += F("  <div class=\"alert alert-warning\">⚠️ No wireless networks found during scan. You can enter your SSID manually below.</div>\n");
        }

        String currentSsid = prefs.getString(NVS_KEY_WIFI_SSID, "");
        html += F("  <form method=\"POST\" action=\"/save\">\n");
        html += F("    <div class=\"form-group\"><label>WiFi Network Name (SSID):</label>\n");
        html += "      <input type=\"text\" id=\"ssidInput\" name=\"ssid\" required value=\"" + currentSsid + "\" placeholder=\"Select from list above or type SSID\" class=\"form-control\"></div>\n";
        html += F("    <div class=\"form-group\"><label>WiFi Password:</label>\n");
        html += F("      <input type=\"password\" id=\"passInput\" name=\"pass\" placeholder=\"Enter WiFi password\" class=\"form-control\"></div>\n");
        html += F("    <button type=\"submit\" class=\"btn btn-primary\" style=\"width:100%;padding:10px;font-size:0.95rem;justify-content:center;\">💾 Save and Connect</button>\n");
        html += F("  </form>\n");
        html += F("</div>\n");

        html += F("<script>\n");
        html += F("function selectWifi(name){let inp=document.getElementById('ssidInput');if(inp){inp.value=name;let pwd=document.getElementById('passInput');if(pwd)pwd.focus();}}\n");
        html += F("</script>\n");

        html += getHtmlFooter();
    }

    void handleSaveWifi() {
        if (!server.hasArg("ssid")) {
            server.send(400, "text/plain", "Missing SSID");
            return;
        }

        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        prefs.putString(NVS_KEY_WIFI_SSID, ssid);
        prefs.putString(NVS_KEY_WIFI_PASS, pass);

        String html = "<!DOCTYPE html>\n<html>\n<head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><style>"
                      "body{font-family:sans-serif;text-align:center;padding:50px;background:#f4f6fa;color:#171c61;}"
                      "@media(prefers-color-scheme:dark){body{background:#0b1120;color:#e2e8f0;}}"
                      "</style></head>\n<body>\n";
        html += "  <h2 style='color:#16a34a;'>Credentials Saved!</h2>\n";
        html += "  <p>Connecting to <b>" + ssid + "</b>... Restarting device...</p>\n";
        html += "  <script>setTimeout(function(){window.location.href='/';},10000);</script>\n";
        html += "</body>\n</html>\n";

        server.send(200, "text/html", html);
        delay(1500);
        ESP.restart();
    }

    // =========================================================================
    // OTA Handlers & WiFi Scan API
    // =========================================================================
    void handleOtaPage() {
        ChunkedHtmlSender html(server);
        html += getHtmlHeader("settings", "Firmware Update");
        html += F("<div class=\"card\" style=\"max-width:560px;margin:20px auto;\">");
        html += F("<div class=\"card-header\"><div class=\"card-title\">🚀 Over-The-Air (OTA) Firmware Flash</div></div>");
        
        html += "<div style=\"background:var(--bg);border:1px solid var(--border);border-radius:6px;padding:12px 14px;margin-bottom:16px;font-size:0.86rem;\">";
        html += "<div style=\"display:flex;justify-content:space-between;margin-bottom:6px;\"><span style=\"color:var(--text-muted);font-weight:600;\">Current Firmware:</span><span class=\"mono\" style=\"font-weight:700;color:var(--navy);\">v" + String(FIRMWARE_VERSION) + "</span></div>";
        html += "<div style=\"display:flex;justify-content:space-between;\"><span style=\"color:var(--text-muted);font-weight:600;\">Build Timestamp:</span><span class=\"mono\" style=\"font-weight:700;color:var(--navy);\">" + String(FIRMWARE_BUILD_DATE) + " " + String(FIRMWARE_BUILD_TIME) + "</span></div>";
        html += F("</div>");

        html += F("<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\" style=\"display:flex;flex-direction:column;gap:14px;\">");
        html += F("<div class=\"form-group\"><label>Select Firmware Binary (.bin)</label><input type=\"file\" name=\"firmware\" accept=\".bin\" required class=\"form-control\"></div>");
        html += F("<button type=\"submit\" class=\"btn btn-primary\" style=\"width:100%;padding:10px;font-size:0.95rem;justify-content:center;\">⚡ Flash Firmware</button>");
        html += F("</form></div>");
        html += getHtmlFooter();
    }

    void handleOtaSuccess() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='12;url=/'><style>body{background:#0f172a;color:#22c55e;font-family:sans-serif;text-align:center;padding:50px;}</style></head><body><h2>Update Successful!</h2><p>Rebooting device... Redirecting in 12s...</p></body></html>");
        delay(1000);
        ESP.restart();
    }

    void handleOtaUpload() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (!Update.end(true)) {
                Update.printError(Serial);
            }
        }
    }

    void handleApiWifiScan() {
        int n = WiFi.scanNetworks();
        String json = "[";
        if (n > 0) {
            struct NetItem { String ssid; int rssi; bool locked; };
            std::vector<NetItem> list;
            for (int i = 0; i < n; i++) {
                String s = WiFi.SSID(i);
                if (s.length() == 0) continue;
                int r = WiFi.RSSI(i);
                bool l = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                bool dup = false;
                for (auto &it : list) {
                    if (it.ssid == s) {
                        dup = true;
                        if (r > it.rssi) it.rssi = r;
                        break;
                    }
                }
                if (!dup) list.push_back({s, r, l});
            }
            WiFi.scanDelete();
            std::sort(list.begin(), list.end(), [](const NetItem &a, const NetItem &b) {
                return a.rssi > b.rssi;
            });
            for (size_t i = 0; i < list.size(); i++) {
                int pct = (list[i].rssi <= -100) ? 0 : ((list[i].rssi >= -50) ? 100 : (2 * (list[i].rssi + 100)));
                String safeSsid = list[i].ssid;
                safeSsid.replace("\\", "\\\\");
                safeSsid.replace("\"", "\\\"");
                json += "{\"ssid\":\"" + safeSsid + "\",\"rssi\":" + String(list[i].rssi) + ",\"pct\":" + String(pct) + ",\"locked\":" + (list[i].locked ? "true" : "false") + "}";
                if (i < list.size() - 1) json += ",";
            }
        }
        json += "]";
        server.send(200, "application/json", json);
    }

    // =========================================================================
    // API Endpoint Handlers
    // =========================================================================
    void handleApiStackControl() {
        if (server.hasArg("inverter_sim")) {
            String inv = server.arg("inverter_sim");
            g_stack.sim_inverter_enabled = (inv == "1" || inv == "true");
        }
        if (server.hasArg("soc")) {
            g_stack.global_soc = server.arg("soc").toFloat();
            g_stack.sim_inverter_enabled = false;
        }
        if (server.hasArg("current")) {
            g_stack.global_current_a = server.arg("current").toFloat();
            g_stack.sim_inverter_enabled = false;
        }
        if (server.hasArg("temp")) {
            g_stack.global_temp_c = server.arg("temp").toFloat();
        }
        if (server.hasArg("spread")) {
            g_stack.cell_spread_mv = (uint16_t)server.arg("spread").toInt();
        }

        recalculatePhysics();

        String json;
        json.reserve(1024);
        json += "{\"status\":\"ok\"";
        json += ",\"inverter_sim\":" + String(g_stack.sim_inverter_enabled ? "true" : "false");
        json += ",\"soc\":" + String(g_stack.global_soc, 2);
        json += ",\"current\":" + String(g_stack.global_current_a, 2);
        json += ",\"temp\":" + String(g_stack.global_temp_c, 1);
        json += ",\"spread\":" + String(g_stack.cell_spread_mv);
        json += ",\"modules\":[";
        for (uint8_t i = 0; i < g_stack.module_count; i++) {
            const ModuleData &mod = g_stack.modules[i];
            if (i > 0) json += ",";
            json += "{\"id\":" + String(mod.id) + ",\"volt\":" + String(mod.voltage_mv / 1000.0f, 2) + ",\"curr\":" + String(mod.current_ma / 1000.0f, 2) + ",\"soc\":" + String(mod.soc, 1) + "}";
        }
        json += "]}";
        server.send(200, "application/json", json);
    }

    void handleApiModuleUpdate() {
        if (!server.hasArg("id")) {
            server.send(400, "application/json", "{\"error\":\"Missing id\"}");
            return;
        }
        int id = server.arg("id").toInt();
        if (id < 1 || id > g_stack.module_count) {
            server.send(400, "application/json", "{\"error\":\"Invalid id\"}");
            return;
        }

        ModuleData &mod = g_stack.modules[id - 1];
        if (server.hasArg("model_type")) {
            mod.model_type = (PylonModelType)server.arg("model_type").toInt();
            const ModelDescriptor &desc = getModelDescriptor(mod.model_type);
            mod.cell_count = desc.cell_count;
        }
        if (server.hasArg("soh_pct")) {
            mod.soh_pct = server.arg("soh_pct").toFloat();
            mod.euro.soh_pct = mod.soh_pct;
        }
        if (server.hasArg("fw_version")) mod.fw_version = server.arg("fw_version");
        if (server.hasArg("barcode")) mod.barcode = server.arg("barcode");

        // Cell voltages, balancing & cell SOH
        int32_t totalPackMv = 0;
        for (uint8_t c = 0; c < mod.cell_count; c++) {
            String vKey = "cell_v_" + String(c);
            String bKey = "cell_bal_" + String(c);
            String sKey = "cell_soh_" + String(c);
            if (server.hasArg(vKey)) {
                mod.cells[c].voltage_mv = (uint16_t)server.arg(vKey).toInt();
            }
            mod.cells[c].balancing = server.hasArg(bKey);
            if (server.hasArg(sKey)) {
                mod.cells[c].soh_count = (uint16_t)server.arg(sKey).toInt();
            }
            totalPackMv += mod.cells[c].voltage_mv;
        }
        mod.voltage_mv = totalPackMv;

        // Current & Hardware Protections
        if (server.hasArg("coc_times")) mod.coc_times = server.arg("coc_times").toInt();
        if (server.hasArg("coca_times")) mod.coca_times = server.arg("coca_times").toInt();
        if (server.hasArg("doc_times")) mod.doc_times = server.arg("doc_times").toInt();
        if (server.hasArg("doca_times")) mod.doca_times = server.arg("doca_times").toInt();
        if (server.hasArg("sc_times")) mod.sc_times = server.arg("sc_times").toInt();
        if (server.hasArg("rv_times")) mod.rv_times = server.arg("rv_times").toInt();
        if (server.hasArg("input_ov_times")) mod.input_ov_times = server.arg("input_ov_times").toInt();
        if (server.hasArg("bmic_err_times")) mod.bmic_err_times = server.arg("bmic_err_times").toInt();
        if (server.hasArg("life_alarm_times")) mod.life_alarm_times = server.arg("life_alarm_times").toInt();
        if (server.hasArg("life_warn_times")) mod.life_warn_times = server.arg("life_warn_times").toInt();
        if (server.hasArg("bat_slp_times")) mod.bat_slp_times = server.arg("bat_slp_times").toInt();
        if (server.hasArg("pwr_slp_times")) mod.pwr_slp_times = server.arg("pwr_slp_times").toInt();

        // Voltage & Thermal Protections
        if (server.hasArg("bat_ov_times")) mod.bat_ov_times = server.arg("bat_ov_times").toInt();
        if (server.hasArg("bat_hv_times")) mod.bat_hv_times = server.arg("bat_hv_times").toInt();
        if (server.hasArg("bat_lv_times")) mod.bat_lv_times = server.arg("bat_lv_times").toInt();
        if (server.hasArg("bat_uv_times")) mod.bat_uv_times = server.arg("bat_uv_times").toInt();
        if (server.hasArg("pwr_ov_times")) mod.pwr_ov_times = server.arg("pwr_ov_times").toInt();
        if (server.hasArg("pwr_hv_times")) mod.pwr_hv_times = server.arg("pwr_hv_times").toInt();
        if (server.hasArg("pwr_lv_times")) mod.pwr_lv_times = server.arg("pwr_lv_times").toInt();
        if (server.hasArg("pwr_uv_times")) mod.pwr_uv_times = server.arg("pwr_uv_times").toInt();
        if (server.hasArg("cot_times")) mod.cot_times = server.arg("cot_times").toInt();
        if (server.hasArg("cut_times")) mod.cut_times = server.arg("cut_times").toInt();
        if (server.hasArg("dot_times")) mod.dot_times = server.arg("dot_times").toInt();
        if (server.hasArg("dut_times")) mod.dut_times = server.arg("dut_times").toInt();
        if (server.hasArg("cht_times")) mod.cht_times = server.arg("cht_times").toInt();
        if (server.hasArg("clt_times")) mod.clt_times = server.arg("clt_times").toInt();
        if (server.hasArg("dht_times")) mod.dht_times = server.arg("dht_times").toInt();
        if (server.hasArg("dlt_times")) mod.dlt_times = server.arg("dlt_times").toInt();

        // Operational Duration & Diagnostic Counters
        if (server.hasArg("cycle_times")) mod.cycle_times = server.arg("cycle_times").toInt();
        if (server.hasArg("soh_times")) mod.soh_times = server.arg("soh_times").toInt();
        if (server.hasArg("shut_times")) mod.shut_times = server.arg("shut_times").toInt();
        if (server.hasArg("rst_times")) mod.rst_times = server.arg("rst_times").toInt();
        if (server.hasArg("power_on_times")) mod.power_on_times = server.arg("power_on_times").toInt();
        if (server.hasArg("idle_times")) mod.idle_times = server.arg("idle_times").toInt();
        if (server.hasArg("chg_times_or_secs")) mod.chg_times_or_secs = server.arg("chg_times_or_secs").toInt();
        if (server.hasArg("dsg_times_or_secs")) mod.dsg_times_or_secs = server.arg("dsg_times_or_secs").toInt();
        if (server.hasArg("dsg_cap_total")) mod.dsg_cap_total = server.arg("dsg_cap_total").toInt();

        // Euro stats
        if (server.hasArg("euro_soh")) {
            mod.euro.soh_pct = server.arg("euro_soh").toFloat();
            mod.soh_pct = mod.euro.soh_pct;
        }
        if (server.hasArg("euro_life")) mod.euro.life_expect_days = server.arg("euro_life").toInt();
        if (server.hasArg("euro_energy")) mod.euro.energy_thro_kwh = server.arg("euro_energy").toFloat();
        if (server.hasArg("euro_eff")) mod.euro.round_trip_eff_pct = server.arg("euro_eff").toFloat();

        // Alarm states
        if (server.hasArg("volt_st")) mod.volt_st = server.arg("volt_st");
        if (server.hasArg("curr_st")) mod.curr_st = server.arg("curr_st");
        if (server.hasArg("temp_st")) {
            mod.temp_st = server.arg("temp_st");
            mod.dtemp_st = mod.temp_st;
            mod.ctemp_st = mod.temp_st;
        }
        if (server.hasArg("b_v_st")) mod.b_v_st = server.arg("b_v_st");

        saveRackConfigToNvs();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    }

    void handleApiModuleAdd() {
        if (g_stack.module_count >= MAX_MODULES) {
            server.send(400, "application/json", "{\"error\":\"Max modules reached\"}");
            return;
        }
        uint8_t newId = g_stack.module_count + 1;
        ModuleData &newMod = g_stack.modules[g_stack.module_count];
        newMod.id = newId;
        newMod.model_type = MODEL_US3000C;
        newMod.fw_version = "V2.8";
        newMod.barcode = "PPYB2022041800" + String(newId < 10 ? "0" : "") + String(newId);
        newMod.cell_count = 15;
        newMod.soc = g_stack.global_soc;
        newMod.soh_pct = 99.0f;
        newMod.volt_st = "Normal";
        newMod.curr_st = "Normal";
        newMod.temp_st = "Normal";
        newMod.dtemp_st = "Normal";
        newMod.ctemp_st = "Normal";
        newMod.mos_temp_st = "Normal";
        newMod.b_v_st = "Normal";
        newMod.b_t_st = "Normal";
        newMod.cycle_times = 120;
        newMod.soh_times = 0;
        newMod.shut_times = 1;
        newMod.rst_times = 3;
        newMod.power_on_times = 12;
        newMod.idle_times = 3690;
        newMod.chg_times_or_secs = 452;
        newMod.dsg_times_or_secs = 450;
        newMod.dsg_cap_total = 24500000;

        // Current & Hardware Protections
        newMod.coc_times = 0;
        newMod.coca_times = 0;
        newMod.doc_times = 0;
        newMod.doca_times = 0;
        newMod.sc_times = 0;
        newMod.rv_times = 0;
        newMod.input_ov_times = 0;
        newMod.bmic_err_times = 0;
        newMod.life_alarm_times = 0;
        newMod.life_warn_times = 0;
        newMod.bat_slp_times = 0;
        newMod.pwr_slp_times = 0;

        // Voltage & Thermal Protections
        newMod.bat_ov_times = 0;
        newMod.bat_hv_times = 0;
        newMod.bat_lv_times = 0;
        newMod.bat_uv_times = 0;
        newMod.pwr_ov_times = 0;
        newMod.pwr_hv_times = 0;
        newMod.pwr_lv_times = 0;
        newMod.pwr_uv_times = 0;
        newMod.cot_times = 0;
        newMod.cut_times = 0;
        newMod.dot_times = 0;
        newMod.dut_times = 0;
        newMod.cht_times = 0;
        newMod.clt_times = 0;
        newMod.dht_times = 0;
        newMod.dlt_times = 0;

        // Euro Stats
        newMod.euro.soh_pct = 99.0f;
        newMod.euro.life_expect_days = 5475;
        newMod.euro.energy_thro_kwh = 1250.0f;
        newMod.euro.round_trip_eff_pct = 96.5f;

        g_stack.module_count++;

        recalculatePhysics();
        saveRackConfigToNvs();
        server.send(200, "application/json", "{\"status\":\"ok\",\"count\":" + String(g_stack.module_count) + "}");
    }

    void handleApiModuleRemove() {
        if (g_stack.module_count <= 1) {
            server.send(400, "application/json", "{\"error\":\"Must have at least 1 module\"}");
            return;
        }
        g_stack.module_count--;
        recalculatePhysics();
        saveRackConfigToNvs();
        server.send(200, "application/json", "{\"status\":\"ok\",\"count\":" + String(g_stack.module_count) + "}");
    }

    void handleApiModuleDelete() {
        if (g_stack.module_count <= 1) {
            server.send(400, "application/json", "{\"error\":\"Cannot remove the only module in the rack\"}");
            return;
        }
        if (!server.hasArg("id")) {
            server.send(400, "application/json", "{\"error\":\"Missing module id\"}");
            return;
        }
        int id = server.arg("id").toInt();
        if (id < 1 || id > g_stack.module_count) {
            server.send(400, "application/json", "{\"error\":\"Invalid module id\"}");
            return;
        }

        int delIdx = id - 1;
        for (int i = delIdx; i < g_stack.module_count - 1; i++) {
            g_stack.modules[i] = g_stack.modules[i + 1];
            g_stack.modules[i].id = (uint8_t)(i + 1);
        }
        g_stack.module_count--;
        recalculatePhysics();
        saveRackConfigToNvs();
        server.send(200, "application/json", "{\"status\":\"ok\",\"count\":" + String(g_stack.module_count) + "}");
    }

    void handleApiHierarchyAutoSort() {
        autoSortHierarchy();
        recalculatePhysics();
        saveRackConfigToNvs();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    }

    void handleApiConsoleLines() {
        server.send(200, "application/json", "[]");
    }

    void handleApiConsoleRaw() {
        g_consoleLog.streamLog(server);
    }

    void handleApiConsoleClear() {
        g_consoleLog.clear();
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    }

    void handleApiConsoleSend() {
        if (!server.hasArg("cmd") && !server.hasArg("c")) {
            server.send(400, "application/json", "{\"error\":\"Missing cmd\"}");
            return;
        }
        String cmd = server.hasArg("cmd") ? server.arg("cmd") : server.arg("c");
        cmd.trim();
        String resp = "";
        if (cmd.length() > 0) {
            resp = uartHandler.processCommand(cmd);
        }

        String txtEscaped = resp;
        txtEscaped.replace("\\", "\\\\");
        txtEscaped.replace("\"", "\\\"");
        txtEscaped.replace("\n", "\\n");
        txtEscaped.replace("\r", "");

        server.send(200, "application/json", "{\"status\":\"ok\",\"response\":\"" + txtEscaped + "\"}");
    }

    void handleApiStackData() {
        String json;
        json.reserve(2048);
        json += "{\"status\":\"ok\",\"stack\":{";
        json += "\"module_count\":" + String(g_stack.module_count) + ",";
        json += "\"global_soc\":" + String(g_stack.global_soc, 2) + ",";
        json += "\"global_current_a\":" + String(g_stack.global_current_a, 2) + ",";
        json += "\"global_temp_c\":" + String(g_stack.global_temp_c, 1) + ",";
        json += "\"cell_spread_mv\":" + String(g_stack.cell_spread_mv) + ",";
        json += "\"sim_inverter_enabled\":" + String(g_stack.sim_inverter_enabled ? "true" : "false") + ",";
        json += "\"auto_physics\":" + String(g_stack.auto_physics ? "true" : "false");
        json += "},\"modules\":[";
        for (uint8_t i = 0; i < g_stack.module_count; i++) {
            const ModuleData &m = g_stack.modules[i];
            if (i > 0) json += ",";
            json += "{\"id\":" + String(m.id) + ",";
            json += "\"model_type\":" + String((int)m.model_type) + ",";
            json += "\"model_name\":\"" + String(getModelDescriptor(m.model_type).name) + "\",";
            json += "\"fw_version\":\"" + m.fw_version + "\",";
            json += "\"barcode\":\"" + m.barcode + "\",";
            json += "\"voltage_v\":" + String(m.voltage_mv / 1000.0f, 3) + ",";
            json += "\"current_a\":" + String(m.current_ma / 1000.0f, 2) + ",";
            json += "\"soc\":" + String(m.soc, 1) + ",";
            json += "\"soh_pct\":" + String(m.soh_pct, 1) + ",";
            json += "\"cycle_times\":" + String(m.cycle_times) + ",";
            json += "\"volt_st\":\"" + m.volt_st + "\",";
            json += "\"curr_st\":\"" + m.curr_st + "\",";
            json += "\"temp_st\":\"" + m.temp_st + "\",";
            json += "\"cell_count\":" + String(m.cell_count) + ",";
            json += "\"cells\":[";
            for (uint8_t c = 0; c < m.cell_count; c++) {
                if (c > 0) json += ",";
                json += "{\"cell\":" + String(c) + ",\"v_mv\":" + String(m.cells[c].voltage_mv) + ",\"bal\":" + (m.cells[c].balancing ? "true" : "false") + ",\"soh\":" + String(m.cells[c].soh_count) + "}";
            }
            json += "]}";
        }
        json += "]}";
        server.send(200, "application/json", json);
    }

    void handleApiTestReset() {
        initBmsDefaults();
        saveRackConfigToNvs();
        server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Stack reset to factory defaults\"}");
    }

    void handleApiTestScenario() {
        String s = server.hasArg("scenario") ? server.arg("scenario") : (server.hasArg("s") ? server.arg("s") : (server.hasArg("name") ? server.arg("name") : "normal"));
        s.toLowerCase();

        if (s == "normal" || s == "healthy") {
            g_stack.module_count = 4;
            g_stack.global_soc = 92.0f;
            g_stack.global_current_a = -1.25f;
            g_stack.global_temp_c = 23.5f;
            g_stack.cell_spread_mv = 7;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = true;

            for (uint8_t i = 0; i < 4; i++) {
                ModuleData &m = g_stack.modules[i];
                m.id = i + 1;
                m.model_type = MODEL_US3000C;
                m.fw_version = "V2.8";
                m.barcode = "PPYB2022041800" + String(i + 1 < 10 ? "0" : "") + String(i + 1);
                m.cell_count = 15;
                m.soc = 92.0f;
                m.soh_pct = 99.5f;
                m.volt_st = "Normal";
                m.curr_st = "Normal";
                m.temp_st = "Normal";
                m.cycle_times = 120 + i * 15;
                m.bat_ov_times = 0;
                m.bat_hv_times = 0;
                m.bat_lv_times = 0;
                m.cot_times = 0;
                m.cut_times = 0;
            }
            recalculatePhysics();
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"normal\",\"message\":\"Healthy 4-module US3000C stack loaded\"}");
        } else if (s == "imbalance" || s == "spread") {
            g_stack.module_count = 2;
            g_stack.global_soc = 85.0f;
            g_stack.global_current_a = 5.0f; // Charging
            g_stack.global_temp_c = 25.0f;
            g_stack.cell_spread_mv = 380;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = false;

            for (uint8_t i = 0; i < 2; i++) {
                ModuleData &m = g_stack.modules[i];
                m.id = i + 1;
                m.model_type = MODEL_US3000C;
                m.fw_version = "V2.8";
                m.cell_count = 15;
                m.soc = 85.0f;
                m.soh_pct = 98.0f;
                m.volt_st = "Normal";
                m.curr_st = "Normal";
                m.temp_st = "Normal";
                int32_t totV = 0;
                for (uint8_t c = 0; c < 15; c++) {
                    uint16_t cv = 3320;
                    if (c == 2) { cv = 3620; m.cells[c].balancing = true; } // High cell & balancing
                    else if (c == 7) { cv = 3240; m.cells[c].balancing = false; } // Low cell
                    else { m.cells[c].balancing = false; }
                    m.cells[c].voltage_mv = cv;
                    totV += cv;
                }
                m.voltage_mv = totV;
            }
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"imbalance\",\"message\":\"High cell imbalance (380mV spread) with active balancing simulated\"}");
        } else if (s == "overvoltage" || s == "alarm_ov") {
            g_stack.module_count = 1;
            g_stack.global_soc = 100.0f;
            g_stack.global_current_a = 15.0f;
            g_stack.global_temp_c = 28.0f;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = false;

            ModuleData &m = g_stack.modules[0];
            m.id = 1;
            m.model_type = MODEL_US3000C;
            m.fw_version = "V2.8";
            m.cell_count = 15;
            m.soc = 100.0f;
            m.soh_pct = 99.0f;
            m.volt_st = "High";
            m.bat_ov_times = 3;
            m.bat_hv_times = 12;
            int32_t totV = 0;
            for (uint8_t c = 0; c < 15; c++) {
                uint16_t cv = (c == 0) ? 3680 : 3550;
                m.cells[c].voltage_mv = cv;
                totV += cv;
            }
            m.voltage_mv = totV;
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"overvoltage\",\"message\":\"Cell overvoltage alarm (3.68V, bat_ov_times=3) simulated\"}");
        } else if (s == "overtemp" || s == "alarm_temp") {
            g_stack.module_count = 1;
            g_stack.global_soc = 75.0f;
            g_stack.global_current_a = 45.0f;
            g_stack.global_temp_c = 54.0f;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = true;

            ModuleData &m = g_stack.modules[0];
            m.id = 1;
            m.model_type = MODEL_US3000C;
            m.temp_st = "High";
            m.dtemp_st = "High";
            m.ctemp_st = "High";
            m.cot_times = 2;
            recalculatePhysics();
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"overtemp\",\"message\":\"High temperature protection (54°C, cot_times=2) simulated\"}");
        } else if (s == "undertemp" || s == "cold") {
            g_stack.module_count = 1;
            g_stack.global_soc = 75.0f;
            g_stack.global_current_a = 0.0f;
            g_stack.global_temp_c = -4.5f;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = true;

            ModuleData &m = g_stack.modules[0];
            m.id = 1;
            m.model_type = MODEL_US3000C;
            m.temp_st = "Low";
            m.cut_times = 2;
            recalculatePhysics();
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"undertemp\",\"message\":\"Low temperature warning (-4.5°C, cut_times=2) simulated\"}");
        } else if (s == "deep_discharge" || s == "low_soc") {
            g_stack.module_count = 1;
            g_stack.global_soc = 4.5f;
            g_stack.global_current_a = -35.0f;
            g_stack.global_temp_c = 27.0f;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = true;

            ModuleData &m = g_stack.modules[0];
            m.id = 1;
            m.model_type = MODEL_US3000C;
            m.volt_st = "Low";
            m.bat_lv_times = 2;
            m.bat_uv_times = 1;
            recalculatePhysics();
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"deep_discharge\",\"message\":\"Deep discharge (4.5% SOC, 45V, bat_lv_times=2) simulated\"}");
        } else if (s == "hierarchy_error" || s == "hierarchy") {
            g_stack.module_count = 2;
            g_stack.global_soc = 80.0f;
            g_stack.global_current_a = -2.0f;
            g_stack.global_temp_c = 24.0f;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = true;

            // Master is US2000C (Rank 20)
            g_stack.modules[0].id = 1;
            g_stack.modules[0].model_type = MODEL_US2000C;
            g_stack.modules[0].fw_version = "V2.8";
            g_stack.modules[0].cell_count = 15;

            // Slave is US5000 (Rank 50) -> Hierarchy Violation!
            g_stack.modules[1].id = 2;
            g_stack.modules[1].model_type = MODEL_US5000;
            g_stack.modules[1].fw_version = "V1.7";
            g_stack.modules[1].cell_count = 16;

            recalculatePhysics();
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"hierarchy_error\",\"message\":\"Hierarchy error simulated (Master US2000C Rank 20 vs Slave US5000 Rank 50)\"}");
        } else if (s == "degraded_soh" || s == "soh") {
            g_stack.module_count = 2;
            g_stack.global_soc = 70.0f;
            g_stack.global_current_a = -1.0f;
            g_stack.global_temp_c = 24.0f;
            g_stack.sim_inverter_enabled = false;
            g_stack.auto_physics = true;

            for (uint8_t i = 0; i < 2; i++) {
                ModuleData &m = g_stack.modules[i];
                m.id = i + 1;
                m.model_type = MODEL_US3000C;
                m.soh_pct = 78.5f - i * 4.0f;
                m.cycle_times = 2150 + i * 220;
                m.euro.soh_pct = m.soh_pct;
                m.euro.energy_thro_kwh = 8540.0f;
            }
            recalculatePhysics();
            saveRackConfigToNvs();
            server.send(200, "application/json", "{\"status\":\"ok\",\"scenario\":\"degraded_soh\",\"message\":\"Degraded battery health (SOH 78.5%, 2150 cycles) simulated\"}");
        } else {
            server.send(400, "application/json", "{\"error\":\"Unknown scenario. Available: normal, imbalance, overvoltage, overtemp, undertemp, deep_discharge, hierarchy_error, degraded_soh\"}");
        }
    }

    void handleApiSet() {
        bool autoPhysicsTriggered = false;
        bool stateChanged = false;

        // 1. Stack Level Parameters
        if (server.hasArg("inverter_sim") || server.hasArg("sim")) {
            String val = server.hasArg("inverter_sim") ? server.arg("inverter_sim") : server.arg("sim");
            g_stack.sim_inverter_enabled = (val == "1" || val == "true");
            stateChanged = true;
        }

        if (server.hasArg("auto_physics") || server.hasArg("physics")) {
            String val = server.hasArg("auto_physics") ? server.arg("auto_physics") : server.arg("physics");
            g_stack.auto_physics = (val == "1" || val == "true");
            stateChanged = true;
        }

        if (server.hasArg("soc") || server.hasArg("stack_soc")) {
            float s = (server.hasArg("soc") ? server.arg("soc") : server.arg("stack_soc")).toFloat();
            if (s >= 0.0f && s <= 100.0f) {
                g_stack.global_soc = s;
                g_stack.sim_inverter_enabled = false;
                autoPhysicsTriggered = true;
                stateChanged = true;
            }
        }

        if (server.hasArg("current") || server.hasArg("stack_current") || server.hasArg("curr") || server.hasArg("i")) {
            String cStr = server.hasArg("current") ? server.arg("current") : (server.hasArg("stack_current") ? server.arg("stack_current") : (server.hasArg("curr") ? server.arg("curr") : server.arg("i")));
            g_stack.global_current_a = cStr.toFloat();
            g_stack.sim_inverter_enabled = false;
            autoPhysicsTriggered = true;
            stateChanged = true;
        }

        if (server.hasArg("temp") || server.hasArg("stack_temp") || server.hasArg("t")) {
            String tStr = server.hasArg("temp") ? server.arg("temp") : (server.hasArg("stack_temp") ? server.arg("stack_temp") : server.arg("t"));
            g_stack.global_temp_c = tStr.toFloat();
            autoPhysicsTriggered = true;
            stateChanged = true;
        }

        if (server.hasArg("spread") || server.hasArg("cell_spread")) {
            String spStr = server.hasArg("spread") ? server.arg("spread") : server.arg("cell_spread");
            g_stack.cell_spread_mv = (uint16_t)spStr.toInt();
            autoPhysicsTriggered = true;
            stateChanged = true;
        }

        if (server.hasArg("modules") || server.hasArg("module_count") || server.hasArg("count")) {
            int cnt = (server.hasArg("modules") ? server.arg("modules") : (server.hasArg("module_count") ? server.arg("module_count") : server.arg("count"))).toInt();
            if (cnt >= 1 && cnt <= MAX_MODULES) {
                for (uint8_t i = g_stack.module_count; i < cnt; i++) {
                    ModuleData &newMod = g_stack.modules[i];
                    newMod.id = i + 1;
                    newMod.model_type = MODEL_US3000C;
                    newMod.fw_version = "V2.8";
                    newMod.barcode = "PPYB2022041800" + String(i + 1 < 10 ? "0" : "") + String(i + 1);
                    newMod.cell_count = 15;
                    newMod.soc = g_stack.global_soc;
                    newMod.soh_pct = 99.0f;
                    newMod.volt_st = "Normal";
                    newMod.curr_st = "Normal";
                    newMod.temp_st = "Normal";
                    newMod.dtemp_st = "Normal";
                    newMod.ctemp_st = "Normal";
                    newMod.mos_temp_st = "Normal";
                    newMod.b_v_st = "Normal";
                    newMod.b_t_st = "Normal";
                    newMod.cycle_times = 120 + i * 10;
                    newMod.euro.soh_pct = 99.0f;
                    newMod.euro.life_expect_days = 5475;
                    newMod.euro.energy_thro_kwh = 1250.0f;
                    newMod.euro.round_trip_eff_pct = 96.5f;
                }
                g_stack.module_count = cnt;
                autoPhysicsTriggered = true;
                stateChanged = true;
            }
        }

        // 2. Module Level Parameters
        int targetM = 0;
        if (server.hasArg("m")) targetM = server.arg("m").toInt();
        else if (server.hasArg("id")) targetM = server.arg("id").toInt();
        else if (server.hasArg("model") || server.hasArg("model_type") || server.hasArg("soh") || server.hasArg("soh_pct") || server.hasArg("cycles") || server.hasArg("volt_st") || server.hasArg("temp_st") || server.hasArg("curr_st") || server.hasArg("cell") || server.hasArg("cell_v") || server.hasArg("bal")) {
            targetM = 1;
        }

        if (targetM >= 1 && targetM <= g_stack.module_count) {
            ModuleData &mod = g_stack.modules[targetM - 1];

            if (server.hasArg("model") || server.hasArg("model_type")) {
                String mName = server.hasArg("model") ? server.arg("model") : server.arg("model_type");
                if (mName == "0" || mName.equalsIgnoreCase("US2000C")) mod.model_type = MODEL_US2000C;
                else if (mName == "1" || mName.equalsIgnoreCase("US3000C")) mod.model_type = MODEL_US3000C;
                else if (mName == "2" || mName.equalsIgnoreCase("US3000D")) mod.model_type = MODEL_US3000D;
                else if (mName == "3" || mName.equalsIgnoreCase("US5000")) mod.model_type = MODEL_US5000;
                else if (mName == "4" || mName.equalsIgnoreCase("UP5000")) mod.model_type = MODEL_UP5000;
                const ModelDescriptor &desc = getModelDescriptor(mod.model_type);
                mod.cell_count = desc.cell_count;
                autoPhysicsTriggered = true;
                stateChanged = true;
            }

            if (server.hasArg("mod_soc")) {
                mod.soc = server.arg("mod_soc").toFloat();
                stateChanged = true;
            }

            if (server.hasArg("soh") || server.hasArg("soh_pct")) {
                float sh = (server.hasArg("soh") ? server.arg("soh") : server.arg("soh_pct")).toFloat();
                mod.soh_pct = sh;
                mod.euro.soh_pct = sh;
                stateChanged = true;
            }

            if (server.hasArg("mod_volt") || server.hasArg("voltage") || server.hasArg("v")) {
                float v = (server.hasArg("mod_volt") ? server.arg("mod_volt") : (server.hasArg("voltage") ? server.arg("voltage") : server.arg("v"))).toFloat();
                if (v < 100.0f) v *= 1000.0f;
                mod.voltage_mv = (int32_t)v;
                stateChanged = true;
            }

            if (server.hasArg("mod_curr")) {
                mod.current_ma = (int32_t)(server.arg("mod_curr").toFloat() * 1000.0f);
                stateChanged = true;
            }

            if (server.hasArg("cycles") || server.hasArg("cycle_times")) {
                mod.cycle_times = (uint32_t)(server.hasArg("cycles") ? server.arg("cycles") : server.arg("cycle_times")).toInt();
                stateChanged = true;
            }

            if (server.hasArg("soh_times")) {
                mod.soh_times = (uint32_t)server.arg("soh_times").toInt();
                stateChanged = true;
            }

            if (server.hasArg("barcode")) {
                mod.barcode = server.arg("barcode");
                stateChanged = true;
            }

            if (server.hasArg("fw") || server.hasArg("fw_version")) {
                mod.fw_version = server.hasArg("fw") ? server.arg("fw") : server.arg("fw_version");
                stateChanged = true;
            }

            // Status strings
            if (server.hasArg("volt_st")) { mod.volt_st = server.arg("volt_st"); stateChanged = true; }
            if (server.hasArg("curr_st")) { mod.curr_st = server.arg("curr_st"); stateChanged = true; }
            if (server.hasArg("temp_st")) {
                mod.temp_st = server.arg("temp_st");
                mod.dtemp_st = mod.temp_st;
                mod.ctemp_st = mod.temp_st;
                stateChanged = true;
            }
            if (server.hasArg("b_v_st")) { mod.b_v_st = server.arg("b_v_st"); stateChanged = true; }
            if (server.hasArg("b_t_st")) { mod.b_t_st = server.arg("b_t_st"); stateChanged = true; }

            // Protections
            if (server.hasArg("bat_ov")) { mod.bat_ov_times = server.arg("bat_ov").toInt(); stateChanged = true; }
            if (server.hasArg("bat_hv")) { mod.bat_hv_times = server.arg("bat_hv").toInt(); stateChanged = true; }
            if (server.hasArg("bat_lv")) { mod.bat_lv_times = server.arg("bat_lv").toInt(); stateChanged = true; }
            if (server.hasArg("bat_uv")) { mod.bat_uv_times = server.arg("bat_uv").toInt(); stateChanged = true; }
            if (server.hasArg("pwr_ov")) { mod.pwr_ov_times = server.arg("pwr_ov").toInt(); stateChanged = true; }
            if (server.hasArg("pwr_hv")) { mod.pwr_hv_times = server.arg("pwr_hv").toInt(); stateChanged = true; }
            if (server.hasArg("pwr_lv")) { mod.pwr_lv_times = server.arg("pwr_lv").toInt(); stateChanged = true; }
            if (server.hasArg("pwr_uv")) { mod.pwr_uv_times = server.arg("pwr_uv").toInt(); stateChanged = true; }
            if (server.hasArg("cot")) { mod.cot_times = server.arg("cot").toInt(); stateChanged = true; }
            if (server.hasArg("cut")) { mod.cut_times = server.arg("cut").toInt(); stateChanged = true; }
            if (server.hasArg("dot")) { mod.dot_times = server.arg("dot").toInt(); stateChanged = true; }
            if (server.hasArg("dut")) { mod.dut_times = server.arg("dut").toInt(); stateChanged = true; }
            if (server.hasArg("coc")) { mod.coc_times = server.arg("coc").toInt(); stateChanged = true; }
            if (server.hasArg("doc")) { mod.doc_times = server.arg("doc").toInt(); stateChanged = true; }
            if (server.hasArg("sc")) { mod.sc_times = server.arg("sc").toInt(); stateChanged = true; }

            // 3. Cell Level Parameters
            int targetC = -1;
            if (server.hasArg("c")) targetC = server.arg("c").toInt();
            else if (server.hasArg("cell")) targetC = server.arg("cell").toInt();

            if (targetC >= 0 && targetC < mod.cell_count) {
                if (server.hasArg("cell_v") || server.hasArg("cell_volt") || server.hasArg("cv") || server.hasArg("v_mv")) {
                    String vStr = server.hasArg("cell_v") ? server.arg("cell_v") : (server.hasArg("cell_volt") ? server.arg("cell_volt") : (server.hasArg("cv") ? server.arg("cv") : server.arg("v_mv")));
                    mod.cells[targetC].voltage_mv = (uint16_t)vStr.toInt();
                    int32_t totV = 0;
                    for (uint8_t ci = 0; ci < mod.cell_count; ci++) totV += mod.cells[ci].voltage_mv;
                    mod.voltage_mv = totV;
                    stateChanged = true;
                }
                if (server.hasArg("cell_bal") || server.hasArg("bal")) {
                    String bStr = server.hasArg("cell_bal") ? server.arg("cell_bal") : server.arg("bal");
                    mod.cells[targetC].balancing = (bStr == "1" || bStr == "true" || bStr == "Y" || bStr == "y");
                    stateChanged = true;
                }
                if (server.hasArg("cell_soh")) {
                    mod.cells[targetC].soh_count = (uint16_t)server.arg("cell_soh").toInt();
                    stateChanged = true;
                }
            }
        }

        if (autoPhysicsTriggered && g_stack.auto_physics) {
            recalculatePhysics();
        }

        bool shouldSave = true;
        if (server.hasArg("save")) {
            String sv = server.arg("save");
            if (sv == "0" || sv == "false") shouldSave = false;
        }
        if (shouldSave && stateChanged && !g_stack.sim_inverter_enabled) {
            saveRackConfigToNvs();
        }

        handleApiStackData();
    }

    void handleApiSettingsSave() {
        if (!checkWebAuth()) return;

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
        }

        // Static IP
        bool staticEn = server.hasArg("ip_static");
        prefs.putBool(NVS_KEY_STATIC_IP_EN, staticEn);
        if (server.hasArg("ip_addr")) prefs.putString(NVS_KEY_STATIC_IP, server.arg("ip_addr"));
        if (server.hasArg("ip_mask")) prefs.putString(NVS_KEY_STATIC_MASK, server.arg("ip_mask"));
        if (server.hasArg("ip_gw")) prefs.putString(NVS_KEY_STATIC_GW, server.arg("ip_gw"));
        if (server.hasArg("ip_dns")) prefs.putString(NVS_KEY_STATIC_DNS, server.arg("ip_dns"));

        // NTP & Timezone
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

        // Web Security & Access Protection
        bool authEn = server.hasArg("auth_en");
        prefs.putBool(NVS_KEY_AUTH_ENABLED, authEn);
        if (server.hasArg("auth_usr") && server.arg("auth_usr").length() > 0) {
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

        server.send(200, "application/json", "{\"status\":\"ok\"}");
        delay(500);
        ESP.restart();
    }

    void handleMetrics() {
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/plain; version=0.0.4; charset=utf-8", "");

        String chunk;
        chunk.reserve(1024);

        auto flushChunk = [&]() {
            if (chunk.length() > 0) {
                server.sendContent(chunk);
                chunk = "";
            }
        };

        SystemDiagnostics diag = SystemStatsManager::getDiagnostics();

        // --- ESP32 System Diagnostics & Telemetry ---
        chunk += "# HELP esp32_uptime_seconds Device uptime in seconds\n";
        chunk += "# TYPE esp32_uptime_seconds counter\n";
        chunk += "esp32_uptime_seconds " + String(diag.uptimeSec) + "\n\n";

        chunk += "# HELP esp32_cpu_usage_percent CPU utilization percentage (0-100)\n";
        chunk += "# TYPE esp32_cpu_usage_percent gauge\n";
        chunk += "esp32_cpu_usage_percent " + String(diag.cpuLoadPct, 1) + "\n\n";

        chunk += "# HELP esp32_cpu_temperature_celsius Internal chip temperature in degrees Celsius\n";
        chunk += "# TYPE esp32_cpu_temperature_celsius gauge\n";
        chunk += "esp32_cpu_temperature_celsius " + String(diag.internalTempC, 1) + "\n\n";

        chunk += "# HELP esp32_cpu_frequency_mhz Current CPU clock frequency in MHz\n";
        chunk += "# TYPE esp32_cpu_frequency_mhz gauge\n";
        chunk += "esp32_cpu_frequency_mhz " + String(diag.cpuFreqMhz) + "\n\n";
        flushChunk();

        chunk += "# HELP esp32_heap_free_bytes Current free heap memory in bytes\n";
        chunk += "# TYPE esp32_heap_free_bytes gauge\n";
        chunk += "esp32_heap_free_bytes " + String(diag.freeHeapBytes) + "\n\n";

        chunk += "# HELP esp32_heap_total_bytes Total heap memory in bytes\n";
        chunk += "# TYPE esp32_heap_total_bytes gauge\n";
        chunk += "esp32_heap_total_bytes " + String(diag.heapTotalBytes) + "\n\n";

        chunk += "# HELP esp32_heap_min_free_bytes Lowest free heap recorded since boot\n";
        chunk += "# TYPE esp32_heap_min_free_bytes gauge\n";
        chunk += "esp32_heap_min_free_bytes " + String(diag.minFreeHeapBytes) + "\n\n";

        chunk += "# HELP esp32_heap_max_alloc_bytes Largest contiguous free block on heap\n";
        chunk += "# TYPE esp32_heap_max_alloc_bytes gauge\n";
        chunk += "esp32_heap_max_alloc_bytes " + String(diag.heapMaxAllocBytes) + "\n\n";

        chunk += "# HELP esp32_heap_fragmentation_percent Heap fragmentation percentage (0-100)\n";
        chunk += "# TYPE esp32_heap_fragmentation_percent gauge\n";
        chunk += "esp32_heap_fragmentation_percent " + String(diag.heapFragmentationPct) + "\n\n";
        flushChunk();

        if (diag.psramTotalBytes > 0) {
            chunk += "# HELP esp32_psram_total_bytes Total PSRAM memory in bytes\n";
            chunk += "# TYPE esp32_psram_total_bytes gauge\n";
            chunk += "esp32_psram_total_bytes " + String(diag.psramTotalBytes) + "\n\n";

            chunk += "# HELP esp32_psram_free_bytes Free PSRAM memory in bytes\n";
            chunk += "# TYPE esp32_psram_free_bytes gauge\n";
            chunk += "esp32_psram_free_bytes " + String(diag.psramFreeBytes) + "\n\n";
            flushChunk();
        }

        chunk += "# HELP esp32_wifi_rssi_dbm WiFi signal strength in dBm\n";
        chunk += "# TYPE esp32_wifi_rssi_dbm gauge\n";
        chunk += "esp32_wifi_rssi_dbm " + String(diag.wifiRssi) + "\n\n";

        chunk += "# HELP esp32_wifi_signal_percent WiFi signal quality percentage (0-100)\n";
        chunk += "# TYPE esp32_wifi_signal_percent gauge\n";
        chunk += "esp32_wifi_signal_percent " + String(diag.wifiSignalPct) + "\n\n";

        chunk += "# HELP esp32_wifi_channel WiFi channel\n";
        chunk += "# TYPE esp32_wifi_channel gauge\n";
        chunk += "esp32_wifi_channel " + String(diag.wifiChannel) + "\n\n";

        chunk += "# HELP esp32_flash_size_bytes Total flash chip size in bytes\n";
        chunk += "# TYPE esp32_flash_size_bytes gauge\n";
        chunk += "esp32_flash_size_bytes " + String(diag.flashSizeBytes) + "\n\n";

        chunk += "# HELP esp32_sketch_size_bytes Flash space consumed by application firmware\n";
        chunk += "# TYPE esp32_sketch_size_bytes gauge\n";
        chunk += "esp32_sketch_size_bytes " + String(diag.sketchSizeBytes) + "\n\n";

        chunk += "# HELP esp32_sketch_free_bytes Free flash space available for OTA updates\n";
        chunk += "# TYPE esp32_sketch_free_bytes gauge\n";
        chunk += "esp32_sketch_free_bytes " + String(diag.sketchFreeBytes) + "\n\n";
        flushChunk();

        chunk += "# HELP esp32_system_info Device system metadata\n";
        chunk += "# TYPE esp32_system_info gauge\n";
        chunk += "esp32_system_info{chip=\"" + diag.chipModel + "\",revision=\"" + String(diag.chipRevision) + "\",cores=\"" + String(diag.cpuCores) + "\",reset_reason=\"" + diag.resetReason + "\",ssid=\"" + diag.wifiSsid + "\",ip=\"" + diag.wifiIp + "\",mac=\"" + diag.wifiMac + "\"} 1\n\n";

        chunk += "# HELP pylon_emulator_info Pylontech BMS Emulator metadata\n";
        chunk += "# TYPE pylon_emulator_info gauge\n";
        chunk += "pylon_emulator_info{version=\"" + String(FIRMWARE_VERSION) + "\",modules=\"" + String(g_stack.module_count) + "\",master_model=\"" + String(getModelDescriptor(g_stack.modules[0].model_type).name) + "\"} 1\n\n";
        flushChunk();

        // End of stream
        server.sendContent("");
    }
};
