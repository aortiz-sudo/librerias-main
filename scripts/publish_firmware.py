"""Publica los binarios de OTA en firmware_repository y verifica lo publicado.

El ESP32 descarga cada binario con la API de GitHub:

    /repos/aortiz-sudo/firmware_repository/contents/<ruta>?ref=<prefijo><versión>

y toma las versiones vigentes de versions.txt (rama main, formato esp32;display;impresora).
Este script mantiene las tres cosas consistentes: binarios, versions.txt y un tag
anotado <componente>-<versión> por cada componente publicado.

Uso (desde la raíz del proyecto):

    python scripts/publish_firmware.py publish --esp32 1.1.0 --display 1.1.0 --printer 1.1.0 [--push]
    python scripts/publish_firmware.py verify          # requiere la variable GITHUB_TOKEN

Opciones de publish:
    --push          Sube main y los tags nuevos (push atómico). Sin esta opción todo queda local.
    --allow-dirty   Permite publicar con cambios sin commitear en el proyecto (no recomendado:
                    el binario no corresponde a ningún commit).
    --no-build      Usa los binarios ya compilados en .pio/build (por defecto hace build limpio).
    --repo RUTA     Clone de firmware_repository (por defecto ./firmware_repository).
"""

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REPO = PROJECT_ROOT / "firmware_repository"

GITHUB_REPO = "aortiz-sudo/firmware_repository"
API_BASE = "https://api.github.com/repos/" + GITHUB_REPO + "/contents"

ESP32_PARTITION_SIZE = 0x1F0000   # app0/app1 en partitions.csv
AVR_FLASH_SIZE = 32 * 1024        # ATmega328PB

# Debe coincidir con firmwares_endpoints y firmware_tag_prefixes en src/main.cpp
# (check_firmware_contract() lo revisa antes de publicar).
COMPONENTS = {
    "esp32": {"index": 0, "prefix": "esp32-", "file": "firmware.bin", "kind": "esp32",
              "envs": ["esp32_release", "esp32_release_legacy"]},
    "display": {"index": 1, "prefix": "display-", "file": "display.bin", "kind": "avr",
                "envs": ["display_release"]},
    "printer": {"index": 2, "prefix": "printer-", "file": "printer.bin", "kind": "avr",
                "envs": ["printer_release"]},
}

VERSION_RE = re.compile(r"^\d+\.\d+\.\d+$")
MAX_VERSION_LEN = 9               # current_*_fw_v[10] en main.cpp


class PublishError(Exception):
    pass


def run(cmd, cwd=None, check=True, capture=True):
    result = subprocess.run(cmd, cwd=cwd, capture_output=capture, text=True)
    if check and result.returncode != 0:
        detail = (result.stderr or result.stdout or "").strip() if capture else ""
        raise PublishError("Falló: " + " ".join(cmd) + ("\n" + detail if detail else ""))
    return result.stdout.strip() if capture else ""


def git(repo, *args, check=True):
    return run(["git", "-C", str(repo)] + list(args), check=check)


def find_pio():
    pio = shutil.which("pio")
    if pio:
        return pio
    home = Path.home() / ".platformio" / "penv"
    for candidate in (home / "Scripts" / "pio.exe", home / "bin" / "pio"):
        if candidate.exists():
            return str(candidate)
    raise PublishError("No se encontró la CLI de PlatformIO (pio).")


def rel_path(env, name):
    return ".pio/build/{}/{}".format(env, name)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def check_firmware_contract():
    """Comprueba que src/main.cpp pida las mismas rutas y prefijos que publica este script."""
    source = (PROJECT_ROOT / "src" / "main.cpp").read_text(encoding="utf-8", errors="replace")
    expected = ['"/repos/{}/contents"'.format(GITHUB_REPO),
                '"esp32_release"', '"esp32_release_legacy"', '"/firmware.bin"',
                '"/.pio/build/display_release/display.bin"',
                '"/.pio/build/printer_release/printer.bin"',
                '"esp32-"', '"display-"', '"printer-"']
    missing = [s for s in expected if s not in source]
    if missing:
        raise PublishError("src/main.cpp ya no coincide con este script; revisa: " + ", ".join(missing))


def read_versions(text):
    fields = [f.strip() for f in text.strip().split(";")]
    if len(fields) != 3:
        raise PublishError("versions.txt no tiene el formato esp32;display;impresora: {!r}".format(text))
    return fields


