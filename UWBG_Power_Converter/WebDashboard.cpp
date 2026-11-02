#include <Arduino.h>
#include <WiFi.h>            // WHAT: the Giga's WiFi library -- WiFi.begin(), WiFiServer, WiFiClient
#include "WebDashboard.h"    // WHAT: this module's own public interface
#include "Config.h"          // WHAT: WIFI_SSID, WIFI_PASSWORD, WEB_DASHBOARD_PORT
#include "Relays.h"          // WHAT: RelayId, RELAY_COUNT -- for listing all 6 relays
#include "LoadControl.h"     // WHAT: loadControl_getDesired()/getActual() -- same source Display.cpp's Relays tab reads from

// ---------------------------------------------------------------------------
// BLOCK 1: the web server object
//
// WHAT: A WiFiServer listening on WEB_DASHBOARD_PORT (80 by default).
// WHY:  File-scope (not inside a function) so both webDashboard_init() and
//       webDashboard_update() can use the same server instance -- same
//       pattern already proven in WiFi_Giga_Test.ino.
// ---------------------------------------------------------------------------
static WiFiServer server(WEB_DASHBOARD_PORT);
static bool wifiConnected = false;   // WHAT: remembers whether setup actually succeeded, so update() can skip work cleanly if not

// ---------------------------------------------------------------------------
// BLOCK 2: webDashboard_init()
//
// WHAT: Joins the configured WiFi network, then starts the server.
// HOW:  Identical connection approach to WiFi_Giga_Test.ino -- the
//       standalone sketch where this was first written and is meant to be
//       bench-verified before being trusted here. Blocking is acceptable
//       because this is one-time startup work, same category as
//       sensors_init()/display_init() elsewhere in setup().
// WHY:  A bounded retry (not an infinite wait) means a missing/wrong
//       network doesn't hang the whole board forever at boot -- if it
//       fails, the rest of the firmware (sensors, charge control, the
//       physical display) still comes up normally; only the web
//       dashboard is unavailable, which is the right failure mode for a
//       feature that's explicitly a bonus view, not required operation.
// ---------------------------------------------------------------------------
void webDashboard_init() {
  Serial.print(F("WebDashboard: connecting to WiFi network "));
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);                 // one-time startup wait, same as other init() calls
    Serial.print(F("."));
    attempt++;
    if (attempt > 40) {         // ~20 seconds -- same bailout as WiFi_Giga_Test.ino
      Serial.println();
      Serial.println(F("WebDashboard: WiFi connection FAILED -- dashboard will not be available. Check WIFI_SSID/WIFI_PASSWORD in Config.h."));
      wifiConnected = false;
      return;                   // leave the rest of setup() to continue normally without WiFi
    }
  }

  Serial.println();
  Serial.print(F("WebDashboard: WiFi connected. Open this in a browser on the same network: http://"));
  Serial.println(WiFi.localIP());

  server.begin();
  wifiConnected = true;
}

// ---------------------------------------------------------------------------
// BLOCK 3: small HTML-sending helpers
//
// WHAT: Each helper writes one piece of the page directly to the client
//       connection, rather than building one large String in RAM first.
// WHY:  Streaming the response out piece by piece keeps memory use flat
//       regardless of how much content the page has -- the Giga has
//       plenty of RAM for a page this small either way, but streaming is
//       the standard, safer habit for embedded HTTP responses and costs
//       nothing extra to do here.
// ---------------------------------------------------------------------------

// Writes one row of a power-reading table (label, V, A, W).
static void sendPowerRow(WiFiClient& client, const char* label, const PowerReading& r) {
  client.print(F("<tr><td>"));
  client.print(label);
  client.print(F("</td><td>"));
  client.print(r.busVoltage_V, 2);
  client.print(F(" V</td><td>"));
  client.print(r.current_A, 2);
  client.print(F(" A</td><td>"));
  client.print(r.power_W, 2);
  client.print(F(" W</td></tr>"));
}

// Writes one row of the relay-state table, reusing the exact same
// desired/actual/WAIT logic the touchscreen's Relays tab already uses --
// see Display.cpp's drawRelayButton() for the original version of this.
static void sendRelayRow(WiFiClient& client, const char* label, RelayId relay) {
  bool desired = loadControl_getDesired(relay);
  bool actual  = loadControl_getActual(relay);

  const char* stateText;
  const char* stateColor;
  if (!desired) {
    stateText = "OFF";
    stateColor = "#888888";       // grey -- matches the touchscreen's OFF color intent
  } else if (actual) {
    stateText = "ON";
    stateColor = "#2e8b2e";       // green
  } else {
    stateText = "WAIT";
    stateColor = "#b8860b";       // amber -- matches the touchscreen's WAIT color intent
  }

  client.print(F("<tr><td>"));
  client.print(label);
  client.print(F("</td><td style=\"color:"));
  client.print(stateColor);
  client.print(F(";font-weight:bold\">"));
  client.print(stateText);
  client.print(F("</td></tr>"));
}

