use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};
use tracing::warn;

use crate::protocol::{decode_request, encode_response, ClientRequest, ServerMessage};

use super::AppState;

const MAX_FRAME: usize = 16 * 1024 * 1024 + 5;

pub async fn run_listener(listener: TcpListener, state: AppState) -> anyhow::Result<()> {
    loop {
        let (stream, addr) = listener.accept().await?;
        tracing::info!("client connected from {addr}");
        let state = state.clone();
        tokio::spawn(async move {
            if let Err(e) = handle_connection(stream, state).await {
                warn!("connection from {addr} ended: {e:#}");
            }
        });
    }
}

async fn handle_connection(mut stream: TcpStream, state: AppState) -> anyhow::Result<()> {
    loop {
        let frame = match read_frame(&mut stream).await {
            Ok(f) => f,
            Err(e) if e.kind() == std::io::ErrorKind::UnexpectedEof => break,
            Err(e) => return Err(e.into()),
        };

        let req = match decode_request(&frame) {
            Ok(r) => r,
            Err(e) => {
                write_message(
                    &mut stream,
                    &ServerMessage::Error(format!("invalid request: {e:#}")),
                )
                .await?;
                continue;
            }
        };

        let resp = match req {
            ClientRequest::GetCandles {
                symbol,
                before_index,
                limit,
            } => match state
                .kline
                .read_candles_raw(&symbol, before_index, limit)
                .await
            {
                Ok((start_index, total, records, indicators)) => ServerMessage::CandleChunk {
                    start_index,
                    total,
                    records,
                    indicators,
                },
                Err(e) => ServerMessage::Error(format!("read candles failed: {e:#}")),
            },
        };

        write_message(&mut stream, &resp).await?;
    }

    Ok(())
}

async fn read_frame(stream: &mut TcpStream) -> std::io::Result<Vec<u8>> {
    let mut header = [0u8; 5];
    stream.read_exact(&mut header).await?;
    let payload_len = u32::from_le_bytes(header[1..5].try_into().unwrap()) as usize;
    if payload_len > MAX_FRAME {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "frame too large",
        ));
    }
    let mut frame = vec![0u8; 5 + payload_len];
    frame[..5].copy_from_slice(&header);
    if payload_len > 0 {
        stream.read_exact(&mut frame[5..]).await?;
    }
    Ok(frame)
}

async fn write_message(stream: &mut TcpStream, msg: &ServerMessage) -> anyhow::Result<()> {
    let frame = encode_response(msg);
    stream.write_all(&frame).await?;
    Ok(())
}
