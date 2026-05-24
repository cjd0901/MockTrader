use super::{SignalSide, TradeSignal};

const INITIAL_CAPITAL: f64 = 100_000.0;

#[derive(Debug, Clone, PartialEq)]
pub struct BacktestSummary {
    pub initial_capital: f64,
    pub final_equity: f64,
    pub total_return_pct: f64,
    pub round_trips: u32,
    pub win_count: u32,
    pub loss_count: u32,
    pub open_position: bool,
}

impl BacktestSummary {
    pub fn empty() -> Self {
        Self {
            initial_capital: INITIAL_CAPITAL,
            final_equity: INITIAL_CAPITAL,
            total_return_pct: 0.0,
            round_trips: 0,
            win_count: 0,
            loss_count: 0,
            open_position: false,
        }
    }
}

/// Long-only: full capital on each buy, flat on sell; mark open lots to `end_price` if given.
pub fn compute_pnl(signals: &[TradeSignal], end_price: Option<f64>) -> BacktestSummary {
    let mut cash = INITIAL_CAPITAL;
    let mut shares = 0.0_f64;
    let mut entry_price = 0.0_f64;
    let mut round_trips = 0_u32;
    let mut win_count = 0_u32;
    let mut loss_count = 0_u32;

    for sig in signals {
        match sig.side {
            SignalSide::Buy => {
                if shares > 0.0 {
                    continue;
                }
                if sig.price > 0.0 && cash > 0.0 {
                    shares = cash / sig.price;
                    entry_price = sig.price;
                    cash = 0.0;
                }
            }
            SignalSide::Sell => {
                if shares <= 0.0 || sig.price <= 0.0 {
                    continue;
                }
                let proceeds = shares * sig.price;
                round_trips += 1;
                if sig.price > entry_price {
                    win_count += 1;
                } else if sig.price < entry_price {
                    loss_count += 1;
                }
                cash = proceeds;
                shares = 0.0;
                entry_price = 0.0;
            }
        }
    }

    let mut open_position = false;
    if shares > 0.0 {
        if let Some(px) = end_price.filter(|p| *p > 0.0) {
            cash = shares * px;
            shares = 0.0;
            open_position = false;
        } else {
            open_position = true;
        }
    }

    let final_equity = if open_position {
        shares * end_price.unwrap_or(entry_price)
    } else {
        cash
    };

    let total_return_pct = if INITIAL_CAPITAL > 0.0 {
        (final_equity / INITIAL_CAPITAL - 1.0) * 100.0
    } else {
        0.0
    };

    BacktestSummary {
        initial_capital: INITIAL_CAPITAL,
        final_equity,
        total_return_pct,
        round_trips,
        win_count,
        loss_count,
        open_position,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::strategy::SignalSide;

    fn sig(side: SignalSide, price: f64) -> TradeSignal {
        TradeSignal {
            bar_index: 0,
            ts_sec: 0,
            side,
            price,
        }
    }

    #[test]
    fn round_trip_profit() {
        let signals = vec![sig(SignalSide::Buy, 10.0), sig(SignalSide::Sell, 11.0)];
        let s = compute_pnl(&signals, None);
        assert_eq!(s.round_trips, 1);
        assert_eq!(s.win_count, 1);
        assert!((s.total_return_pct - 10.0).abs() < 1e-6);
        assert!((s.final_equity - 110_000.0).abs() < 1e-3);
    }

    #[test]
    fn open_position_marked_to_market() {
        let signals = vec![sig(SignalSide::Buy, 10.0)];
        let s = compute_pnl(&signals, Some(10.5));
        assert!(!s.open_position);
        assert!((s.total_return_pct - 5.0).abs() < 1e-6);
    }
}
