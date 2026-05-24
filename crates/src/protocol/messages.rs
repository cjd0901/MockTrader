//! Typed TCP request/response payloads (inside a frame).

use anyhow::{bail, Context};

use super::frame;
use super::kline_record;

pub mod msg_type {
    pub const GET_CANDLES: u8 = 2;
    pub const CANDLE_CHUNK: u8 = 102;
    pub const ERROR: u8 = 255;
}

const MAX_SYMBOL_LEN: usize = 32;
const MAX_ERROR_LEN: usize = 4096;

/// Chunk header size inside `CandleChunk` payload (before body).
pub const CANDLE_CHUNK_HEADER_SIZE: usize = 16;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GetCandlesRequest {
    pub symbol: String,
    pub before_index: Option<u64>,
    pub limit: u32,
}

impl GetCandlesRequest {
    pub fn decode_payload(payload: &[u8]) -> anyhow::Result<Self> {
        if payload.len() < 14 {
            bail!("GetCandles payload too short");
        }
        let sym_len = payload[0] as usize;
        if sym_len == 0 || sym_len > MAX_SYMBOL_LEN || payload.len() < 1 + sym_len + 13 {
            bail!("invalid symbol length");
        }
        let symbol = std::str::from_utf8(&payload[1..1 + sym_len])
            .context("symbol utf-8")?
            .to_string();

        let off = 1 + sym_len;
        let has_before = payload[off];
        let before_index = u64::from_le_bytes(payload[off + 1..off + 9].try_into()?);
        let limit = u32::from_le_bytes(payload[off + 9..off + 13].try_into()?);

        if limit == 0 {
            bail!("limit must be > 0");
        }

        Ok(Self {
            symbol,
            before_index: if has_before != 0 {
                Some(before_index)
            } else {
                None
            },
            limit,
        })
    }
}

/// `CandleChunk` body = `records` (N×32) then `indicators` (N×24), same bar order.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CandleChunkBody {
    pub start_index: u64,
    pub total: u64,
    pub records: Vec<u8>,
    pub indicators: Vec<u8>,
}

impl CandleChunkBody {
    pub fn bar_count(&self) -> usize {
        if self.records.len() % kline_record::SIZE != 0 {
            return 0;
        }
        self.records.len() / kline_record::SIZE
    }

