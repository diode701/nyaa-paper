# Security Policy

Nyaa Paper downloads and runs wallpapers from the Steam Workshop as child processes of `linux-wallpaperengine`. This has a few real risk vectors — worth understanding before you trust it blindly.

## Supported versions

| Version | Supported |
|---------|-----------|
| 0.2.x   | ✅ Active development |
| < 0.2   | ❌ |

## Reporting a vulnerability

Wallpapers from the Workshop can contain arbitrary code through `project.json` properties, scene scripts, and shaders. While `linux-wallpaperengine` sandboxes some of this, **you should never assume safety.**

If you find a vulnerability in Nyaa Paper itself — not in LWE, not in a specific wallpaper — report it privately:

- **Email:** undeadsan7@gmail.com
- **Do not** open a public GitHub issue.

You'll get an acknowledgment within 48 hours. After triage, a fix is prioritised and a security advisory published.

## Scope

**In scope:**
- The Java application code (this repo)
- The install / uninstall scripts
- The build pipeline

**Out of scope:**
- Vulnerabilities in `linux-wallpaperengine` — report [upstream](https://github.com/Almamu/linux-wallpaperengine)
- Malicious workshop content — use at your own risk
