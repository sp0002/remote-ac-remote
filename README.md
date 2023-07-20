# remote-ac-remote
Controlling air-con through the web.

## Web app
Uses Flask, SocketIO for the web side.

## Microcontroller
Uses a ESP-01 (ESP8266 chip) to connect to the Flask web app, then sends the required data to an Arduino to control the air conditioner.



# Get started running

## IR codes
The IR codes in this repo are for my LG air conditioner, other air conditioners may have different IR codes.
You can find the IR codes of your remote control with ir-checker.ino with the circuit:
![IR checker circuit](img/receiver.png)
You could also use other microcontrollers, such as the ESP32.


## Testing the web app
Ideally, you should use a python virtual environment.
To run Gunicorn, a UNIX-based or UNIX-like OS is required. Virtual environment running Linux could be used to run Gunicorn if on Windows.
Create one with `python -m venv remote` within the web-app directory.
Activate it `. remote/bin/activate`.

pip install the items in requirements.txt `pip install -r requirements.txt`.

Run the flask server using the Gunicorn WSGI server.
`gunicorn --worker-class eventlet -w 1 --log-level debug --bind 0.0.0.0:5000 app:app`

Test the flask server by going to the ip address of the server at port 5000, for example localhost:5000.


## Wiring up the ESP-01 and uploading the ESP-01 codes

