//var socket = io.connect('https://' + document.domain + ':' + location.port);
var socket = io();

function function_onoff() {
    socket.emit("change", ["onoff", 0])
}

function function_temp_up(){
    socket.emit("change", ["temp", 1])
}

function function_temp_down(){
    socket.emit("change", ["temp", -1])
}

socket.on("web", (setting, value) => {
  document.getElementById(setting).innerHTML = value;
  if(setting=="temp"){
    switch(value){
      case 30:
        document.getElementById("button_temp_up").setAttribute("disabled", "disabled");
        break;
      case 29:
        document.getElementById("button_temp_up").removeAttribute("disabled");
        break;
      case 19:
        document.getElementById("button_temp_down").removeAttribute("disabled");
        break;
      case 18:
        document.getElementById("button_temp_down").setAttribute("disabled", "disabled");
        break;
    }
  }
});

socket.on("remote", ir_code => {
  console.log(ir_code);
});
