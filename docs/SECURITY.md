# Security

Scope: a single-appliance, offline device. This is a design/hardening summary,
not a completed security audit.

## Attack surface

- **No network.** Execution is 100 % local; no cloud service, no listening TCP
  socket. The only IPC is the local `AF_UNIX` bus in `/run/nui`.
- **Bus.** `SOCK_SEQPACKET` sockets under `/run/nui` (mode 0770, owner `nui`).
  Every received frame is validated (`valid_header`: magic, protocol version,
  payload-length bound) before use. Because SEQPACKET preserves message
  boundaries, a malformed datagram does not desync the stream: the engine
  **drops it and continues** (fail-open on the trusted local bus) rather than
  tearing down the session. This is verified by an automated malformed-frame
  injection test (`tests/integration/run_malformed.sh`, ctest `malformed`):
  5 malformed variants (bad magic, bad version, oversized/mismatched length,
  truncated) are all dropped, and a valid gesture sent afterwards on the same
  connection is still processed. A valid header with an unknown message type is
  safely ignored.
- **Model integrity.** `.hef` files are verified against recorded SHA-256
  (FIPS 180-4, self-contained, cross-checked against coreutils `sha256sum`) before
  loading, mitigating tampered/corrupted model files.

## systemd hardening

Applied per service (validated with `systemd-analyze verify`: no invalid
directives). Hardening is **not uniform** because device needs differ:

- `nui-engine`: strongest sandbox — `PrivateDevices=yes`, `ProtectSystem=strict`,
  `MemoryDenyWriteExecute=yes`, `SystemCallFilter=@system-service`,
  `RestrictAddressFamilies=AF_UNIX`. It touches no devices.
- `nui-perception`: needs the Hailo accelerator and camera, so `PrivateDevices=no`.
  Tighten with `DeviceAllow` once the exact node names are confirmed on target
  (e.g. `/dev/hailo0`, `/dev/video*`, `/dev/media*`, `/dev/dma_heap/*`) —
  **"Je ne sais pas"** the exact nodes without the target. No `MemoryDenyWriteExecute`.
- `nui-ui`: needs the GPU (`/dev/dri`) and the Wayland socket, so `PrivateDevices=no`
  and no `MemoryDenyWriteExecute` (would break the QML JIT / GPU driver mappings).

All services run as the unprivileged system user `nui` with `NoNewPrivileges=yes`,
`ProtectHome=yes`, `PrivateTmp=yes`, and `ReadWritePaths=/run/nui`.

## What is NOT validated

- No broad fuzzing of the bus. A targeted malformed-frame injection test exists
  (see above), but systematic fuzzing / runtime penetration testing does not.
- Device-level ACLs for perception/ui are placeholders pending target node names.
- Physical security of the device and projector is out of scope.
- No secure-boot / disk-encryption posture is specified here.

These are open items; the project is not claimed to be security-complete
(see `docs/STATUS.md`).
