//! Gateway: polls the node over Modbus RTU (TCP bridge to the simulated UART) and publishes the
//! values as JSON, either to stdout or to an MQTT broker (optionally over TLS).

mod modbus;

use rumqttc::{AsyncClient, MqttOptions, QoS, TlsConfiguration, Transport};
use std::time::Duration;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;
use tokio::time::{sleep, timeout};

struct Args {
    modbus_addr: String,
    mqtt: Option<(String, u16)>,
    ca_file: Option<String>,
    topic: String,
    interval_ms: u64,
    count: Option<u32>,
}

fn parse_args() -> Args {
    let mut a = Args {
        modbus_addr: "127.0.0.1:3456".into(),
        mqtt: None,
        ca_file: None,
        topic: "nodes/node1/telemetry".into(),
        interval_ms: 1000,
        count: None,
    };
    let argv: Vec<String> = std::env::args().skip(1).collect();
    let mut i = 0;
    while i < argv.len() {
        let val = argv.get(i + 1).cloned().unwrap_or_default();
        match argv[i].as_str() {
            "--modbus" => a.modbus_addr = val,
            "--mqtt" => {
                let (h, p) = val.rsplit_once(':').expect("--mqtt host:port");
                a.mqtt = Some((h.to_string(), p.parse().expect("port")));
            }
            "--ca" => a.ca_file = Some(val),
            "--topic" => a.topic = val,
            "--interval-ms" => a.interval_ms = val.parse().expect("interval"),
            "--count" => a.count = Some(val.parse().expect("count")),
            other => {
                eprintln!("unknown option {other}\nusage: gateway [--modbus host:port] [--mqtt host:port --ca ca.crt] [--topic t] [--interval-ms n] [--count n]");
                std::process::exit(2);
            }
        }
        i += 2;
    }
    a
}

async fn poll(stream: &mut TcpStream) -> Result<Vec<u16>, String> {
    stream.write_all(&modbus::read_holding_request(1, 0, 4)).await.map_err(|e| e.to_string())?;
    let mut buf = [0u8; 64];
    let mut got = 0usize;
    // reply for 4 registers is 3 + 8 + 2 = 13 bytes
    while got < 13 {
        let n = timeout(Duration::from_secs(2), stream.read(&mut buf[got..]))
            .await
            .map_err(|_| "timeout waiting for the node".to_string())?
            .map_err(|e| e.to_string())?;
        if n == 0 {
            return Err("connection closed".into());
        }
        got += n;
        if got >= 5 && buf[1] & 0x80 != 0 {
            break;
        }
    }
    modbus::parse_read_holding_reply(1, &buf[..got]).map_err(|e| format!("{e:?}"))
}

#[tokio::main]
async fn main() {
    let args = parse_args();
    let mut stream = TcpStream::connect(&args.modbus_addr).await.expect("connect to the node");

    let client = args.mqtt.as_ref().map(|(host, port)| {
        let mut opts = MqttOptions::new("stm32-gateway", host, *port);
        opts.set_keep_alive(Duration::from_secs(10));
        if let Some(ca) = &args.ca_file {
            let ca = std::fs::read(ca).expect("read CA file");
            opts.set_transport(Transport::tls_with_config(TlsConfiguration::Simple {
                ca,
                alpn: None,
                client_auth: None,
            }));
        }
        let (client, mut eventloop) = AsyncClient::new(opts, 10);
        tokio::spawn(async move {
            loop {
                if let Err(e) = eventloop.poll().await {
                    eprintln!("mqtt: {e}");
                    sleep(Duration::from_secs(1)).await;
                }
            }
        });
        client
    });

    let mut sent = 0u32;
    loop {
        match poll(&mut stream).await {
            Ok(regs) if regs.len() == 4 => {
                let json = format!(
                    "{{\"temperature_c\":{:.1},\"samples\":{},\"uptime_s\":{},\"i2c_errors\":{}}}",
                    regs[0] as i16 as f32 / 10.0, regs[1], regs[2], regs[3]
                );
                println!("{json}");
                if let Some(c) = &client {
                    if let Err(e) = c.publish(&args.topic, QoS::AtLeastOnce, false, json.clone()).await {
                        eprintln!("publish failed: {e}");
                    }
                }
            }
            Ok(r) => eprintln!("unexpected register count {}", r.len()),
            Err(e) => eprintln!("poll error: {e}"),
        }
        sent += 1;
        if args.count.is_some_and(|c| sent >= c) {
            break;
        }
        sleep(Duration::from_millis(args.interval_ms)).await;
    }
    // give the MQTT event loop a moment to flush the last publish
    if client.is_some() {
        sleep(Duration::from_millis(500)).await;
    }
}
