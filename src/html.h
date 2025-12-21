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
.btn-group { display: flex; gap: 5px; }
.output-btn { padding: 5px 15px; border-radius: 5px; border: 1px solid #888; background: #eee; cursor: pointer; }
.output-btn.active { background: #4caf50; color: #fff; }
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

