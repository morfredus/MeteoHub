"""embed_web_files.py - minifie puis embarque les pages web dans le firmware, a CHAQUE compilation.

Ce que ca fait
--------------
Lit chaque fichier de data/ (sources versionnees), minifie les .html / .css / .js, et ecrit
include/web_pages.h : un tableau PROGMEM par fichier (`web_<nom>`), que web_manager.cpp sert.

  data/*.html, *.js, *.css  --(minification)-->  include/web_pages.h  (genere, ignore par Git)

data/ reste la SOURCE : on y edite des fichiers lisibles. web_pages.h n'est pas versionne : il
est reconstruit par le premier `pio run` d'un clone neuf.

Garanties
---------
- La minification (scripts/webmin.py) ne retire que ce qui est sur ; au moindre doute un fichier
  est embarque tel quel. Elle est testee : `python scripts/test_webmin.py`.
- web_pages.h n'est reecrit que si son contenu change : sinon chaque build changerait sa date et
  recompilerait web_manager.cpp.
- Une erreur interrompt le build, au lieu de compiler avec des pages absentes ou perimees.
- Pour deboguer le JavaScript dans le navigateur, embarquer les sources lisibles :
  definir NO_MINIFY=1 dans l'environnement avant `pio run`.

Dans un script PlatformIO, ecrire `Import("env")` seul (pas `env = Import("env")` : SCons ne renvoie
rien, `env` deviendrait None).
"""

import os
import sys

try:
    Import("env")            # noqa: F821  (fourni par PlatformIO)
except NameError:
    raise SystemExit("Erreur : ce script doit etre execute via PlatformIO (pio run).")

_PROJECT_DIR = env["PROJECT_DIR"]                                    # noqa: F821
_INCLUDE_DIR = env.get("PROJECT_INCLUDE_DIR")                        # noqa: F821
_DATA_DIR = os.path.join(_PROJECT_DIR, "data")
_HEADER_PATH = os.path.join(_INCLUDE_DIR, "web_pages.h")

sys.path.insert(0, os.path.join(_PROJECT_DIR, "scripts"))
import webmin  # noqa: E402

_TEXT_EXTENSIONS = (".html", ".htm", ".js", ".css")
_MINIFY = os.environ.get("NO_MINIFY", "") in ("", "0")


def _to_c_array(binary_data):
    return ", ".join(f"0x{byte:02x}" for byte in binary_data)


def _load(path, name):
    """Contenu a embarquer pour un fichier (minifie si c'est du texte web) et taille d'origine."""
    with open(path, "rb") as handle:
        raw = handle.read()
    if not _MINIFY or not name.lower().endswith(_TEXT_EXTENSIONS):
        return raw, len(raw)
    text = raw.decode("utf-8")
    mini = webmin.minify_by_extension(name, text)
    if not mini.strip():            # garde-fou : jamais embarquer un fichier vide par erreur
        return raw, len(raw)
    return mini.encode("utf-8"), len(raw)


def _build_header():
    lines = [
        "#pragma once",
        "",
        "/*",
        " * Fichier GENERE par scripts/embed_web_files.py a chaque compilation : ne pas le modifier,",
        " * ne pas le versionner. Sources : data/ (pages minifiees avant embarquement).",
        " */",
        "",
        "#include <pgmspace.h>",
        "",
    ]
    total_in = total_out = 0
    for item in sorted(os.listdir(_DATA_DIR)):
        file_path = os.path.join(_DATA_DIR, item)
        if not os.path.isfile(file_path):
            continue
        content, original = _load(file_path, item)
        total_in += original
        total_out += len(content)
        variable_name = "web_" + item.replace(".", "_").replace("-", "_")
        lines.append(f"// Contenu du fichier : {item}")
        lines.append(f"const char {variable_name}[] PROGMEM = {{")
        # Par blocs de 16 octets pour garder des lignes lisibles.
        for i in range(0, len(content), 16):
            lines.append("    " + _to_c_array(content[i:i + 16]) + ",")
        lines.append("    0x00")                 # terminateur nul
        lines.append("};")
        lines.append("")
    return "\n".join(lines), total_in, total_out


def _embed():
    if not os.path.isdir(_DATA_DIR):
        raise SystemExit(f"Dossier data/ introuvable : {_DATA_DIR}")
    try:
        header, total_in, total_out = _build_header()
    except (OSError, UnicodeDecodeError) as error:
        raise SystemExit(f"Generation de web_pages.h impossible : {error}")

    previous = None
    if os.path.exists(_HEADER_PATH):
        with open(_HEADER_PATH, encoding="utf-8", newline="") as handle:
            previous = handle.read()
    written = previous != header
    if written:
        with open(_HEADER_PATH, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(header)

    gain = 100 * (1 - total_out / total_in) if total_in else 0
    mode = "minifie" if _MINIFY else "NON minifie (NO_MINIFY)"
    print(f"Pages web : {mode}, {total_in} -> {total_out} octets ({gain:.1f} % de gain), "
          f"web_pages.h {'regenere' if written else 'inchange'}")


_embed()
