# justctf 2025 private

# HTTP Signing

It is a MITM attack [RFC9421](https://www.rfc-editor.org/rfc/rfc9421.html).
Essentially you have to generate a special RSA key that can verify given (EdDSA) signature.

Attack is described in "Digital Signatures Do Not Guarantee Exclusive Ownership" and the [RFC](https://www.rfc-editor.org/rfc/rfc9421.html#name-signing-signature-values), so nothing super hard to figure out.

However, the whole setup is a bit complicated, so the task should be interesting enough.

It's likely that there were similar crypto challenges in the past as the attack is somehow known,
but I haven't seen it in the context of HTTP signing yet.

### Setup

Alice - HTTP client. Players connect to Alice and can provide arbitrary HTTP Proxy that Alice will use.
This service reads/prints to stdin/stdout, and should be setup with socat or nsjail to be exposed on a TCP port.
It must be able to connect to external URLs for the proxy to work (no egress filtering).

Bob - HTTP server. Should be set behind TLS reverse proxy (just so there is HTTPS, not strictly necessary).
Alice sends requests to Bob. Since players can provide the proxy, players are in MITM position.

The docker-compose generates EdDSA key for Bob. It should be mostly constant (i.e. do not change for every request).
Alice generates a new key for herself every time a player connects.

Both scripts use web-both-auth project that Docker pulls from GitHub and applies a small patch.

**TODO: create production flag**

TODO: timeouts for Alice and Bob.

TODO: Alice could protect from SSRF attacks by validating the proxy to be non-local.

### Run task

```
cd task
docker compose up
```

### Run solver

```
cd solver
docker build . -t mallory
docker run --rm -p1338:1338 mallory
```

It may take a minute or two to solve. Sometimes it fails, then just re-run (Alice generates new keys for every connection, some keys are harder to break).

Idk why the script doesn't print debug data when run from Docker...
