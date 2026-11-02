// =====================================================================
// WiFi_Giga_Test
//
// WHAT: A standalone, throwaway sketch that does exactly one thing --
//       connect the Giga R1 WiFi to a WiFi network and print the IP
//       address it gets assigned.
//
// HOW:  Uses the "WiFi" library bundled with the Giga's board package.
//       The Giga's onboard wireless module is a Murata module, and
//       Arduino intentionally gave it the same API shape as their older
//       WiFiNINA-based boards (WiFi.begin(), WiFi.status(), etc.) --
//       but we have NOT used this library before in this project, so
//       we verify it compiles and actually works on real hardware
//       before building anything real on top of it.
//
// WHY:  Same reason we built PWM_Giga_Test before touching
//       BoostConverter.cpp: a brand-new library on a board we haven't
//       used it on is exactly the kind of thing that *looks* like it
//       should just work from the documentation, but might not (wrong
//       library picked up by the IDE, firmware version mismatch on the
//       wireless module, etc.). Easier to find that out here than
//       buried inside the main firmware.
// =====================================================================

#include <WiFi.h>
// WHAT: Pulls in the WiFi library for the Giga.
// HOW:  Installed automatically with the "Arduino Mbed OS Giga Boards"
//       package -- nothing extra to install via Library Manager.
// WHY:  Gives us WiFi.begin(), WiFi.status(), WiFi.localIP(), etc.

// ---------------------------------------------------------------------
// BLOCK 1: network credentials
//
// WHAT: The WiFi network name (SSID) and password the Giga should
//       join.
// HOW:  Plain constants -- fill these in with YOUR network's info
//       before uploading.
// WHY:  This is a local-network-only demo feature (same room/lab), so
//       there's no need for anything fancier than a hardcoded
//       SSID/password here. Just don't commit your real password to a
//       public GitHub repo if this project's repo is ever made public
//       -- swap in a throwaway hotspot name, or keep this file out of
//       version control, if that matters for your situation.
// ---------------------------------------------------------------------
const char WIFI_SSID[]     = "YOUR_WIFI_NETWORK_NAME_HERE";
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD_HERE";

// ---------------------------------------------------------------------
// BLOCK 4: the web server object
//
// WHAT: A WiFiServer listening on port 80 -- the default port browsers
//       use when you type a plain IP address with no ":port" suffix.
// HOW:  Same "WiFi" library as above (WiFiServer, WiFiClient) --
//       declared at file scope (not inside setup()/loop()) so both
//       functions below can use it.
// WHY:  Port 80 means you'll be able to just type the IP address into a
//       browser's address bar with nothing extra appended, which matters
//       for a demo where someone unfamiliar with the project needs to
//       reach it quickly.
// ---------------------------------------------------------------------
WiFiServer server(80);

// ---------------------------------------------------------------------
// BLOCK 2: setup() -- connect once at startup
//
// WHAT: Joins the WiFi network and waits until connected.
// HOW:  WiFi.begin(ssid, password) starts the connection attempt;
//       WiFi.status() tells us when it's actually joined
//       (WL_CONNECTED). We loop and print a breadcrumb each try so we
//       can see progress in the Serial Monitor, instead of a silent
//       wait that looks like a hang.
// WHY:  This is a one-time startup action -- same category as the
//       INA226/display init calls elsewhere in this project that are
//       allowed to block briefly in setup(), since loop() hasn't
//       started yet and there's nothing else to keep running.
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // WHAT: wait for the Serial Monitor to actually be listening.
    // WHY:  without this, early prints can be missed if the monitor
    //       connects a moment after boot -- we hit this exact issue
    //       earlier in this project with other test sketches.
  }

  Serial.println(F("--- WiFi_Giga_Test start ---"));
  Serial.print(F("Connecting to WiFi network: "));
  Serial.println(WIFI_SSID);
  Serial.flush();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // WHAT: starts the connection attempt (non-blocking itself, but we
  //       block right after with our own wait loop below).

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    // WHAT: keep checking connection status until it succeeds.
    // HOW:  print a dot and a short pause each try, so we can see it's
    //       alive and not frozen, and so we don't spam the monitor.
    Serial.print(F("."));
    attempt++;
    if (attempt > 40) {
      // WHAT: safety bailout after ~20 seconds (40 x 500ms) of trying.
      // WHY:  if the SSID/password is wrong or the network isn't in
      //       range, we want a clear failure message instead of
      //       looping forever with no explanation.
      Serial.println();
      Serial.println(F("FAILED to connect after ~20 seconds."));
      Serial.println(F("Check WIFI_SSID / WIFI_PASSWORD, and that the network is 2.4GHz or dual-band (check your router's docs for the Giga's wireless module support)."));
      Serial.flush();
      return; // leave setup() -- loop() will just do nothing below
    }
    delay(500);
    // WHAT: a plain delay() is fine here -- this is one-time startup
    //       code before loop() begins, not the running firmware.
  }

  Serial.println();
  Serial.println(F("WiFi connected!"));
  Serial.print(F("Giga's IP address: "));
  Serial.println(WiFi.localIP());

  // WHAT: start listening for incoming browser connections, now that
  //       WiFi is actually joined (no point starting this before then).
  server.begin();
  Serial.println(F("Web server started on port 80."));
  Serial.println(F("--- open the IP address above in a browser on the SAME network ---"));
  Serial.flush();
}

