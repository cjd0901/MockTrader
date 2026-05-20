//! Binary TCP protocol. K线 payload 与 `get_5min.py` 一致：每条 32 字节、8×i32 小端。

use anyhow::{bail, Context};

pub const KLINE_RECORD_SIZE: usize = 32;

pub const MSG_REQ_LIST_STOCKS: u8 = 1;
pub const MSG_REQ_GET_CANDLES: u8 = 2;

pub const MSG_RSP_STOCK_LIST: u8 = 101;
pub const MSG_RSP_CANDLE_CHUNK: u8 = 102;
pub const MSG_RSP_ERROR: u8 = 255;

const MAX_SYMBOL_LEN: usize = 32;
const MAX_NAME_LEN: usize = 128;
const MAX_ERROR_LEN: usize = 4096;
const MAX_FRAME_PAYLOAD: usize = 16 * 1024 * 1024;

#[derive(Debug, Clone)]
pub struct StockEntry {
    pub symbol: String,
    pub display_name: String,
}

#[derive(Debug)]
pub enum ClientRequest {
    ListStocks,
    GetCandles {
        symbol: String,
        before_index: Option<u64>,
        limit: u32,
    },
}

#[derive(Debug)]
pub enum ServerMessage {
    StockList(Vec<StockEntry>),
    CandleChunk {
        start_index: u64,
        total: u64,
        /// 原始 K 线字节，`len() % 32 == 0`
        records: Vec<u8>,
    },
    Error(String),
}

pub fn decode_request(frame: &[u8]) -> anyhow::Result<ClientRequest> {
    if frame.len() < 5 {
        bail!("frame too short");
    }
    let msg_type = frame[0];
    let payload_len = u32::from_le_bytes(frame[1..5].try_into()?) as usize;
    if frame.len() != 5 + payload_len {
        bail!("frame length mismatch");
    }
    if payload_len > MAX_FRAME_PAYLOAD {
        bail!("payload too large");
    }
    let payload = &frame[5..];

    match msg_type {
        MSG_REQ_LIST_STOCKS => {
            if !payload.is_empty() {
                bail!("listStocks payload must be empty");
            }
            Ok(ClientRequest::ListStocks)
        }
        MSG_REQ_GET_CANDLES => decode_get_candles(payload),
        other => bail!("unknown request type: {other}"),
    }
}

fn decode_get_candles(payload: &[u8]) -> anyhow::Result<ClientRequest> {
    if payload.len() < 14 {
        bail!("getCandles payload too short");
    }
    let sym_len = payload[0] as usize;
    if sym_len == 0 || sym_len > MAX_SYMBOL_LEN || payload.len() < 1 + sym_len + 13 {
        bail!("invalid symbol length");
    }
    let symbol = std::str::from_utf8(&payload[1..1 + sym_len])
        .context("symbol utf-8")?
        .to_string();

    let off = 1 + sym_len;
    let before_flag = payload[off];
    let before_index = u64::from_le_bytes(payload[off + 1..off + 9].try_into()?);
    let limit = u32::from_le_bytes(payload[off + 9..off + 13].try_into()?);

    let before_index = if before_flag != 0 {
        Some(before_index)
    } else {
        None
    };

    if limit == 0 {
        bail!("limit must be > 0");
    }

    Ok(ClientRequest::GetCandles {
        symbol,
        before_index,
        limit,
    })
}

#[cfg(test)]
pub fn encode_request(req: &ClientRequest) -> Vec<u8> {
    match req {
        ClientRequest::ListStocks => encode_frame(MSG_REQ_LIST_STOCKS, &[]),
        ClientRequest::GetCandles {
            symbol,
            before_index,
            limit,
        } => {
            let sym_bytes = symbol.as_bytes();
            let mut payload = Vec::with_capacity(1 + sym_bytes.len() + 13);
            payload.push(sym_bytes.len().min(MAX_SYMBOL_LEN) as u8);
            payload.extend_from_slice(&sym_bytes[..sym_bytes.len().min(MAX_SYMBOL_LEN)]);
            if let Some(idx) = before_index {
                payload.push(1);
                payload.extend_from_slice(&idx.to_le_bytes());
            } else {
                payload.push(0);
                payload.extend_from_slice(&0u64.to_le_bytes());
            }
            payload.extend_from_slice(&limit.to_le_bytes());
            encode_frame(MSG_REQ_GET_CANDLES, &payload)
        }
    }
}

pub fn encode_response(msg: &ServerMessage) -> Vec<u8> {
    match msg {
        ServerMessage::StockList(stocks) => {
            let mut payload = Vec::new();
            let count: u16 = stocks.len().try_into().unwrap_or(u16::MAX);
            payload.extend_from_slice(&count.to_le_bytes());
            for s in stocks {
                let sym = s.symbol.as_bytes();
                let name = s.display_name.as_bytes();
                let sym_len = sym.len().min(MAX_SYMBOL_LEN) as u8;
                let name_len: u16 = name.len().min(MAX_NAME_LEN).try_into().unwrap_or(u16::MAX);
                payload.push(sym_len);
                payload.extend_from_slice(&sym[..sym_len as usize]);
                payload.extend_from_slice(&name_len.to_le_bytes());
                payload.extend_from_slice(&name[..name_len as usize]);
            }
            encode_frame(MSG_RSP_STOCK_LIST, &payload)
        }
        ServerMessage::CandleChunk {
            start_index,
            total,
            records,
        } => {
            debug_assert!(records.len() % KLINE_RECORD_SIZE == 0);
            let mut payload = Vec::with_capacity(16 + records.len());
            payload.extend_from_slice(&start_index.to_le_bytes());
            payload.extend_from_slice(&total.to_le_bytes());
            payload.extend_from_slice(records);
            encode_frame(MSG_RSP_CANDLE_CHUNK, &payload)
        }
        ServerMessage::Error(message) => {
            let bytes = message.as_bytes();
            let len: u16 = bytes.len().min(MAX_ERROR_LEN).try_into().unwrap_or(u16::MAX);
            let mut payload = Vec::with_capacity(2 + len as usize);
            payload.extend_from_slice(&len.to_le_bytes());
            payload.extend_from_slice(&bytes[..len as usize]);
            encode_frame(MSG_RSP_ERROR, &payload)
        }
    }
}

