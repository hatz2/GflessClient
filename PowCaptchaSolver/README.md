# PowCaptchaSolver

Runs Gameforge's `gf-pow-captcha` challenge for the launcher.

Since 2026 a login can be answered with a challenge of type `gf-pow-captcha` instead of the image-drop
puzzle. There is nothing for the user to answer: Gameforge's own script (the `script` URL of the
challenge) runs a proof of work and a set of browser checks by itself and reports the result. In the
official client the user only sees "Solving..." for a few seconds. It does need a real browser, though.

This helper opens a small window of the Edge or Chrome that is installed (throwaway profile, no
extensions) on a local page that embeds that script exactly as the client does, waits for the script
to say it is done and reports the verdict to the launcher. When the Gameforge account uses a SOCKS5
proxy, the browser's traffic goes through it as well: Chromium cannot log in to a proxy with a user
and password, so the helper runs a relay on `127.0.0.1` that takes the browser without credentials
and forwards to the real proxy. Names are resolved by the proxy, never by this machine.

The launcher (`PowCaptchaSolver` class in `Launcher/src/auth`) runs `PowCaptchaSolver.exe` from its
own folder when a login gets this type of challenge, shows a progress dialog meanwhile and retries
the login once the check passed.

The launcher runs as administrator, and Edge and Chrome started that way relaunch themselves without
those rights: the process the helper starts exits at once and the window belongs to another one. So
the helper never looks at that process: the page pings it while the window lives (closed by hand =
the pings stop), closes itself after reporting, and whatever is left is found by the throwaway
profile folder and stopped.

## Prerequisites

- [Python](https://python.org/) 3.10+ (standard library only, no packages)
- A Chromium browser: Edge (shipped with Windows) or Chrome. `GFLESS_BROWSER` can point to another one.

## Build

```powershell
.\build_exe.ps1          # runs the tests, then packages dist\PowCaptchaSolver.exe
```

Copy `dist\PowCaptchaSolver.exe` next to `GflessClient.exe`. The installer script picks it up from
`PowCaptchaSolver\dist`.

## Protocol

The launcher writes one JSON object to the helper's stdin and closes it:

```json
{"challengeId": "...", "script": "https://pow-captcha.gameforge.com/...", "locale": "en-GB",
 "proxy": {"host": "1.2.3.4", "port": 1080, "user": "...", "password": "..."}, "timeout": 200}
```

`proxy` is omitted (or `null`) for a direct login. The helper prints progress on stderr and one JSON
line on stdout with the verdict, `{"status": "solved" | "failed" | "error", "detail": "..."}`, and exits
with 0 (solved), 1 (the check ran but did not pass) or 2 (it could not run: no browser, bad input,
timeout). Credentials are never printed.

Manual run, for development:

```powershell
'{"challengeId": "<id>", "script": "<script url>"}' | python src\pow_captcha_solver.py
```

## Tests

```powershell
python -m unittest discover -s tests
```

They cover the SOCKS5 relay (through a fake proxy with credentials), the local page and the request
parsing; none of them opens a browser.
