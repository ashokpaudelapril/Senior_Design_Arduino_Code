#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include "Display.h"   // Reuses DisplayData -- the dashboard shows exactly what the touchscreen shows

// ---------------------------------------------------------------------------
// WEB DASHBOARD: read-only status page served over the Giga's own WiFi
//
// WHAT: A second "display" for the same data the touchscreen already
//       shows (solar/wind/bus/battery, duty cycle, efficiency,
//       temperatures, fault status) plus the 6 relays' states -- but
//       reachable from any browser on the same WiFi network instead of
//       standing at the physical panel.
//
// HOW:  webDashboard_init() connects to WiFi (Config.h credentials) and
//       starts a web server. webDashboard_update() takes the same
//       DisplayData struct the touchscreen uses, checks for a waiting
//       browser connection, and if one exists, serves a simple
//       auto-refreshing HTML page built from it.
//
// WHY READ-ONLY, DELIBERATELY: unlike the touchscreen's Relays tab, this
//       page has no buttons, forms, or any way to change system state --
//       confirmed in the Product Specification's Safety section: relay
//       control is explicitly excluded from this dashboard's scope and
//       stays touchscreen-only, so no network-originated request can
//       ever switch power on this system. Duty cycle is likewise never
//       settable here, same reasoning as the touchscreen's own manual
//       mode being the only place that happens.
//
// STILL UNVERIFIED ON REAL HARDWARE -- see Config.h's BLOCK 31 comment.
// ---------------------------------------------------------------------------

void webDashboard_init();
void webDashboard_update(const DisplayData& data);

#endif
