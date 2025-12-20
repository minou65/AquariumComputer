// html.h

#ifndef _HTML_h
#define _HTML_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

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
)=====";


#endif

