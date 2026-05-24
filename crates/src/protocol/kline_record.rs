//! On-disk / wire candlestick record (32 bytes, little-endian i32 fields).

/// One bar in `.bin` files and in `CandleChunk` `records` block.
pub const SIZE: usize = 32;

/// Field byte offsets (all i32 LE unless noted). See `docs/PROTOCOL.md`.
pub mod off {
    pub const DATE: usize = 0;
    pub const TIME: usize = 4;
    pub const OPEN: usize = 8;
    pub const HIGH: usize = 12;
    pub const LOW: usize = 16;
    pub const CLOSE: usize = 20;
    pub const VOLUME: usize = 24;
    pub const AMOUNT: usize = 28;
}

const _: () = {
    assert!(off::DATE == 0);
    assert!(off::TIME == 4);
    assert!(off::OPEN == 8);
    assert!(off::HIGH == 12);
    assert!(off::LOW == 16);
    assert!(off::CLOSE == 20);
    assert!(off::VOLUME == 24);
    assert!(off::AMOUNT == 28);
    assert!(off::AMOUNT + 4 == SIZE);
};

pub fn read_i32(record: &[u8], offset: usize) -> i32 {
    i32::from_le_bytes(record[offset..offset + 4].try_into().expect("i32 field"))
}

/// Price fields are stored as `round(price * 100)`.
pub fn price_from_i32(v: i32) -> f64 {
    f64::from(v) / 100.0
}