def validate_binary(path, kind):
    data = path.read_bytes()
    if not data:
        raise PublishError("{} está vacío.".format(path))
    if kind == "esp32":
        if data[0] != 0xE9:
            raise PublishError("{} no es una imagen de app del ESP32 (byte mágico 0xE9).".format(path))
        if len(data) > ESP32_PARTITION_SIZE:
            raise PublishError("{} ({} bytes) no cabe en la partición OTA.".format(path, len(data)))
    elif len(data) > AVR_FLASH_SIZE:
        raise PublishError("{} ({} bytes) no cabe en la flash del ATmega.".format(path, len(data)))
    return data


def publish(args):
    requested = {name: getattr(args, name) for name in COMPONENTS if getattr(args, name)}
    if not requested:
        raise PublishError("Indica al menos un componente: --esp32, --display o --printer.")

    for name, version in requested.items():
        if not VERSION_RE.match(version) or len(version) > MAX_VERSION_LEN:
            raise PublishError("Versión inválida para {}: {!r} (formato X.Y.Z, máximo {} caracteres)."
                               .format(name, version, MAX_VERSION_LEN))

    check_firmware_contract()

    # Estado del código fuente.
    dirty = git(PROJECT_ROOT, "status", "--porcelain")
    if dirty and not args.allow_dirty:
        raise PublishError("El proyecto tiene cambios sin commitear. Haz commit primero, "
                           "o usa --allow-dirty si de verdad quieres publicar así.")
    source_rev = git(PROJECT_ROOT, "describe", "--always", "--dirty", "--abbrev=12")

    # Estado del clone de firmware_repository.
    repo = Path(args.repo).resolve()
    if not (repo / ".git").exists():
        raise PublishError("{} no es un clone de {}.".format(repo, GITHUB_REPO))
    if git(repo, "rev-parse", "--abbrev-ref", "HEAD") != "main":
        raise PublishError("El clone no está en la rama main.")
    if git(repo, "status", "--porcelain"):
        raise PublishError("El clone de firmware_repository tiene cambios sin commitear.")
    git(repo, "fetch", "origin", "--tags")
    if git(repo, "rev-parse", "HEAD") != git(repo, "rev-parse", "origin/main"):
        raise PublishError("El clone no está al día con origin/main (haz git pull --ff-only).")

    tags = ["{}{}".format(COMPONENTS[name]["prefix"], version) for name, version in requested.items()]
    for tag in tags:
        if git(repo, "rev-parse", "-q", "--verify", "refs/tags/" + tag, check=False):
            raise PublishError("El tag {} ya existe en el clone.".format(tag))
        if git(repo, "ls-remote", "--tags", "origin", "refs/tags/" + tag):
            raise PublishError("El tag {} ya existe en GitHub.".format(tag))

    versions_path = repo / "versions.txt"
    versions = read_versions(versions_path.read_text(encoding="utf-8"))

    envs = [env for name in requested for env in COMPONENTS[name]["envs"]]

    if not args.no_build:
        pio = find_pio()
        env_args = [a for env in envs for a in ("-e", env)]
        print("Build limpio de: " + ", ".join(envs))
        run([pio, "run", "-t", "clean"] + env_args, cwd=str(PROJECT_ROOT), capture=False)
        run([pio, "run"] + env_args, cwd=str(PROJECT_ROOT), capture=False)

    published = []
    for name in requested:
        comp = COMPONENTS[name]
        for env in comp["envs"]:
            src = PROJECT_ROOT / ".pio" / "build" / env / comp["file"]
            if not src.exists():
                raise PublishError("No existe {}; ¿se compiló el entorno {}?".format(src, env))
            data = validate_binary(src, comp["kind"])
            dst = repo / rel_path(env, comp["file"])
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(data)
            published.append((rel_path(env, comp["file"]), len(data), sha256(data)))
        versions[comp["index"]] = requested[name]

    # Sin salto de línea final: el firmware compara las versiones tal cual.
    versions_path.write_bytes(";".join(versions).encode("ascii"))

    git(repo, "add", "--", "versions.txt", *[p for p, _, _ in published])

    summary = ", ".join("{} {}".format(name, version) for name, version in requested.items())
    body = ["release: " + summary, "",
            "Fuente: SMARTFUEL-GAS " + source_rev,
            "versions.txt: " + ";".join(versions), "", "SHA-256:"]
    body += ["  {}  {} bytes  {}".format(p, size, digest) for p, size, digest in published]
    if args.co_author:
        body += ["", "Co-Authored-By: " + args.co_author]
    git(repo, "commit", "-m", "\n".join(body))

    for name, version in requested.items():
        tag = "{}{}".format(COMPONENTS[name]["prefix"], version)
        git(repo, "tag", "-a", tag, "-m", "{} {} (fuente SMARTFUEL-GAS {})".format(name, version, source_rev))

    commit = git(repo, "rev-parse", "--short", "HEAD")
    print("\nCommit {} en {}".format(commit, repo))
    for p, size, digest in published:
        print("  {:48s} {:>8d} bytes  {}".format(p, size, digest))
    print("versions.txt: " + ";".join(versions))
    print("Tags: " + ", ".join(tags))

    if args.push:
        git(repo, "push", "--atomic", "origin", "main", *["refs/tags/" + t for t in tags])
        print("\nPublicado en GitHub. Verifica con:  python scripts/publish_firmware.py verify")
    else:
        print("\nTodo quedó local. Para subirlo:  git -C \"{}\" push --atomic origin main {}"
              .format(repo, " ".join("refs/tags/" + t for t in tags)))


