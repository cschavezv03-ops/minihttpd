# MiniHTTPd

Servidor HTTP/1.1 basico desarrollado en C para Linux, sin bibliotecas HTTP externas. El proyecto permite estudiar la comunicacion TCP, el procesamiento de solicitudes HTTP y la atencion concurrente de clientes mediante `epoll` en un solo hilo.

## Funcionalidades

- Solicitudes `GET` y entrega de archivos estaticos desde `www/`.
- Archivos HTML, CSS, JavaScript, PNG y JPG/JPEG con el tipo MIME correspondiente.
- Analisis de la linea de solicitud HTTP/1.1 y los encabezados `Host`, `Connection` y `User-Agent`.
- Concurrencia basada en eventos (`epoll`) y sockets no bloqueantes.
- Conexiones persistentes (*keep-alive*) para reutilizar una misma conexion TCP.
- Respuestas HTTP `200`, `400`, `403`, `404`, `405` y `500`.
- Comprobacion de rutas mediante `realpath()` para restringir el acceso a `www/`, validacion de limites de los buffers y construccion acotada de cabeceras con `snprintf()`.

## Requisitos

- Linux (se desarrollo y probo en Fedora).
- GCC y Make.

## Compilacion y ejecucion

Desde la raiz del repositorio:

```bash
make clean && make
./minihttpd
```

El servidor escucha en `localhost:8080`. Tambien puede iniciarse con `make run` despues de compilarlo.

Para limpiar el ejecutable:

```bash
make clean
```

## Pruebas

Con el servidor ejecutandose, abrir en el navegador:

<http://127.0.0.1:8080/>

O consultar los recursos desde otra terminal:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/style.css
curl -i http://127.0.0.1:8080/script.js
curl -o /tmp/image.png http://127.0.0.1:8080/image.png
```

Comprobar respuestas de error:

```bash
curl -i http://127.0.0.1:8080/noexiste.html
curl -i -X POST http://127.0.0.1:8080/
```

Las respuestas esperadas son `404 Not Found` y `405 Method Not Allowed`, respectivamente. El archivo `/` corresponde a `www/index.html`. Los recursos solicitados deben existir en el directorio `www/`.

## Estructura del proyecto

```text
minihttpd/
├── Makefile
├── README.md
├── include/
│   ├── server.h
│   ├── http.h
│   ├── files.h
│   └── mime.h
├── src/
│   ├── main.c
│   ├── server.c
│   ├── http.c
│   ├── files.c
│   └── mime.c
└── www/
    ├── index.html
    ├── style.css
    ├── script.js
    └── image.png
```

- `server.c`: crea y configura el socket TCP de escucha.
- `main.c`: coordina `epoll`, la recepcion de solicitudes y el envio de respuestas y archivos.
- `http.c`: analiza solicitudes y construye cabeceras HTTP.
- `files.c`: resuelve y valida rutas, abre archivos y obtiene su tamano.
- `mime.c`: identifica el tipo de contenido a partir de la extension.
- `www/`: contiene los recursos estaticos publicados.

## Funcionamiento general

El servidor registra el socket de escucha y los clientes en `epoll`. `EPOLLIN` indica cuando se pueden aceptar conexiones o recibir solicitudes, mientras que `EPOLLOUT` permite continuar el envio de una respuesta sin bloquear el hilo. Para una solicitud `GET` valida, el servidor busca el archivo dentro de `www/`, obtiene su tipo MIME y tamano, envia la cabecera HTTP y transmite el contenido mediante `sendfile()`. Con *keep-alive*, puede atender otra solicitud reutilizando la misma conexion TCP.

## Alcance

MiniHTTPd es una implementacion academica para comprender HTTP/1.1 y la concurrencia basada en eventos. Solo implementa `GET` y archivos estaticos; no incluye TLS, aplicaciones dinamicas ni las caracteristicas de seguridad y rendimiento de un servidor web de produccion.

## Referencias

- [RFC 9110 - HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110)
- [RFC 9112 - HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112)
