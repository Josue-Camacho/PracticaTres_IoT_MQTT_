import json
import logging
import math
import re
import threading
import time
import uuid
from pathlib import Path

from mcp.server import MCPServer
import paho.mqtt.client as mqtt
from pydantic import StrictInt

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger(__name__)

CONFIG_PATH = Path(__file__).resolve().with_name("mqtt_config.json")
DEFAULT_CONFIG = {
    "host": "broker.hivemq.com",
    "port": 1883,
    "distance_topic": "sis234/grupo3/distancia/cm",
    "servo_topic": "sis234/grupo3/servo/cmd",
}


def load_config():
    config = DEFAULT_CONFIG.copy()
    if CONFIG_PATH.exists():
        with CONFIG_PATH.open(encoding="utf-8") as file:
            config.update(json.load(file))
    return config


class MqttBridge:
    def __init__(self, config):
        self.config = config
        self.lock = threading.Lock()
        self.distance = None
        self.received_at = None
        self.client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id="mcp-" + uuid.uuid4().hex[:16],
            protocol=mqtt.MQTTv311,
        )
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.reconnect_delay_set(min_delay=1, max_delay=30)

    def start(self):
        self.client.connect_async(self.config["host"], self.config["port"], 60)
        self.client.loop_start()

    def stop(self):
        self.client.disconnect()
        self.client.loop_stop()

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            logger.error("MQTT: conexión rechazada: %s", reason_code)
            return
        result, _ = client.subscribe(self.config["distance_topic"], qos=0)
        if result != mqtt.MQTT_ERR_SUCCESS:
            logger.error("MQTT: no se pudo solicitar la suscripción: %s", result)
        else:
            logger.info("MQTT conectado; suscripción solicitada")

    def on_disconnect(self, client, userdata, disconnect_flags, reason_code, properties):
        with self.lock:
            self.distance = None
            self.received_at = None
        logger.warning("MQTT desconectado: %s", reason_code)

    def on_message(self, client, userdata, message):
        if message.topic != self.config["distance_topic"] or message.retain:
            return
        try:
            payload = message.payload.decode("ascii")
            if not re.fullmatch(r"[0-9]+(?:\.[0-9]+)?", payload):
                return
            distance = float(payload)
            if not math.isfinite(distance) or not 10 <= distance <= 100:
                return
        except (UnicodeDecodeError, ValueError, OverflowError):
            return
        with self.lock:
            self.distance = distance
            self.received_at = time.monotonic()

    def last_distance(self):
        with self.lock:
            if self.received_at is None:
                return "Lectura no disponible: no hay una medición reciente."
            age = time.monotonic() - self.received_at
            if age > 3:
                return f"Lectura no disponible: la última medición tiene {age:.1f} segundos."
            return f"Distancia: {self.distance:.1f} cm. Antigüedad: {age:.1f} segundos."

    def publish_servo(self, angle):
        if type(angle) is not int or not 0 <= angle <= 180:
            return "Error: el ángulo debe ser un entero entre 0 y 180 grados."
        if not self.client.is_connected():
            return "Error: MQTT no está conectado."
        info = self.client.publish(self.config["servo_topic"], str(angle), qos=0, retain=False)
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            return f"Error al publicar el comando MQTT: {mqtt.error_string(info.rc)}."
        try:
            info.wait_for_publish(timeout=2.0)
            if not info.is_published():
                return "No se confirmó el envío MQTT dentro del plazo; el resultado es incierto."
        except (RuntimeError, ValueError) as exc:
            return f"Error al enviar el comando MQTT: {exc}."
        return f"Comando de {angle}° enviado por MQTT. No hay confirmación del movimiento físico."


bridge = MqttBridge(load_config())
mcp = MCPServer("esp32-mqtt")


@mcp.tool()
def consultar_distancia() -> str:
    """Consulta la última medición reciente del sensor, en centímetros."""
    return bridge.last_distance()


@mcp.tool()
def mover_servo(angulo: StrictInt) -> str:
    """Publica una posición entera de 0 a 180 grados para el servo."""
    return bridge.publish_servo(angulo)


if __name__ == "__main__":
    bridge.start()
    try:
        mcp.run(transport="stdio")
    finally:
        bridge.stop()