    pub fn encode_payload(&self) -> Vec<u8> {
        const BAR_BYTES: usize = kline_record::SIZE + super::indicator_pack::SIZE;
        debug_assert!(self.records.len().is_multiple_of(kline_record::SIZE));
        let bar_count = self.bar_count();
        debug_assert_eq!(self.records.len() + self.indicators.len(), bar_count * BAR_BYTES);
        debug_assert_eq!(self.indicators.len(), bar_count * super::indicator_pack::SIZE);

        let mut payload =
            Vec::with_capacity(CANDLE_CHUNK_HEADER_SIZE + self.records.len() + self.indicators.len());
        payload.extend_from_slice(&self.start_index.to_le_bytes());
        payload.extend_from_slice(&self.total.to_le_bytes());
        payload.extend_from_slice(&self.records);
        payload.extend_from_slice(&self.indicators);
        payload
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ErrorText(pub String);

impl ErrorText {
    pub fn encode_payload(message: &str) -> Vec<u8> {
        let bytes = message.as_bytes();
        let len: u16 = bytes.len().min(MAX_ERROR_LEN).try_into().unwrap_or(u16::MAX);
        let mut payload = Vec::with_capacity(2 + len as usize);
        payload.extend_from_slice(&len.to_le_bytes());
        payload.extend_from_slice(&bytes[..len as usize]);
        payload
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ClientRequest {
    GetCandles(GetCandlesRequest),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ServerMessage {
    CandleChunk(CandleChunkBody),
    Error(ErrorText),
}

pub fn decode_request(frame_bytes: &[u8]) -> anyhow::Result<ClientRequest> {
    let frame = frame::decode(frame_bytes)?;
    match frame.msg_type {
        msg_type::GET_CANDLES => Ok(ClientRequest::GetCandles(
            GetCandlesRequest::decode_payload(frame.payload)?,
        )),
        other => bail!("unknown request type: {other}"),
    }
}

pub fn encode_response(msg: &ServerMessage) -> Vec<u8> {
    match msg {
        ServerMessage::CandleChunk(body) => {
            frame::encode(msg_type::CANDLE_CHUNK, &body.encode_payload())
        }
        ServerMessage::Error(ErrorText(text)) => {
            frame::encode(msg_type::ERROR, &ErrorText::encode_payload(text))
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const CHUNK_BODY_BAR_BYTES: usize =
        kline_record::SIZE + crate::protocol::indicator_pack::SIZE;

    impl GetCandlesRequest {
        fn encode_payload(&self) -> Vec<u8> {
            let sym_bytes = self.symbol.as_bytes();
            let sym_len = sym_bytes.len().min(MAX_SYMBOL_LEN);
            let mut payload = Vec::with_capacity(1 + sym_len + 13);
            payload.push(sym_len as u8);
            payload.extend_from_slice(&sym_bytes[..sym_len]);
            if let Some(idx) = self.before_index {
                payload.push(1);
                payload.extend_from_slice(&idx.to_le_bytes());
            } else {
                payload.push(0);
                payload.extend_from_slice(&0u64.to_le_bytes());
            }
            payload.extend_from_slice(&self.limit.to_le_bytes());
            payload
        }
    }

    impl CandleChunkBody {
        fn decode_payload(payload: &[u8]) -> anyhow::Result<Self> {
            if payload.len() < CANDLE_CHUNK_HEADER_SIZE {
                bail!("CandleChunk header too short");
            }
            let start_index = u64::from_le_bytes(payload[0..8].try_into()?);
            let total = u64::from_le_bytes(payload[8..16].try_into()?);
            let body = &payload[CANDLE_CHUNK_HEADER_SIZE..];

            if body.is_empty() {
                return Ok(Self {
                    start_index,
                    total,
                    records: Vec::new(),
                    indicators: Vec::new(),
                });
            }

            if body.len() % CHUNK_BODY_BAR_BYTES != 0 {
                bail!(
                    "CandleChunk body {} bytes is not N×{} (records+indicators)",
                    body.len(),
                    CHUNK_BODY_BAR_BYTES
                );
            }

            let bar_count = body.len() / CHUNK_BODY_BAR_BYTES;
            let records_len = bar_count * kline_record::SIZE;
            let records = body[..records_len].to_vec();
            let indicators = body[records_len..].to_vec();

            Ok(Self {
                start_index,
                total,
                records,
                indicators,
            })
        }
    }

    impl ErrorText {
        fn decode_payload(payload: &[u8]) -> anyhow::Result<Self> {
            if payload.len() < 2 {
                bail!("Error payload too short");
            }
            let len = u16::from_le_bytes(payload[0..2].try_into()?) as usize;
            if payload.len() < 2 + len {
                bail!("Error text truncated");
            }
            let text = std::str::from_utf8(&payload[2..2 + len])
                .context("error utf-8")?
                .to_string();
            Ok(Self(text))
        }
    }

    fn encode_request(req: &ClientRequest) -> Vec<u8> {
        match req {
            ClientRequest::GetCandles(inner) => {
                frame::encode(msg_type::GET_CANDLES, &inner.encode_payload())
            }
        }
    }

    fn decode_response(frame_bytes: &[u8]) -> anyhow::Result<ServerMessage> {
        let frame = frame::decode(frame_bytes)?;
        match frame.msg_type {
            msg_type::CANDLE_CHUNK => Ok(ServerMessage::CandleChunk(
                CandleChunkBody::decode_payload(frame.payload)?,
            )),
            msg_type::ERROR => Ok(ServerMessage::Error(ErrorText::decode_payload(
                frame.payload,
            )?)),
            other => bail!("unknown response type: {other}"),
        }
    }

    #[test]
    fn roundtrip_get_candles() {
        let req = ClientRequest::GetCandles(GetCandlesRequest {
            symbol: "002475".into(),
            before_index: Some(100),
            limit: 400,
        });
        let decoded = decode_request(&encode_request(&req)).unwrap();
        assert_eq!(
            decoded,
            ClientRequest::GetCandles(GetCandlesRequest {
                symbol: "002475".into(),
                before_index: Some(100),
                limit: 400,
            })
        );
    }

    #[test]
    fn roundtrip_candle_chunk() {
        let mut records = vec![0u8; 64];
        for (i, chunk) in records.chunks_mut(kline_record::SIZE).enumerate() {
            chunk[0..4].copy_from_slice(&(20200102i32 + i as i32).to_le_bytes());
        }
        let indicators = vec![0u8; 48];
        let body = CandleChunkBody {
            start_index: 10,
            total: 1000,
            records,
            indicators,
        };
        let msg = ServerMessage::CandleChunk(body);
        match decode_response(&encode_response(&msg)).unwrap() {
            ServerMessage::CandleChunk(decoded) => {
                assert_eq!(decoded.start_index, 10);
                assert_eq!(decoded.total, 1000);
                assert_eq!(decoded.bar_count(), 2);
            }
            ServerMessage::Error(_) => panic!("wrong variant"),
        }
    }
}