// ---------------------------------------------------------------------------
// BLOCK 4: webDashboard_update()
//
// WHAT: Checks for a waiting browser connection; if one exists, serves
//       the full status page built from the DisplayData passed in.
// HOW:  Same non-blocking server.available() check as WiFi_Giga_Test.ino,
//       called from the main .ino at the same point the touchscreen
//       display is refreshed, reusing the identical DisplayData struct
//       that was just built for it -- no duplicate sensor reads.
// WHY READ-ONLY: this function never reads anything from the client
//       request, and never calls loadControl_setDesired() or
//       boost_setDutyCycle() or anything else that could change system
//       state -- it only ever writes a page out. There is no code path
//       here, intentionally, by which a network request could affect
//       the running system.
// ---------------------------------------------------------------------------
void webDashboard_update(const DisplayData& data) {
  if (!wifiConnected) {
    return;   // WiFi never came up at boot -- nothing to serve, nothing to check
  }

  WiFiClient client = server.available();   // non-blocking: empty/invalid if nobody's connecting right now
  if (!client) {
    return;
  }

  // Give the browser a brief, bounded moment to finish sending its
  // request before we respond -- same reasoning as WiFi_Giga_Test.ino's
  // handleClient(). We don't parse the request at all (every request
  // gets the same page), so this is just drain-and-discard.
  unsigned long waitStart = millis();
  while (!client.available() && millis() - waitStart < 1000) {
    // bounded wait for one already-connected client; doesn't stall loop()
  }
  while (client.available()) {
    client.read();
  }

  // --- HTTP response headers ---
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println();

  // --- Page head: a meta-refresh tag reloads the whole page every 2
  //     seconds, which is the simplest possible way to keep the data
  //     current without writing any JavaScript -- appropriate for a
  //     bonus monitoring view, not a requirement to make fancy. ---
  client.print(F("<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"2\">"));
  client.print(F("<title>UWBG Power Converter</title>"));
  client.print(F("<style>body{font-family:sans-serif;background:#111;color:#eee;padding:16px}"
                 "h1{font-size:1.3em}table{border-collapse:collapse;margin-bottom:20px}"
                 "td{padding:4px 12px;border-bottom:1px solid #333}"
                 ".fault{color:#e05050;font-weight:bold}.ok{color:#2e8b2e;font-weight:bold}</style>"));
  client.print(F("</head><body>"));
  client.print(F("<h1>UWBG Power Converter -- Live Status</h1>"));

  // --- Fault status, impossible to miss at the top of the page ---
  client.print(F("<p>Status: "));
  if (data.faulted) {
    client.print(F("<span class=\"fault\">FAULT</span> -- "));
    client.print(data.faultReason);
  } else {
    client.print(F("<span class=\"ok\">OK</span>"));
  }
  client.print(F("</p>"));

  // --- Power readings table ---
  client.print(F("<table><tr><th>Point</th><th>Voltage</th><th>Current</th><th>Power</th></tr>"));
  sendPowerRow(client, "Solar", data.solar);
  sendPowerRow(client, "Wind", data.wind);
  sendPowerRow(client, "Common Bus", data.commonBus);
  sendPowerRow(client, "Battery", data.battery);
  client.print(F("</table>"));

  // --- Duty cycle, efficiency, temperatures ---
  client.print(F("<table>"));
  client.print(F("<tr><td>Duty Cycle</td><td>"));
  client.print(data.dutyPercent, 1);
  client.print(F(" %</td></tr>"));
  client.print(F("<tr><td>Efficiency</td><td>"));
  client.print(data.efficiencyPercent, 1);
  client.print(F(" %</td></tr>"));
  client.print(F("<tr><td>GaN Switch Temp</td><td>"));
  if (data.temps.valid) {
    client.print(data.temps.ganSwitchTemp_C, 1);
    client.print(F(" C"));
  } else {
    client.print(F("--"));
  }
  client.print(F("</td></tr><tr><td>Inductor Temp</td><td>"));
  if (data.temps.valid) {
    client.print(data.temps.inductorTemp_C, 1);
    client.print(F(" C"));
  } else {
    client.print(F("--"));
  }
  client.print(F("</td></tr></table>"));

  // --- Relay states -- view-only, matching the touchscreen's Relays tab exactly ---
  client.print(F("<table><tr><th>Relay</th><th>State</th></tr>"));
  sendRelayRow(client, "Solar", RELAY_SOLAR);
  sendRelayRow(client, "Wind", RELAY_WIND);
  sendRelayRow(client, "Data Center Load", RELAY_LOAD_DATACENTER);
  sendRelayRow(client, "EV Charging Load", RELAY_LOAD_EV);
  sendRelayRow(client, "LED Lamp Load", RELAY_LOAD_LED_LAMP);
  sendRelayRow(client, "Other Load", RELAY_LOAD_OTHER);
  client.print(F("</table>"));

  client.print(F("<p style=\"color:#888;font-size:0.85em\">Read-only view. Duty cycle and relay control are only ever set at the physical touchscreen, never over the network.</p>"));
  client.print(F("</body></html>"));

  delay(1);           // same brief pause as WiFi_Giga_Test.ino, so the data above actually goes out before closing
  client.stop();       // close this connection; frees it for the next request
}
