

'use strict';

var bufferSize = 1024;
var audioContext;
var microphone;
//var wsUri = "wss://dafang.com:7681";
var wsUri; // = "192.168.1.18:7681";
var SecurePort = "7681";
var UnsecurePort = "7682";
var connected = false;
var WsConnection;
var isResampleNeeded = false;
var myPCMProcessingNode ;
var isStarted = false;
var resamplerObj;

var outSampleRate = 8000;
var outVolume = 100;


var errorCallback = function(e) {
    alert("Error in getUserMedia: " + e);
};

function handlePWM(e){  }

function wsOpen(){
    connected = true;
    console.log("connection made.");
    var configString = "ServerSetValues [" + outSampleRate.toString() + "," + outVolume.toString() + "]";
    console.log("Configstring is ="+configString);
    WsConnection.send(configString);
}

function wsError(error){
    connected = false;
    console.log('WebSocket Error ' + error.data);
    stop()
}

function WsClose(closeEvent){
    stopAudio()
    connected = false;
    console.log('WS connection closed --- Code: ' + closeEvent.code + ' --- reason: ' + closeEvent.reason);
}

function stopAudio()
{
    window.localStream.getAudioTracks()[0].stop();
    microphone = null;
    resamplerObj = null;
}

///////////////////////////////////////

var audioInputSelect = document.querySelector('select#audioSource');
var selectors = [audioInputSelect];


function gotDevices(deviceInfos) {
  // Handles being called several times to update labels. Preserve values.
  var values = selectors.map(function(select) {
    return select.value;
  });
  selectors.forEach(function(select) {
    while (select.firstChild) {
      select.removeChild(select.firstChild);
    }
  });
  for (var i = 0; i !== deviceInfos.length; ++i) {
    var deviceInfo = deviceInfos[i];
    var option = document.createElement('option');
    option.value = deviceInfo.deviceId;
    if (deviceInfo.kind === 'audioinput') {
      option.text = deviceInfo.label ||
          'microphone ' + (audioInputSelect.length + 1);
      audioInputSelect.appendChild(option);
    }
  }
  selectors.forEach(function(select, selectorIndex) {
    if (Array.prototype.slice.call(select.childNodes).some(function(n) {
      return n.value === values[selectorIndex];
    })) {
      select.value = values[selectorIndex];
    }
  });
}

function getAllDevices()
{
    navigator.mediaDevices.enumerateDevices().then(gotDevices).catch(handleError);
}

/*
function start() {
  if (window.stream) {
    window.stream.getTracks().forEach(function(track) {
      track.stop();
    });
  }
  var audioSource = audioInputSelect.value;
  var videoSource = videoSelect.value;
  var constraints = {
    audio: {deviceId: audioSource ? {exact: audioSource} : undefined},
    video: {deviceId: videoSource ? {exact: videoSource} : undefined}
  };
  navigator.mediaDevices.getUserMedia(constraints).
      then(gotStream).then(gotDevices).catch(handleError);
}
*/
/////////////////////////////////////////////////////
function start()
{
    console.log("Start");
    try {
        window.AudioContext = window.AudioContext || window.webkitAudioContext;
        audioContext = new AudioContext();
    } catch(e) {
        alert('Web Audio API is not supported in this browser');
    }

    // Check if there is microphone input.
    try {
        navigator.getUserMedia = navigator.getUserMedia ||  navigator.webkitGetUserMedia || navigator.mozGetUserMedia ||  navigator.msGetUserMedia;
      var hasMicrophoneInput = (navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia || navigator.msGetUserMedia);

    } catch(e) {
        alert("getUserMedia() is not supported in your browser");
    }

    myPCMProcessingNode = audioContext.createScriptProcessor(bufferSize, 1, 1);


    myPCMProcessingNode.onaudioprocess = function(e)
    {
        var input = e.inputBuffer.getChannelData(0);
        if (isResampleNeeded == true)
        {
            //var buffer = inputSample;
            var resampledBuffer = resamplerObj.resampler(input);
            var resamplerObj = new Resampler(48000, 8000, 1, input.length, false);
            let Uint16Buf = new Uint16Array(resampledBuffer.length);

            for (let i=0;i<resampledBuffer.length;i++){

                let s = Math.max(-1, Math.min(1, resampledBuffer[i]));
                s = s < 0 ? s * 0x8000 : s * 0x7FFF;
                Uint16Buf[i] = s;
            }

            if(connected == true){
                WsConnection.send(Uint16Buf);
            }


        }
        else
        {
            let Uint16Buf = new Uint16Array(input.length);
            for (let i=0;i<input.length;i++){

                let s = Math.max(-1, Math.min(1, input[i]));
                s = s < 0 ? s * 0x8000 : s * 0x7FFF;
                Uint16Buf[i] = s;
            }

            if(connected == true){
                WsConnection.send(Uint16Buf);
            }
        }
    }



    if (location.protocol != 'https:')
    {
        wsUri = "ws://" + window.location.hostname +":"+ UnsecurePort;
    }
    else
    {
        wsUri = "wss://" + window.location.hostname + ":"+ SecurePort;
    }

    console.log(wsUri);

    var audioSource = audioInputSelect.value;
    var constraints = {
        audio: {deviceId: audioSource ? {exact: audioSource} : undefined}
      };


    navigator.getUserMedia(constraints, function(stream) {
        window.localStream = stream
        //var ctx = new AudioContext();

        microphone = audioContext.createMediaStreamSource(stream);
        microphone.connect(myPCMProcessingNode);
        myPCMProcessingNode.connect(audioContext.destination);

        var gainNode = audioContext.createGain();
        microphone.connect(gainNode);
        gainNode.connect(myPCMProcessingNode);
        document.getElementById('volume').onchange = function() {
                gainNode.gain.value = this.value; // Any number between 0 and 1.
                console.log("Volume="+this.value);
        };

    }, errorCallback);

    WsConnection = new WebSocket(wsUri);
    WsConnection.onmessage = handlePWM;
    WsConnection.onerror = wsError;
    WsConnection.onopen = wsOpen;
    WsConnection.onclose = WsClose;

    $("#mic").attr('src','mic-sel.png');
}



function stop()
{
    console.log("Stop");
    stopAudio();
    if (connected == true)
        WsConnection.close();
    connected = false;
    $("#mic").attr('src','mic.png');
    isStarted = false;
}

function startorstop()
{
    if (isStarted == true)
    {
        stop();
        isStarted = false;
    } else {
        start();
        isStarted = true;
    }

}
