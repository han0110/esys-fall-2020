use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug)]
pub enum EventKind {
    // when variation ratio of luminosity over the threshold
    Luminosity {
        from: u32,
        to: u32,
    },
    // when variation ratio of position over the threshold
    Position {
        from: (u32, u32, u32),
        to: (u32, u32, u32),
    },
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Event {
    pub timestamp: u32,
    pub secret: u32,
    pub kind: EventKind,
}

#[cfg(test)]
mod tests {
    use super::*;

    macro_rules! test_serialize {
        ($name:ident, $event:expr, $expected:expr) => {
            #[test]
            fn $name() {
                let bin = bincode::serialize(&$event).unwrap();
                assert_eq!(bin, $expected);
            }
        };
    }

    test_serialize!(
        luminosity,
        Event {
            timestamp: 1608119495,
            secret: 12345678,
            kind: EventKind::Luminosity { from: 10, to: 800 },
        },
        vec![
            199, 244, 217, 95, // timestamp
            78, 97, 188, 0, // secret
            0, 0, 0, 0, // kind
            10, 0, 0, 0, // from
            32, 3, 0, 0 // to
        ]
    );
    test_serialize!(
        position,
        Event {
            timestamp: 1608119495,
            secret: 12345678,
            kind: EventKind::Position {
                from: (0, 3, 12),
                to: (400, 600, 11)
            },
        },
        vec![
            199, 244, 217, 95, // timestamp
            78, 97, 188, 0, // secret
            1, 0, 0, 0, // kind
            0, 0, 0, 0, // from x
            3, 0, 0, 0, // from y
            12, 0, 0, 0, // from z
            144, 1, 0, 0, // to x
            88, 2, 0, 0, // to y
            11, 0, 0, 0 // to z
        ]
    );
}
