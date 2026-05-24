//! One bar of MACD + KDJ wire values (24 bytes, little-endian i32 × 6).

pub const SIZE: usize = 24;

/// Field order on the wire (each i32 LE, value = round(indicator × 100)).
pub mod field {
    pub const MACD_DIF: usize = 0;
    pub const MACD_DEA: usize = 4;
    pub const MACD_BAR: usize = 8;
    pub const KDJ_K: usize = 12;
    pub const KDJ_D: usize = 16;
    pub const KDJ_J: usize = 20;
}

const _: () = {
    assert!(field::MACD_DIF == 0);
    assert!(field::MACD_DEA == 4);
    assert!(field::MACD_BAR == 8);
    assert!(field::KDJ_K == 12);
    assert!(field::KDJ_D == 16);
    assert!(field::KDJ_J == 20);
    assert!(field::KDJ_J + 4 == SIZE);
};

#[cfg(test)]
mod tests {
    use super::field::*;

    #[test]
    fn field_layout_covers_pack() {
        assert_eq!(KDJ_J + 4, super::SIZE);
    }
}
