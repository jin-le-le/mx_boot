# Technical Documentation

> ⚠️ The detailed docs in this folder are written in **Chinese**.
> File paths, code blocks and tables are language-neutral, so you can still navigate via the index below.
> For an English overview, see [README.en.md](../README.en.md) in the project root.

## Index

| Doc | Content |
|-----|---------|
| [PORTING_GUIDE.md](PORTING_GUIDE.md) | 10-step porting guide for adding a new chip (test checklist + pitfalls + production scenarios) |
| [PROTOCOL_SPEC.md](PROTOCOL_SPEC.md) | UART protocol spec (frame format / opcodes / `device_info_t` / state machine) |
| [IMAGE_FORMAT.md](IMAGE_FORMAT.md) | 256-byte image header layout (field offsets, magic, signature, CRC) |
| [ECDSA_DESIGN.md](ECDSA_DESIGN.md) | Design rationale for choosing ECDSA P-256 over HMAC / RSA |
| [architecture_diagrams.md](architecture_diagrams.md) | Mermaid source for 4 architecture diagrams (system / signing flow / upgrade sequence / size comparison) |

## Reading order for first-time contributors

1. **[README.md](../README.md)** — project overview + supported chips + Flash layout
2. **PORTING_GUIDE.md** — if you want to port to a new chip
3. **PROTOCOL_SPEC.md** — if you want to write your own host tool
4. **IMAGE_FORMAT.md** — if you want to understand or modify the firmware header
5. **ECDSA_DESIGN.md** — if you want to understand the security design tradeoffs
6. **architecture_diagrams.md** — visual reference
