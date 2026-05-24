//! TCP binary protocol for K-line + indicator streaming.
//!
//! See `docs/PROTOCOL.md` for the wire layout.

pub mod frame;
pub mod indicator_pack;
pub mod kline_record;
pub mod messages;

pub use frame::{FRAME_HEADER_SIZE, MAX_FRAME_PAYLOAD};
pub use indicator_pack::SIZE as INDICATOR_PACK_SIZE;
pub use kline_record::SIZE as KLINE_RECORD_SIZE;
pub use messages::{
    decode_request, encode_response, CandleChunkBody, ClientRequest, ErrorText,
    GetCandlesRequest, ServerMessage,
};
