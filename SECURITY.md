# Security Policy

## Supported versions

WiFeeder v2 is under active bring-up. Security fixes target the `main` branch (and are back-ported to `develop` as needed).

## Reporting a vulnerability

Please **do not** open a public issue for security-sensitive findings (credential leaks, remote command paths, MQTT auth bypasses, etc.).

1. Use GitHub **Private vulnerability reporting** on this repository if enabled:  
   [Security advisories](https://github.com/natsumed/WiFeeder/security/advisories/new)
2. Or contact the repository owner (@natsumed) directly with:
   - a clear description of the issue
   - impact and affected components (STM32 / Pi host / protocol / cloud MQTT)
   - steps to reproduce (non-destructive preferred)

You should receive an acknowledgement within a few days. Please give us a reasonable window to fix before any public disclosure.

## Safe handling of secrets

Never commit MQTT production credentials, API tokens, private keys, or device certificates. Use local env files or out-of-band provisioning; keep examples pointing at `localhost` only.
