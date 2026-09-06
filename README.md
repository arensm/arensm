# arensm projects

Dieses Verzeichnis ist der gemeinsame Einstiegspunkt fuer die Projekte von
`arensm`. Projekte werden nach ihrem primaeren Build- und Laufzeittyp
einsortiert.

## Kategorien

| Verzeichnis | Inhalt |
| --- | --- |
| `platform-io/` | Mikrocontroller-Firmware, die mit PlatformIO gebaut wird |
| `bash/` | Eigenstaendige Bash-Skripte und Bash-Projekte |
| `docs/adr/` | Uebergreifende Architecture Decision Records (ADRs) |

Weitere Kategorien werden erst angelegt, wenn ein passendes Projekt migriert
wird. Der verbindliche Aufbau ist in
[`docs/adr/0001-einheitlicher-projektaufbau.md`](docs/adr/0001-einheitlicher-projektaufbau.md)
beschrieben.

## Grundregeln

- Ein Projekt liegt genau in einer fachlich passenden Kategorie.
- Bestehende Quellen werden bei einer Migration nicht veraendert oder geloescht.
- Der neue Stand muss eigenstaendig baubar und dokumentiert sein.
- Zugangsdaten und lokale Build-Artefakte gehoeren nicht ins Repository.
- Alle Ordnernamen werden kleingeschrieben.
- Jedes Projekt enthaelt eine zweisprachige `.env.example`; private Werte
  stehen ausschließlich in der ignorierten `.env`.
