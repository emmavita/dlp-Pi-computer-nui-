//! nui-supervisor — optional process supervisor for the NUI services.
//!
//! Responsibility (single): keep `nui-engine`, `nui-perception` and `nui-ui`
//! running, restarting each on exit with exponential backoff. It spawns the
//! real service binaries and monitors their exit status; it does NOT use any
//! heartbeat/UDS channel, so it requires no change to the (frozen) service
//! interfaces or the `proto` contracts.
//!
//! Scope note: this duplicates what `systemd` `Restart=` already provides. It
//! exists as a std-only, dependency-free, FFI-free artifact and is collapsible:
//! removing it costs no functionality when the services run under systemd.
//! Shutdown / whole-group teardown is delegated to systemd (KillMode=control-group);
//! this binary deliberately does not install signal handlers (which std cannot
//! do portably without an external crate or FFI).

use std::process::Command;
use std::thread;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

struct ServiceSpec {
    name: &'static str,
    program: String,
    args: Vec<String>,
    env: Vec<(String, String)>,
}

fn now_ms() -> u128 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_millis())
        .unwrap_or(0)
}

fn log(name: &str, msg: &str) {
    // stderr is captured by journald when run under systemd.
    eprintln!("[{}] supervisor/{}: {}", now_ms(), name, msg);
}

/// Supervise a single service forever: start, wait, back off, restart.
fn supervise(spec: ServiceSpec) {
    let initial = Duration::from_millis(500);
    let max_backoff = Duration::from_secs(30);
    let reset_after = Duration::from_secs(10);
    let mut backoff = initial;

    loop {
        let started = Instant::now();
        log(spec.name, &format!("starting {}", spec.program));

        let mut cmd = Command::new(&spec.program);
        cmd.args(&spec.args);
        for (k, v) in &spec.env {
            cmd.env(k, v);
        }
        // stdout/stderr are inherited so service logs surface in journald.

        match cmd.spawn() {
            Ok(mut child) => match child.wait() {
                Ok(status) => log(spec.name, &format!("exited with {}", status)),
                Err(e) => log(spec.name, &format!("wait error: {}", e)),
            },
            Err(e) => log(spec.name, &format!("spawn failed: {} ({})", spec.program, e)),
        }

        // Reset backoff if the service ran long enough to be considered stable.
        if started.elapsed() >= reset_after {
            backoff = initial;
        }

        log(spec.name, &format!("restarting in {:?}", backoff));
        thread::sleep(backoff);

        let doubled = backoff.saturating_mul(2);
        backoff = if doubled > max_backoff { max_backoff } else { doubled };
    }
}

fn env_or(key: &str, default: &str) -> String {
    std::env::var(key).unwrap_or_else(|_| default.to_string())
}

fn main() {
    let bin_dir = env_or("NUI_BIN_DIR", ".");
    let perc_sock = env_or("NUI_PERCEPTION_SOCK", "/tmp/nui_perception.sock");
    let ui_sock = env_or("NUI_UI_SOCK", "/tmp/nui_ui.sock");
    let calib = std::env::var("NUI_CALIB").ok();

    let mut engine_env = vec![
        ("NUI_PERCEPTION_SOCK".to_string(), perc_sock.clone()),
        ("NUI_UI_SOCK".to_string(), ui_sock.clone()),
    ];
    if let Some(c) = &calib {
        engine_env.push(("NUI_CALIB".to_string(), c.clone()));
    }

    let specs = vec![
        ServiceSpec {
            name: "engine",
            program: format!("{}/nui-engine", bin_dir),
            args: vec![],
            env: engine_env,
        },
        ServiceSpec {
            name: "perception",
            program: format!("{}/nui-perception", bin_dir),
            args: vec![],
            env: vec![("NUI_PERCEPTION_SOCK".to_string(), perc_sock.clone())],
        },
        ServiceSpec {
            name: "ui",
            program: format!("{}/nui-ui", bin_dir),
            args: vec![],
            env: vec![("NUI_UI_SOCK".to_string(), ui_sock.clone())],
        },
    ];

    log(
        "main",
        "starting NUI supervisor (process supervision; shutdown lifecycle is systemd's job)",
    );

    // engine is the UDS server; perception/ui are clients that reconnect on
    // their own, so a short stagger is sufficient (not a hard dependency).
    let mut handles = Vec::new();
    let mut first = true;
    for spec in specs {
        if !first {
            thread::sleep(Duration::from_millis(300));
        }
        first = false;
        handles.push(thread::spawn(move || supervise(spec)));
    }
    for h in handles {
        let _ = h.join();
    }
}
