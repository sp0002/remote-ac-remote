import os

from flask import Flask, render_template, redirect, url_for, flash, request
from flask_socketio import SocketIO, emit, disconnect
from flask_login import LoginManager, current_user, login_user, logout_user, UserMixin

from flask_wtf import FlaskForm
from wtforms import StringField, PasswordField
from wtforms.validators import InputRequired, Length

from dotenv import load_dotenv

from argon2 import PasswordHasher, exceptions

app = Flask(__name__)
socketio = SocketIO(app)
# socketio = SocketIO(app, logger=True, engineio_logger=True)  # For debugging.
app.secret_key = os.urandom(24)

login_manager = LoginManager()
login_manager.init_app(app)

load_dotenv()

ph = PasswordHasher(time_cost=3, memory_cost=1048576, parallelism=4)


state = {
    "onoff": os.environ.get("RR_DEFAULT_ON_OFF"),
    "temp": int(os.environ.get("RR_DEFAULT_TEMP")),
}

last_on = False


def get_ir_code():
    if state["onoff"] == "OFF":
        return "0x88C0051"
    else:
        ir_code = "100010000000"
        if last_on:
            ir_code += "1000"  # Code for when remote control is already on "On".
        else:
            ir_code += "0000"
        temp_to_code = f"{state['temp']-15:b}"
        ir_code += "0"*(4-len(temp_to_code)) + temp_to_code
        ir_code += "0100"  # Fan speed 5.
        return f"{hex(int(ir_code, 2))}"


class User(UserMixin):
    def __init__(self, u_id):
        self.id = u_id


@login_manager.user_loader
def load_user(u_id):
    return User(u_id)


class LoginForm(FlaskForm):
    username = StringField("username", validators=[InputRequired(), Length(min=6)])
    password = PasswordField("Password", validators=[InputRequired(), Length(min=10)])


@app.route("/", methods=["GET", "POST"])
def index():
    if current_user.is_authenticated:
        return render_template("index.html", state=state)
    return redirect(url_for("login"))


@app.route("/login", methods=["GET", "POST"])
def login():
    if current_user.is_authenticated:
        return redirect(url_for("index"))
    form = LoginForm()
    if form.validate_on_submit():
        # Verify password hash first.
        # This is so that someone who puts in the wrong username will have the same delay in the response from the server.
        try:
            ph.verify(os.environ.get("RR_PASS"), form.password.data)  # Password123
        except exceptions.VerifyMismatchError or exceptions.VerificationError or exceptions.InvalidHash:
            pass
        else:
            if form.username.data == os.environ.get("RR_USER"):
                login_user(User(u_id=0))
                return redirect(url_for("index"))
        flash("Login information is incomplete or incorrect.")
    return render_template("login.html", form=form)


@app.route("/logout", methods=["GET", "POST"])
def logout():
    if current_user.is_authenticated:
        logout_user()
    else:
        return 'no', 403
    return redirect(url_for("login"))


@app.route("/token_enter", methods=["POST"])
def token_enter():
    token = request.form.get("token", None)
    if token == os.environ.get("RR_TOKEN"):
        login_user(User(u_id=0))
    return "OK", 200


@socketio.on("connect")
def connect_handler():
    if current_user.is_authenticated:
        emit("remote", get_ir_code(), broadcast=False)
        for i in state:
            emit("web", (i, state[i]), broadcast=False)
    else:
        emit("hi", "403", broadcast=False)
        disconnect()


@socketio.on("change")
def handle_change(change):
    if current_user.is_authenticated:
        global last_on
        if change[0] == "onoff":
            if state["onoff"] == "OFF":
                state["onoff"] = "ON"
                last_on = False
            else:
                state["onoff"] = "OFF"
                last_on = True
            emit("web", ("onoff", state["onoff"]), broadcast=True)
        if change[0] == "temp":
            state["temp"] += change[1]
            if state["temp"] < 18:
                state["temp"] = 18  # Minimum temperature of air-conditioner
            elif state["temp"] > 30:
                state["temp"] = 30  # Maximum temperature of air-conditioner
            last_on = (state["onoff"] == "ON")
            emit("web", ("temp", state["temp"]), broadcast=True)
        emit("remote", get_ir_code(), broadcast=True)
    else:
        disconnect()


if __name__ == "__main__":
    socketio.run(app, "0.0.0.0", 443, ssl_context=("cert.pem", "key.pem"), debug=True)