// ---------------------------------------------------------------------
// BLOCK 3: loop() -- heartbeat only, for now
//
// WHAT: Prints a heartbeat once a second so we can tell the sketch is
//       still alive (not frozen) and see if the connection ever drops.
// WHY:  Matches the breadcrumb-style debugging pattern that worked
//       well for diagnosing earlier "nothing shows in Serial Monitor"
//       issues in this project -- confirms the board is alive even if
//       something later looks wrong.
// ---------------------------------------------------------------------
unsigned long lastHeartbeat = 0;

// ---------------------------------------------------------------------
// BLOCK 5: handleClient() -- replies to one browser connection
//
// WHAT: Reads (and discards) whatever the browser sent, then sends back
//       a trivial, fixed HTML page. Every request gets the same reply
//       for now -- there's no routing by path yet, since the point of
//       this test is just confirming a browser can reach the Giga at
//       all, not building the real dashboard.
// HOW:  A short, BOUNDED wait (up to 1 second) for the request bytes to
//       actually arrive after the connection opens -- a TCP connection
//       completing and the browser's HTTP request bytes showing up
//       aren't the same instant. This wait only happens while handling
//       one real, already-connected client, so it can't stall loop()
//       the way a wait for WiFi to connect in setup() would.
// WHY:  This is the standard shape of a minimal Arduino-style web
//       server response (status line, headers, blank line, body) --
//       tried first since it's the normal documented way to do this,
//       same reasoning as trying Mbed's PwmOut before reaching for
//       direct STM32 HAL register access.
// ---------------------------------------------------------------------
void handleClient(WiFiClient& client) {
  Serial.println(F("Client connected."));   // breadcrumb -- confirms a browser actually reached the board

  unsigned long waitStart = millis();
  while (!client.available() && millis() - waitStart < 1000) {
    // WHAT: give the browser up to 1 second to finish sending its
    //       request before we give up on reading it.
  }

  while (client.available()) {
    client.read();   // WHAT: discard each byte of the request -- we don't parse it yet
  }

  // WHAT: the HTTP response itself -- a status line, two headers, a
  //       required blank line, then the actual page content.
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));     // tells the browser not to keep this connection open
  client.println();                            // the blank line that separates headers from body -- required by HTTP
  client.println(F("<html><body><h1>Hello from the Giga!</h1></body></html>"));

  delay(1);          // WHAT: brief pause so the data above actually goes out before we close the connection
  client.stop();     // WHAT: close this connection -- frees it up so the NEXT request can be accepted
  Serial.println(F("Client disconnected."));
}

void loop() {
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    Serial.print(F("heartbeat | WiFi status: "));
    Serial.println(WiFi.status() == WL_CONNECTED ? F("CONNECTED") : F("NOT CONNECTED"));
  }

  // BLOCK 5: check for an incoming browser connection -- non-blocking.
  // WHAT: server.available() returns immediately either way; an empty/
  //       invalid WiFiClient (its bool conversion is false) means
  //       "nobody's knocking right now", so this never stalls loop()
  //       the way the bounded wait inside handleClient() briefly does
  //       once a real client IS connected.
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}
