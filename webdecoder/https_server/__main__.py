# based on https://gist.github.com/dergachev/7028596
# Generate self signed certificate server.pem:
# openssl req -new -x509 -keyout server.pem -out server.pem -days 365 -nodes

from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl
from pathlib import Path

port = 8001

httpd = HTTPServer(("localhost", port), SimpleHTTPRequestHandler)
ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ssl_context.load_cert_chain(Path(__file__).parent / "server.pem")
httpd.socket = ssl_context.wrap_socket(
    httpd.socket,
    server_side=True,
)

print(f"Serving on https://localhost:{port}")
httpd.serve_forever()
