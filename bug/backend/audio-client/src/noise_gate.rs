// Modified from https://github.com/Michael-F-Bryan/noise-gate with MIT license:
//
// Copyright (c) 2019 Michael-F-Bryan <michaelfbryan@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

use dasp::sample::{Sample, SignedSample};

pub trait Sink<S> {
    fn record(&mut self, sample: S);
    fn end_of_transmission(&mut self);
}

#[derive(Debug, Clone, PartialEq)]
pub struct NoiseGate<S>
where
    S: Sample,
{
    pub open_threshold: S,
    pub release_time: usize,
    state: State,
}

impl<S> NoiseGate<S>
where
    S: Sample,
{
    pub fn new(open_threshold: S, release_time: usize) -> Self {
        NoiseGate {
            open_threshold,
            release_time,
            state: State::Closed,
        }
    }

    pub fn is_open(&self) -> bool {
        match self.state {
            State::Open | State::Closing { .. } => true,
            State::Closed => false,
        }
    }

    pub fn is_closed(&self) -> bool {
        !self.is_open()
    }

    pub fn process<D>(&mut self, samples: &[S], sink: &mut D)
    where
        D: Sink<S>,
    {
        for &sample in samples {
            let was_open = self.is_open();

            self.next_state(sample);

            if self.is_open() {
                sink.record(sample);
            } else if was_open {
                sink.end_of_transmission();
            }
        }
    }

    fn next_state(&mut self, sample: S) {
        self.state = match self.state {
            State::Open => {
                if below_threshold(sample, self.open_threshold) {
                    State::Closing {
                        remaining_samples: self.release_time,
                    }
                } else {
                    State::Open
                }
            }

            State::Closing { remaining_samples } => {
                if below_threshold(sample, self.open_threshold) {
                    if remaining_samples == 0 {
                        State::Closed
                    } else {
                        State::Closing {
                            remaining_samples: remaining_samples - 1,
                        }
                    }
                } else {
                    State::Open
                }
            }

            State::Closed => {
                if below_threshold(sample, self.open_threshold) {
                    State::Closed
                } else {
                    State::Open
                }
            }
        };
    }
}

#[derive(Debug, Copy, Clone, PartialEq)]
enum State {
    Open,
    Closing { remaining_samples: usize },
    Closed,
}

fn below_threshold<S>(sample: S, threshold: S) -> bool
where
    S: Sample,
{
    let sample = sample.to_signed_sample();
    let threshold = abs(threshold.to_signed_sample());
    let negated_threshold = S::EQUILIBRIUM.to_signed_sample() - threshold;

    negated_threshold < sample && sample < threshold
}

fn abs<S: SignedSample>(sample: S) -> S {
    let zero = S::EQUILIBRIUM;
    if sample >= zero {
        sample
    } else {
        -sample
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const OPEN_THRESHOLD: i16 = 100;
    const RELEASE_TIME: usize = 5;

    macro_rules! test_state_transition {
        ($name:ident: $from:expr, $sample:expr => $expected:expr) => {
            #[test]
            fn $name() {
                let mut noise_gate = NoiseGate::new(OPEN_THRESHOLD, RELEASE_TIME);
                noise_gate.state = $from;
                noise_gate.next_state($sample);
                assert_eq!(noise_gate.state, $expected);
            }
        };
    }

    test_state_transition!(open_to_open: State::Open, 101 => State::Open);
    test_state_transition!(open_to_closing: State::Open, 40 => State::Closing { remaining_samples: RELEASE_TIME });
    test_state_transition!(closing_to_closed: State::Closing { remaining_samples: 0 }, 40 => State::Closed);
    test_state_transition!(closing_to_closing: State::Closing { remaining_samples: 1 }, 40 => State::Closing { remaining_samples: 0 });
    test_state_transition!(reopen_when_closing: State::Closing { remaining_samples: 1 }, 101 => State::Open);
    test_state_transition!(closed_to_closed: State::Closed, 40 => State::Closed);
    test_state_transition!(closed_to_open: State::Closed, 101 => State::Open);
}
