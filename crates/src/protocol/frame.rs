//! TCP frame envelope: `[u8 type][u32 LE length][payload]`.

use anyhow::{bail, Context};

pub const FRAME_HEADER_SIZE: usize = 5;
pub const MAX_FRAME_PAYLOAD: usize = 16 * 1024 * 1024;

/// Parsed frame header + slice into payload (borrows `bytes`).
pub struct Frame<'a> {
    pub msg_type: u8,
    pub payload: &'a [u8],
}

pub fn decode(bytes: &[u8]) -> anyhow::Result<Frame<'_>> {
    if bytes.len() < FRAME_HEADER_SIZE {
        bail!("frame too short");
    }
    let msg_type = bytes[0];
    let payload_len = u32::from_le_bytes(bytes[1..5].try_into().context("length")?) as usize;
    if bytes.len() != FRAME_HEADER_SIZE + payload_len {
        bail!("frame length mismatch");
    }
    if payload_len > MAX_FRAME_PAYLOAD {
        bail!("payload too large");
    }
    Ok(Frame {
        msg_type,
        payload: &bytes[FRAME_HEADER_SIZE..],
    })
}

pub fn encode(msg_type: u8, payload: &[u8]) -> Vec<u8> {
    let len = payload.len() as u32;
    let mut out = Vec::with_capacity(FRAME_HEADER_SIZE + payload.len());
    out.push(msg_type);
    out.extend_from_slice(&len.to_le_bytes());
    out.extend_from_slice(payload);
    out
}
