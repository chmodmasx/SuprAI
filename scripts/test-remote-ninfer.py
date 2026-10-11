#!/usr/bin/env python3
"""Bounded external smoke test of the user's OpenAI-compatible NInfer endpoint.

No third-party dependencies. Synthetic prompts only; no secrets or completions
are printed. Never retries generation automatically after an ambiguous failure.
"""
from __future__ import annotations

import argparse
import json
import os
import secrets
import sys
import urllib.error
import urllib.request
from urllib.parse import urlparse


class SmokeFailure(Exception):
    pass


def api_base() -> str:
    base = os.environ.get("NINFER_BASE_URL", "").rstrip("/")
    parsed = urlparse(base)
    if parsed.scheme != "https" or parsed.hostname != "ninfer.minilinux.win" or parsed.path not in ("/v1", "/v1/"):
        raise SmokeFailure("El endpoint debe ser https://ninfer.minilinux.win/v1")
    if parsed.username or parsed.password or parsed.query or parsed.fragment or parsed.port:
        raise SmokeFailure("URL inválida: credenciales, puerto o parámetros no admitidos")
    return base


def get_json(url: str, api_key: str | None = None) -> dict:
    headers = {"Accept": "application/json", "User-Agent": "SuprAI-remote-smoke/1"}
    if api_key:
        headers["Authorization"] = "Bearer " + api_key
    request = urllib.request.Request(url, headers=headers, method="GET")
    try:
        with urllib.request.urlopen(request, timeout=25) as response:
            if response.status != 200:
                raise SmokeFailure(f"GET: HTTP {response.status}")
            raw = response.read(1_000_001)
            if len(raw) > 1_000_000:
                raise SmokeFailure("Respuesta JSON inesperadamente grande")
            body = json.loads(raw)
            if not isinstance(body, dict):
                raise SmokeFailure("Respuesta JSON no es un objeto")
            return body
    except urllib.error.HTTPError as exc:
        raise SmokeFailure(f"GET: HTTP {exc.code}") from None
    except (urllib.error.URLError, OSError, ValueError, json.JSONDecodeError) as exc:
        raise SmokeFailure(f"GET: conexión o JSON inválido ({type(exc).__name__})") from None


def chat_stream(base: str, api_key: str, model: str, messages: list[dict]) -> str:
    payload = {
        "model": model,
        "messages": messages,
        "stream": True,
        "enable_thinking": False,
        "max_tokens": 192,
        "temperature": 0,
    }
    request = urllib.request.Request(
        base + "/chat/completions",
        data=json.dumps(payload, separators=(",", ":")).encode("utf-8"),
        headers={
            "Authorization": "Bearer " + api_key,
            "Content-Type": "application/json",
            "Accept": "text/event-stream",
            "User-Agent": "SuprAI-remote-smoke/1",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=80) as response:
            if response.status != 200:
                raise SmokeFailure(f"Chat: HTTP {response.status}")
            if "text/event-stream" not in response.headers.get("Content-Type", ""):
                raise SmokeFailure("Chat: el servidor no devolvió SSE")
            out: list[str] = []
            pending: list[str] = []
            completed = False
            count = 0
            for raw in response:
                if len(raw) > 1_000_000:
                    raise SmokeFailure("Chat: línea SSE demasiado grande")
                line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                if line.startswith("data:"):
                    pending.append(line[5:].lstrip())
                elif line == "" and pending:
                    data = "\n".join(pending)
                    pending.clear()
                    if data == "[DONE]":
                        completed = True
                        break
                    try:
                        event = json.loads(data)
                    except json.JSONDecodeError:
                        raise SmokeFailure("Chat: evento SSE inválido") from None
                    if "error" in event:
                        raise SmokeFailure("Chat: evento de error del proveedor")
                    for choice in event.get("choices", []):
                        chunk = choice.get("delta", {}).get("content", "")
                        if isinstance(chunk, str):
                            out.append(chunk)
                    count += 1
                    if count > 5000:
                        raise SmokeFailure("Chat: exceso de eventos SSE")
            if not completed:
                raise SmokeFailure("Chat: transmisión terminada sin [DONE]")
            answer = "".join(out).strip()
            if not answer:
                raise SmokeFailure("Chat: respuesta final vacía")
            if len(answer) > 50000:
                raise SmokeFailure("Chat: respuesta excesiva")
            return answer
    except urllib.error.HTTPError as exc:
        raise SmokeFailure(f"Chat: HTTP {exc.code}") from None
    except (urllib.error.URLError, OSError) as exc:
        raise SmokeFailure(f"Chat: problema de red ({type(exc).__name__})") from None


def summary(line: str) -> None:
    print(line, flush=True)
    target = os.environ.get("GITHUB_STEP_SUMMARY")
    if target:
        with open(target, "a", encoding="utf-8") as fp:
            fp.write(line + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--health-only", action="store_true")
    parser.add_argument("--inference-only", action="store_true")
    args = parser.parse_args()
    if args.health_only == args.inference_only:
        raise SmokeFailure("Especificá --health-only o --inference-only")

    base = api_base()
    if args.health_only:
        health = get_json(base[:-3] + "/health")
        if health.get("status") != "ok":
            raise SmokeFailure("Health respondió, pero no está ready")
        summary("PASS: NInfer HTTPS /health accesible desde GitHub Actions.")
        return 0

    api_key = os.environ.get("NINFER_API_KEY", "")
    if not api_key:
        summary("SKIP: Inferencia remota no comprobada (falta el secreto NINFER_API_KEY).")
        return 0

    catalog = get_json(base + "/models", api_key)
    models = catalog.get("data", [])
    ids = [m.get("id") for m in models if isinstance(m, dict) and isinstance(m.get("id"), str)]
    if not ids:
        raise SmokeFailure("Models: ningún ID de modelo disponible")
    model = ids[0]
    summary("PASS: /v1/models autenticado; ID descubierto: " + model)

    marker = "CI" + secrets.token_hex(5).upper()
    first_user = {"role": "user", "content": "Recordá el código " + marker + ". Respondé solamente: recibido."}
    first_answer = chat_stream(base, api_key, model, [
        {"role": "system", "content": "Respondé breve y sin razonamiento."},
        first_user,
    ])
    summary("PASS: primera respuesta SSE con contenido (" + str(len(first_answer)) + " caracteres).")
    second_answer = chat_stream(base, api_key, model, [
        {"role": "system", "content": "Respondé breve y sin razonamiento."},
        first_user,
        {"role": "assistant", "content": first_answer},
        {"role": "user", "content": "¿Qué código te pedí recordar? Respondé solo con el código."},
    ])
    if marker not in second_answer.upper():
        raise SmokeFailure("Contexto: la segunda respuesta no recuperó el código sintético")
    summary("PASS: segunda respuesta SSE conserva contexto reconstruido de dos turnos.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except SmokeFailure as exc:
        print("FAIL: " + str(exc), file=sys.stderr)
        sys.exit(1)