fn encode_frame(msg_type: u8, payload: &[u8]) -> Vec<u8> {
    let len = payload.len() as u32;
    let mut out = Vec::with_capacity(5 + payload.len());
    out.push(msg_type);
    out.extend_from_slice(&len.to_le_bytes());
    out.extend_from_slice(payload);
    out
}

#[cfg(test)]
pub fn decode_response(frame: &[u8]) -> anyhow::Result<ServerMessage> {
    if frame.len() < 5 {
        bail!("frame too short");
    }
    let msg_type = frame[0];
    let payload_len = u32::from_le_bytes(frame[1..5].try_into()?) as usize;
    if frame.len() != 5 + payload_len {
        bail!("frame length mismatch");
    }
    if payload_len > MAX_FRAME_PAYLOAD {
        bail!("payload too large");
    }
    let payload = &frame[5..];

    match msg_type {
        MSG_RSP_STOCK_LIST => decode_stock_list(payload),
        MSG_RSP_CANDLE_CHUNK => decode_candle_chunk(payload),
        MSG_RSP_ERROR => {
            if payload.len() < 2 {
                bail!("error payload too short");
            }
            let len = u16::from_le_bytes(payload[0..2].try_into()?) as usize;
            if payload.len() < 2 + len {
                bail!("error text truncated");
            }
            let text = std::str::from_utf8(&payload[2..2 + len])
                .context("error utf-8")?
                .to_string();
            Ok(ServerMessage::Error(text))
        }
        other => bail!("unknown response type: {other}"),
    }
}

#[cfg(test)]
fn decode_stock_list(payload: &[u8]) -> anyhow::Result<ServerMessage> {
    if payload.len() < 2 {
        bail!("stock list too short");
    }
    let count = u16::from_le_bytes(payload[0..2].try_into()?) as usize;
    let mut stocks = Vec::with_capacity(count);
    let mut off = 2;
    for _ in 0..count {
        if off >= payload.len() {
            bail!("truncated stock list");
        }
        let sym_len = payload[off] as usize;
        off += 1;
        if off + sym_len > payload.len() {
            bail!("truncated symbol");
        }
        let symbol = std::str::from_utf8(&payload[off..off + sym_len])
            .context("symbol utf-8")?
            .to_string();
        off += sym_len;
        if off + 2 > payload.len() {
            bail!("truncated name length");
        }
        let name_len = u16::from_le_bytes(payload[off..off + 2].try_into()?) as usize;
        off += 2;
        if off + name_len > payload.len() {
            bail!("truncated display name");
        }
        let display_name = std::str::from_utf8(&payload[off..off + name_len])
            .context("display name utf-8")?
            .to_string();
        off += name_len;
        stocks.push(StockEntry {
            symbol,
            display_name,
        });
    }
    Ok(ServerMessage::StockList(stocks))
}

#[cfg(test)]
fn decode_candle_chunk(payload: &[u8]) -> anyhow::Result<ServerMessage> {
    if payload.len() < 16 {
        bail!("candle chunk header too short");
    }
    let start_index = u64::from_le_bytes(payload[0..8].try_into()?);
    let total = u64::from_le_bytes(payload[8..16].try_into()?);
    let records = payload[16..].to_vec();
    if !records.len().is_multiple_of(KLINE_RECORD_SIZE) {
        bail!(
            "candle records size {} not multiple of {}",
            records.len(),
            KLINE_RECORD_SIZE
        );
    }
    Ok(ServerMessage::CandleChunk {
        start_index,
        total,
        records,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roundtrip_list_and_candles_req() {
        let req = ClientRequest::GetCandles {
            symbol: "002475".into(),
            before_index: Some(100),
            limit: 400,
        };
        let frame = encode_request(&req);
        let decoded = decode_request(&frame).unwrap();
        match decoded {
            ClientRequest::GetCandles {
                symbol,
                before_index,
                limit,
            } => {
                assert_eq!(symbol, "002475");
                assert_eq!(before_index, Some(100));
                assert_eq!(limit, 400);
            }
            _ => panic!("wrong variant"),
        }
    }

    #[test]
    fn roundtrip_candle_chunk() {
        let mut records = vec![0u8; 64];
        for (i, chunk) in records.chunks_mut(32).enumerate() {
            chunk[0..4].copy_from_slice(&(20200102i32 + i as i32).to_le_bytes());
        }
        let frame = encode_response(&ServerMessage::CandleChunk {
            start_index: 10,
            total: 1000,
            records,
        });
        match decode_response(&frame).unwrap() {
            ServerMessage::CandleChunk {
                start_index,
                total,
                records,
            } => {
                assert_eq!(start_index, 10);
                assert_eq!(total, 1000);
                assert_eq!(records.len(), 64);
            }
            _ => panic!("wrong variant"),
        }
    }
}
