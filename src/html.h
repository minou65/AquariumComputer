// html.h

#ifndef _HTML_h
#define _HTML_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

const char html_output_styles[] PROGMEM = R"====(
.led {display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 5px; }
.led.off {background-color: grey;}
.led.on {background-color: green;}
.led.delayedoff {background-color: orange;}
.btn-group { display: flex; gap: 5px; flex-wrap: nowrap; }
.output-btn { min-width: 70px; padding: 5px 15px; border-radius: 5px; border: 1px solid #888; background: #eee; cursor: pointer; }
.output-btn.on.active { background: #4caf50; color: #fff; }
.output-btn.off.active { background: #f44336; color: #fff; }
.output-btn.auto.active { background: #03a9f4; color: #fff; }
.brightness-input { width: 50px; margin-left: 10px; text-align: center; }
)====";

const char html_form_end[] PROGMEM = R"=====(
</br><form action='/reboot' method='get'><button type='submit'>Reboot</button></form>
</br><form action='/' method='get' style='display:inline;'>
  <button type='submit'>Home</button>
  </br>
</form>
</br><form action='/post' method='post' style='display:inline;' onsubmit="event.preventDefault(); postReset();">
  <button type='submit'>Reset to factory defaults</button>
</form>
<script>
function postReset() {
    fetch('/post', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'reset=true'
    })
    .then(response => {
        if (response.ok) { window.location.href = '/'; }
    })
    .catch(error => {
        console.error('Reset fehlgeschlagen:', error);
    });
}
</script>
)=====";

const char html_end_template[] PROGMEM = R"=====(
</div></body></html>
)=====";

const char html_script[] PROGMEM = R"=====(
let timeOffsetMs = 0;
let clockIntervalId = null;

function syncServerTime() {
  fetch('/DateTime')
    .then(response => response.text())
    .then(text => {
      const serverTimeStr = text.trim(); // YYYY-MM-DD HH:MM:SS
      const serverTime = new Date(serverTimeStr.replace(' ', 'T'));
      if (isNaN(serverTime.getTime())) return;

      const clientTime = new Date();
      timeOffsetMs = serverTime.getTime() - clientTime.getTime();

      updateDateTime();
      if (clockIntervalId === null) {
        clockIntervalId = setInterval(updateDateTime, 1000);
      }
    })
    .catch(() => { /* ignore */ });
}

function updateDateTime() {
  const now = new Date(Date.now() + timeOffsetMs);
  const timeStr = now.toLocaleTimeString('de-DE', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false
  });
  const el = document.getElementById('DateTimeValue');
  if (el) el.innerHTML = timeStr;
}

// Resync every 10 min
setInterval(syncServerTime, 600000);

function requestData() {
  fetch('/data')
    .then(response => response.json())
    .then(data => updateData(data))
    .catch(error => console.error('Error fetching data:', error));
}

function updateLED(element, status) {
   if (element) {
       element.classList.remove('on', 'off', 'delayedoff');
       element.classList.add(status);
   }
}

function updateData(jsonData) {
    const rssi = document.getElementById('RSSIValue');
    if (rssi) rssi.innerHTML = jsonData.RSSI + "dBm";

    if (jsonData.outputs) {
        const relay = document.getElementById('RelayValue');
        if (relay) relay.innerHTML = jsonData.outputs.relay;

        for (let i = 1; i <= 3; i++) {
            let input = document.getElementById('brightnessCh' + i);
            let onBtn = document.querySelector('#Channel' + i + 'Btns .output-btn.on');
            if (input && !(onBtn && onBtn.classList.contains('active'))) {
                input.value = jsonData.outputs['ch' + i];
            }
        }
    }

    if (jsonData.heater) {
        const temp = document.getElementById('TemperatureValue');
        if (temp) temp.innerHTML = jsonData.heater.currentTemperature + "&#176;C";

        const heater = document.getElementById('HeaterValue');
        if (heater) heater.innerHTML = jsonData.heater.enabled ? "ON" : "OFF";
    }
}

function setOutputButtonState(ch, state) {
  let group = document.getElementById(ch + 'Btns');
  if (group) {
    group.querySelectorAll('.output-btn').forEach(btn => {
      if (btn.getAttribute('data-state') === state) {
        btn.classList.add('active');
      } else {
        btn.classList.remove('active');
      }
    });
  }
}

document.addEventListener('DOMContentLoaded', function() {
  syncServerTime();
  requestData();
  setInterval(requestData, 5000);

  document.querySelectorAll('.btn-group').forEach(group => {
    group.querySelectorAll('.output-btn').forEach(btn => {
      btn.addEventListener('click', function() {
        group.querySelectorAll('.output-btn').forEach(b => b.classList.remove('active'));
        this.classList.add('active');

        let ch = this.getAttribute('data-ch');
        let state = this.getAttribute('data-state');
        let param = '';

        if (ch === '1' || ch === '2' || ch === '3') {
          let input = document.getElementById('brightnessCh' + ch);
          if (input) {
            input.disabled = (state !== 'on');
            if (state === 'on') input.value = 100;
            else if (state === 'off') input.value = 0;

            param = 'ch' + ch + '=' + state;
            if (state === 'on') {
              param += '&brightnessCh' + ch + '=' + input.value;
            }
          }
        } else if (ch === 'relay') {
          param = 'relay=' + state;
        } else if (ch === 'heater') {
          param = 'heater=' + state;
        }

        fetch('/post', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: param
        }).then(response => {
          if (response.ok) window.location.reload();
        });
      });
    });
  });

  for (let i = 1; i <= 3; i++) {
    let input = document.getElementById('brightnessCh' + i);
    let onBtn = document.querySelector('#Channel' + i + 'Btns .output-btn.on');
    if (input) {
      input.addEventListener('change', function() {
        if (onBtn && onBtn.classList.contains('active')) {
          let val = input.value;
          if (val < 0) val = 0;
          if (val > 100) val = 100;
          let data = 'ch' + i + '=on&brightnessCh' + i + '=' + val;
          fetch('/post', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: data
          });
        }
      });
    }
  }
});
)=====";

#endif