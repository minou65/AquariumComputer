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
.output-value-placeholder { min-width: 40px; display: inline-block; visibility: hidden;}
)====";

const char html_form_end[] PROGMEM = R"=====(
</br><form action='/reboot' method='get'><button type='submit'>Reboot</button></form>
</br><a href='/'>Home</a>
</br><a href='#' onclick="postReset()">Reset to factory defaults</a>
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
requestDateTime();
setInterval(requestDateTime, 1000);

function requestDateTime() { 
   var xhttp = new XMLHttpRequest();
   xhttp.onreadystatechange = function() {
       if (this.readyState == 4 && this.status == 200) {
           document.getElementById('DateTimeValue').innerHTML = this.responseText;
       }
   };
   xhttp.open('GET', 'DateTime', true);
   xhttp.send(); 
}

function updateLED(element, status) {
   if (element) {
       element.classList.remove('on', 'off', 'delayedoff');
       element.classList.add(status);
   }
}

function updateData(jsonData) {
    document.getElementById('RSSIValue').innerHTML = jsonData.RSSI + "dBm";
    if (jsonData.outputs) {
        document.getElementById('Channel1Value').innerHTML = jsonData.outputs.ch1 + "%";
        document.getElementById('Channel2Value').innerHTML = jsonData.outputs.ch2 + "%";
        document.getElementById('Channel3Value').innerHTML = jsonData.outputs.ch3 + "%";
        document.getElementById('RelayValue').innerHTML = jsonData.outputs.relay;
    }
    if (jsonData.heater) {
        document.getElementById('TemperatureValue').innerHTML = jsonData.heater.currentTemperature + "&#176;C";
        document.getElementById('HeaterValue').innerHTML = jsonData.heater.enabled ? "ON" : "OFF";
    }
}


document.addEventListener('DOMContentLoaded', function() {
  document.querySelectorAll('.btn-group').forEach(group => {
    group.querySelectorAll('.output-btn').forEach(btn => {
      btn.addEventListener('click', function() {
        // Remove active from all buttons in this group
        group.querySelectorAll('.output-btn').forEach(b => b.classList.remove('active'));
        // Set active on clicked button
        this.classList.add('active');
        // Send state to backend via POST to /post
        let ch = this.getAttribute('data-ch');
        let state = this.getAttribute('data-state');
        let param = '';
        if (ch === '1') param = 'ch1=' + state;
        else if (ch === '2') param = 'ch2=' + state;
        else if (ch === '3') param = 'ch3=' + state;
        else if (ch === 'relay') param = 'relay=' + state;
        else if (ch === 'heater') param = 'heater=' + state;
        fetch('/post', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: param
        }).then(response => {
          if (response.ok) window.location.reload();
        });
        // Helligkeitsfeld aktivieren/deaktivieren
        if (ch === '1' || ch === '2' || ch === '3') {
          let input = document.getElementById('brightnessCh' + ch);
          if (input) {
            input.disabled = (state !== 'on');
            if (state !== 'on') input.value = 0;
          }
        }
      });
    });
  });

  // Helligkeitsfelder: POST bei Änderung, nur wenn ON
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
)=====";


#endif