def api_get(url, token):
    request = urllib.request.Request(url, headers={
        "Accept": "application/vnd.github.raw",
        "Authorization": "Bearer " + token,
        "User-Agent": "desarrolloidt",
        "X-GitHub-Api-Version": "2022-11-28",
    })
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.status, response.headers.get("Content-Length"), response.read()


def verify(args):
    token = os.environ.get("GITHUB_TOKEN", "").strip()
    if not token:
        raise PublishError("Define la variable de entorno GITHUB_TOKEN con el token de los equipos.")
    repo = Path(args.repo).resolve()
    ok = True

    # Igual que check_firmware_updates(): versions.txt de la rama por defecto.
    try:
        status, length, body = api_get(API_BASE + "/versions.txt", token)
    except urllib.error.HTTPError as e:
        raise PublishError("versions.txt -> HTTP {} (¿token sin acceso a {}?)".format(e.code, GITHUB_REPO))
    except urllib.error.URLError as e:
        raise PublishError("No se pudo conectar con api.github.com: {}".format(e.reason))
    raw = body.decode("ascii", errors="replace")
    print("versions.txt -> HTTP {}, {!r}".format(status, raw))
    if raw != raw.strip():
        print("  AVISO: versions.txt tiene espacios o saltos de línea al final.")
    versions = read_versions(raw)

    git(repo, "fetch", "origin", "--tags")
    for name, comp in COMPONENTS.items():
        version = versions[comp["index"]]
        tag = comp["prefix"] + version
        for env in comp["envs"]:
            path = rel_path(env, comp["file"])
            url = "{}/{}?ref={}".format(API_BASE, path, tag)
            try:
                status, length, body = api_get(url, token)
            except urllib.error.HTTPError as e:
                print("FALLA {:8s} {} -> HTTP {}".format(name, url, e.code))
                ok = False
                continue
            expected = subprocess.run(["git", "-C", str(repo), "show", "{}:{}".format(tag, path)],
                                      capture_output=True)
            same_len = length is not None and int(length) == len(body)
            same_hash = expected.returncode == 0 and sha256(expected.stdout) == sha256(body)
            good = status == 200 and same_len and same_hash
            ok &= good
            print("{} {:8s} {} -> HTTP {}, Content-Length {}, SHA-256 {}".format(
                "OK   " if good else "FALLA", name, url.replace(API_BASE, ""), status, length,
                "igual al tag" if same_hash else "DIFERENTE o sin tag local"))

    if not ok:
        raise PublishError("La verificación encontró problemas.")
    print("\nTodo coincide con lo que descargará el firmware.")


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    parser = argparse.ArgumentParser(description="Publica y verifica los binarios de OTA.")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("publish", help="Compila, copia, actualiza versions.txt, hace commit y tags.")
    for name in COMPONENTS:
        p.add_argument("--" + name, metavar="X.Y.Z", help="Versión de " + name)
    p.add_argument("--push", action="store_true", help="Sube main y los tags nuevos.")
    p.add_argument("--allow-dirty", action="store_true", help="Permite cambios sin commitear en el proyecto.")
    p.add_argument("--no-build", action="store_true", help="No compila; usa .pio/build tal como está.")
    p.add_argument("--repo", default=str(DEFAULT_REPO), help="Clone de firmware_repository.")
    p.add_argument("--co-author", metavar="'Nombre <correo>'", help="Agrega un trailer Co-Authored-By al commit.")
    p.set_defaults(func=publish)

    v = sub.add_parser("verify", help="Descarga lo publicado como lo hace el firmware y lo compara.")
    v.add_argument("--repo", default=str(DEFAULT_REPO), help="Clone de firmware_repository.")
    v.set_defaults(func=verify)

    args = parser.parse_args()
    try:
        args.func(args)
    except PublishError as e:
        print("ERROR: {}".format(e), file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
