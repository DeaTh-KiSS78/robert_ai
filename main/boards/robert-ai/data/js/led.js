    // actualizare status LED-uri
    updateLEDStatus();
 
 function stopLEDs() {
    fetch('/stopleds')
        .then(r => r.text())
        .then(t => {
            alert('LED-uri oprite');
            updateLEDStatus();
        });
}

function startLEDs() {
    fetch('/startleds')
        .then(r => r.text())
        .then(t => {
            alert('LED-uri pornite');
            updateLEDStatus();
        });
}

function updateLEDStatus() {
    fetch('/ledstatus')
        .then(r => r.text())
        .then(status => {
            document.getElementById('ledStatus').textContent = 'Status: ' + (status === 'running' ? 'Pornite' : 'Oprite');
        });
}
