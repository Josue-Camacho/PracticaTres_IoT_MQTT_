Servidor MCP para el ESP32

El servidor usa stdio para Claude Desktop y MQTT TCP en broker.hivemq.com:1883, sin usuario, contraseña ni TLS. El broker es público: los mensajes y comandos pueden ser observados o enviados por otros clientes. Utiliza solo datos de prueba.
Instalación manual en Windows

Se necesita Python 3.10 o posterior. Desde PowerShell, en esta carpeta:

py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt

Se instalan el SDK oficial mcp==2.2.0 y paho-mqtt==2.1.0. Para cambiar el broker o los tópicos, copia mqtt_config.example.json como mqtt_config.json y edítalo. Si no existe, se usan los valores del ejemplo automáticamente. Las rutas se resuelven junto a server.py, independientemente del directorio desde el que Claude inicie el proceso.
Claude Desktop

    Abre claude_desktop_config.example.json y sustituye ambas rutas de ejemplo por las rutas absolutas de tu .venv\Scripts\python.exe y de server.py. En JSON usa barras / o barras invertidas dobles \\.
    En Claude Desktop abre Settings → Developer → Edit Config. Agrega la entrada esp32-mqtt del ejemplo dentro de mcpServers en claude_desktop_config.json; conserva las demás entradas existentes. No sustituyas todo el archivo si ya contiene otros servidores.
    Guarda el archivo y sal completamente de Claude Desktop, incluido el icono de la bandeja de Windows. Abre Claude Desktop nuevamente. Claude inicia el servidor automáticamente; no se necesita abrirlo en una terminal.
    Revisa la lista de herramientas del chat para encontrar consultar_distancia y mover_servo. Con el ESP32 conectado y publicando, pide «consulta la distancia»; después pide «mueve el servo a 90 grados» y aprueba el uso de la herramienta si Claude lo solicita.

Funcionamiento

Estado de la integración: Claude Desktop reconoce e inicia el servidor y ya invocó consultar_distancia con el ESP32 desconectado. Quedan pendientes la consulta de una lectura real y el movimiento físico mediante Claude; RF6 aún no está verificado de extremo a extremo.

    consultar_distancia() devuelve la última distancia válida y su antigüedad. La lectura no está disponible si no se recibió una medición en los últimos tres segundos. Se descartan mensajes retenidos y valores fuera del rango de trabajo del firmware (10–100 cm).
    mover_servo(angulo) acepta un entero de 0 a 180 y publica en sis234/grupo3/servo/cmd con QoS 0 y retain desactivado. La confirmación indica el envío MQTT, no que el servo haya alcanzado la posición.
    El servidor se suscribe a sis234/grupo3/distancia/cm y reintenta la conexión MQTT automáticamente. Una desconexión invalida la lectura anterior. Si MQTT no está conectado, el comando devuelve un error.

Si las herramientas no aparecen, comprueba las rutas, el entorno virtual, el JSON y que Claude se haya cerrado completamente antes de reiniciarlo. Los mensajes de registro se envían a stderr, sin interferir con stdio.
